package io.github.freefrank.lostodyssey;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

/**
 * Save export and import for players who cannot reach the app's private
 * storage. The runtime keeps each slot in {@code <filesDir>/save/<slot>/}
 * (kernel/xam.cpp); a ZIP holds them as {@code save/<slot>/<file>}, the layout
 * of the PC save folder and of the RGH save converter's download.
 *
 * Import takes any folder that holds a {@code save.bin} as a slot, at any
 * depth, so ZIPs of a PC {@code save/} folder or of Xenia's {@code 00000001/}
 * folder work too. File times are kept: Continue loads the newest save.
 */
final class SaveTransfer {
    static final String DIRECTORY = "save";
    static final String SAVE_FILE = "save.bin";
    private static final String CONTENT_FILE = ".lo-content";
    private static final String STAGING = "save-transfer";
    private static final long MAX_FILE_BYTES = 16L << 20;
    private static final long MAX_TOTAL_BYTES = 256L << 20;
    // XCONTENT_DATA as kernel/xam.cpp stores it in .lo-content (tools/import_xenia_saves.py).
    private static final int MAX_FILE_NAME = 42;
    private static final int MAX_DISPLAY_NAME = 128;
    private static final int CONTENT_DATA_SIZE = 308;

    private SaveTransfer() {}

    static final class InvalidArchiveException extends IOException {
        InvalidArchiveException(String message) { super(message); }
    }

    static final class SaveFile {
        final String name;
        final byte[] data;
        final long time;
        SaveFile(String name, byte[] data, long time) { this.name = name; this.data = data; this.time = time; }
    }

    static final class Slot {
        final String name;
        final List<SaveFile> files = new ArrayList<>();
        Slot(String name) { this.name = name; }
    }

    static File saveRoot(File filesDir) {
        return new File(filesDir, DIRECTORY);
    }

    /** The runtime's content-name rule (ValidContentName), plus no hidden folders. */
    static boolean validSlotName(String name) {
        return name != null && !name.isEmpty() && !name.startsWith(".")
            && name.getBytes(StandardCharsets.UTF_8).length < MAX_FILE_NAME
            && name.indexOf('/') < 0 && name.indexOf('\\') < 0 && name.indexOf(':') < 0;
    }

    /** Slot folders with a non-empty save.bin, by name. */
    static List<File> listSlots(File saveRoot) {
        List<File> slots = new ArrayList<>();
        File[] children = saveRoot.listFiles();
        if (children == null) return slots;
        Arrays.sort(children);
        for (File child : children)
            if (child.isDirectory() && validSlotName(child.getName()) && new File(child, SAVE_FILE).length() > 0)
                slots.add(child);
        return slots;
    }

    /** Writes the slots as save/<slot>/<file> entries with their file times. */
    static void exportZip(List<File> slots, OutputStream output) throws IOException {
        ZipOutputStream zip = new ZipOutputStream(output);
        byte[] buffer = new byte[64 * 1024];
        for (File slot : slots) {
            File[] files = slot.listFiles(File::isFile);
            if (files == null) continue;
            Arrays.sort(files);
            for (File file : files) {
                ZipEntry entry = new ZipEntry(DIRECTORY + "/" + slot.getName() + "/" + file.getName());
                entry.setTime(file.lastModified());
                zip.putNextEntry(entry);
                try (InputStream input = new FileInputStream(file)) {
                    for (int n; (n = input.read(buffer)) > 0; ) zip.write(buffer, 0, n);
                }
                zip.closeEntry();
            }
        }
        zip.finish();
    }

