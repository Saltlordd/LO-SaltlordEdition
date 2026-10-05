package io.github.freefrank.lostodyssey;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

public class SaveTransferTest {
    @Rule public TemporaryFolder temporary = new TemporaryFolder();

    private static final long SAVED_AT = 1_790_000_000_000L;

    private static byte[] zip(String... namesAndContents) throws IOException {
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        try (ZipOutputStream zip = new ZipOutputStream(bytes)) {
            for (int i = 0; i < namesAndContents.length; i += 2) {
                ZipEntry entry = new ZipEntry(namesAndContents[i]);
                entry.setTime(SAVED_AT);
                zip.putNextEntry(entry);
                zip.write(namesAndContents[i + 1].getBytes("UTF-8"));
                zip.closeEntry();
            }
        }
        return bytes.toByteArray();
    }

    private static List<SaveTransfer.Slot> read(byte[] zip) throws IOException {
        return SaveTransfer.readZip(new ByteArrayInputStream(zip));
    }

    private File slot(File saveRoot, String name, String save) throws IOException {
        File slot = new File(saveRoot, name);
        assertTrue(slot.mkdirs());
        Files.write(new File(slot, "save.bin").toPath(), save.getBytes("UTF-8"));
        assertTrue(new File(slot, "save.bin").setLastModified(SAVED_AT));
        return slot;
    }

    @Test
    public void exportedSavesImportUnchanged() throws IOException {
        File source = temporary.newFolder("source", "save");
        slot(source, "user00", "first");
        Files.write(new File(source, "user00/.lo-content").toPath(), SaveTransfer.contentData("user00"));
        slot(source, "user03", "second");
        // Folders the runtime would not list are left out.
        assertTrue(new File(source, "empty").mkdirs());
        assertEquals(2, SaveTransfer.listSlots(source).size());

        ByteArrayOutputStream exported = new ByteArrayOutputStream();
        SaveTransfer.exportZip(SaveTransfer.listSlots(source), exported);
        List<SaveTransfer.Slot> slots = read(exported.toByteArray());
        assertEquals(2, slots.size());

        File target = temporary.newFolder("target", "save");
        for (SaveTransfer.Slot slot : slots) SaveTransfer.install(slot, target);
        for (String name : new String[] { "user00", "user03" }) {
            File save = new File(target, name + "/save.bin");
            assertArrayEquals(Files.readAllBytes(new File(source, name + "/save.bin").toPath()),
                Files.readAllBytes(save.toPath()));
            // Continue loads the newest save, so the time must survive (ZIP keeps 2 s steps).
            assertEquals(SAVED_AT / 2000, save.lastModified() / 2000);
            assertTrue(new File(target, name + "/.lo-content").isFile());
        }
        assertFalse(new File(target.getParentFile(), "save-transfer/user00").exists());
    }

    @Test
    public void acceptsConverterAndXeniaLayouts() throws IOException {
        List<SaveTransfer.Slot> converter = read(zip(
            "README.txt", "notes", "conversion.json", "{}",
            "save/user05/save.bin", "data", "save/user05/.lo-content", "meta"));
        assertEquals(1, converter.size());
        assertEquals("user05", converter.get(0).name);
        assertEquals(2, converter.get(0).files.size());

        List<SaveTransfer.Slot> xenia = read(zip(
            "B13EBABEBABEBABE/4D5307FA/00000001/user00/save.bin", "a",
            "B13EBABEBABEBABE/4D5307FA/00000001/user00/__thumbnail.png", "png",
            "B13EBABEBABEBABE/4D5307FA/00000001/user01/save.bin", "b",
            "__MACOSX/user00/._save.bin", "junk"));
        assertEquals(2, xenia.size());

        // A Xenia save has no .lo-content; import adds one naming the folder.
        File target = temporary.newFolder("save");
        SaveTransfer.install(xenia.get(0), target);
        assertArrayEquals(SaveTransfer.contentData("user00"),
            Files.readAllBytes(new File(target, "user00/.lo-content").toPath()));
        assertTrue(new File(target, "user00/__thumbnail.png").isFile());
    }

    @Test
    public void replacesAnExistingSlotWhole() throws IOException {
        File target = temporary.newFolder("save");
        File old = slot(target, "user00", "old");
        Files.write(new File(old, "stale.dat").toPath(), new byte[] { 1 });
        SaveTransfer.install(read(zip("user00/save.bin", "new")).get(0), target);
        assertEquals("new", new String(Files.readAllBytes(new File(target, "user00/save.bin").toPath()), "UTF-8"));
        assertFalse(new File(target, "user00/stale.dat").exists());
    }

    @Test
    public void rejectsUnsafeOrUnusableZips() throws IOException {
        String[][] bad = {
            { "../user00/save.bin", "x" },
            { "save/../../user00/save.bin", "x" },
            { "/user00/save.bin", "x" },
            { "save.bin", "x" },
            { "user00/save.bin", "\0\0\0" },
            { "a/user00/save.bin", "x", "b/user00/save.bin", "y" },
            { ".hidden/save.bin", "x" },
            { "notes.txt", "no saves" },
        };
        for (String[] entries : bad) {
            try {
                read(zip(entries));
                fail("accepted " + entries[0]);
            } catch (SaveTransfer.InvalidArchiveException expected) {
                // rejected as intended
            }
        }
        try {
            read("not a zip".getBytes("UTF-8"));
            fail("accepted a file that is not a ZIP");
        } catch (SaveTransfer.InvalidArchiveException expected) {
            // no entries means no saves
        }
    }

    @Test
    public void slotNamesFollowTheRuntimeRule() {
        assertTrue(SaveTransfer.validSlotName("user00"));
        assertFalse(SaveTransfer.validSlotName(""));
        assertFalse(SaveTransfer.validSlotName(".."));
        assertFalse(SaveTransfer.validSlotName("a:b"));
        assertFalse(SaveTransfer.validSlotName("x".repeat(42)));
        assertTrue(SaveTransfer.validSlotName("x".repeat(41)));
    }
}