    /**
     * Reads the slots from a ZIP: the folders that directly hold a save.bin and
     * the files beside it. Throws for unsafe paths, oversized files, a save.bin
     * outside a folder, an empty save, or two folders with the same slot name.
     */
    static List<Slot> readZip(InputStream input) throws IOException {
        Map<String, List<SaveFile>> folders = new LinkedHashMap<>();
        long total = 0;
        ZipInputStream zip = new ZipInputStream(input);
        for (ZipEntry entry; (entry = zip.getNextEntry()) != null; zip.closeEntry()) {
            if (entry.isDirectory()) continue;
            String path = entry.getName().replace('\\', '/');
            String padded = "/" + path + "/";
            if (path.startsWith("/") || padded.contains("/../") || padded.contains("/./"))
                throw new InvalidArchiveException("unsafe path in the ZIP: " + entry.getName());
            int slash = path.lastIndexOf('/');
            String folder = slash < 0 ? "" : path.substring(0, slash);
            String name = path.substring(slash + 1);
            byte[] data = readBounded(zip, name);
            total += data.length;
            if (total > MAX_TOTAL_BYTES) throw new InvalidArchiveException("the ZIP is too large for saves");
            folders.computeIfAbsent(folder, ignored -> new ArrayList<>())
                .add(new SaveFile(name, data, entry.getTime()));
        }

        List<Slot> slots = new ArrayList<>();
        for (Map.Entry<String, List<SaveFile>> folder : folders.entrySet()) {
            SaveFile save = null;
            for (SaveFile file : folder.getValue()) if (file.name.equals(SAVE_FILE)) save = file;
            if (save == null) continue;
            String path = folder.getKey();
            if (path.isEmpty())
                throw new InvalidArchiveException("save.bin must be inside its slot folder, such as user00/save.bin");
            String name = path.substring(path.lastIndexOf('/') + 1);
            if (!validSlotName(name)) throw new InvalidArchiveException("unsupported slot name: " + name);
            if (allZero(save.data)) throw new InvalidArchiveException(name + "/save.bin is empty");
            for (Slot other : slots)
                if (other.name.equals(name)) throw new InvalidArchiveException("the ZIP holds two " + name + " folders");
            Slot slot = new Slot(name);
            slot.files.addAll(folder.getValue());
            slots.add(slot);
        }
        if (slots.isEmpty()) throw new InvalidArchiveException("no save folders (with save.bin) in the ZIP");
        return slots;
    }

    /**
     * Writes a slot beside the save folder, then renames it into place, so a
     * failed import leaves the old slot as it was. Adds a .lo-content when the
     * source (Xenia) has none.
     */
    static void install(Slot slot, File saveRoot) throws IOException {
        File staging = new File(saveRoot.getParentFile(), STAGING);
        File incoming = new File(staging, slot.name);
        File previous = new File(staging, slot.name + ".previous");
        deleteRecursively(incoming);
        deleteRecursively(previous);
        if (!incoming.mkdirs()) throw new IOException("cannot create " + incoming);
        boolean hasContent = false;
        for (SaveFile file : slot.files) {
            if (file.name.equals(CONTENT_FILE)) hasContent = true;
            write(new File(incoming, file.name), file.data, file.time);
        }
        if (!hasContent) write(new File(incoming, CONTENT_FILE), contentData(slot.name), -1);

        File target = new File(saveRoot, slot.name);
        if (!saveRoot.isDirectory() && !saveRoot.mkdirs()) throw new IOException("cannot create " + saveRoot);
        if (target.exists() && !target.renameTo(previous)) throw new IOException("cannot replace " + target);
        if (!incoming.renameTo(target)) {
            //noinspection ResultOfMethodCallIgnored
            previous.renameTo(target);
            throw new IOException("cannot move " + incoming + " to " + target);
        }
        deleteRecursively(previous);
    }

    /** Big-endian XCONTENT_DATA: device 1, save data, the folder name as file and display name. */
    static byte[] contentData(String name) {
        ByteBuffer data = ByteBuffer.allocate(CONTENT_DATA_SIZE);
        data.putInt(1).putInt(1);
        byte[] display = name.getBytes(StandardCharsets.UTF_16BE);
        data.put(display, 0, Math.min(display.length, (MAX_DISPLAY_NAME - 1) * 2));
        data.position(8 + MAX_DISPLAY_NAME * 2);
        data.put(name.getBytes(StandardCharsets.UTF_8));
        return data.array();
    }

    private static byte[] readBounded(InputStream input, String name) throws IOException {
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        byte[] buffer = new byte[64 * 1024];
        for (int n; (n = input.read(buffer)) > 0; ) {
            if (output.size() + n > MAX_FILE_BYTES) throw new InvalidArchiveException(name + " is too large for a save");
            output.write(buffer, 0, n);
        }
        return output.toByteArray();
    }

    private static void deleteRecursively(File file) {
        File[] children = file.listFiles();
        if (children != null) for (File child : children) deleteRecursively(child);
        //noinspection ResultOfMethodCallIgnored
        file.delete();
    }

    private static boolean allZero(byte[] data) {
        for (byte value : data) if (value != 0) return false;
        return true;
    }

    private static void write(File file, byte[] data, long time) throws IOException {
        try (FileOutputStream output = new FileOutputStream(file)) {
            output.write(data);
            output.getFD().sync();
        }
        //noinspection ResultOfMethodCallIgnored
        if (time > 0) file.setLastModified(time);
    }
}
