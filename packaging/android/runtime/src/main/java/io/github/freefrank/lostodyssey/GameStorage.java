package io.github.freefrank.lostodyssey;

import android.content.Context;
import android.media.MediaScannerConnection;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Where the user's game data lives: {@code Android/data/<package>/files/game/}
 * on the internal shared storage or on an SD card.
 *
 * Android creates that folder only when the app first asks for it, and a PC
 * sees new folders over USB (MTP) only after the media scanner has indexed a
 * file inside them. The launcher therefore creates the disc folders on every
 * volume and scans a README in each, so they are there to copy into.
 */
final class GameStorage {
    static final String[] DISCS = { "disc1", "disc2", "disc3", "disc4" };
    static final String README = "README.txt";
    private static final String README_TEXT =
        "Copy the imported game data here: disc1/default.xex must exist.\n"
        + "Put disc2-disc4 beside disc1 and DLC in dlc/<content-id>.\n"
        + "The game uses the first storage (internal or SD card) whose disc1 has default.xex,\n"
        + "otherwise this folder on the internal storage.\n";

    private GameStorage() {}

    /** The {@code files} folder of every mounted shared-storage volume, internal first. */
    static List<File> volumes(Context context) {
        List<File> result = new ArrayList<>();
        File[] dirs = context.getExternalFilesDirs(null);
        if (dirs != null) for (File dir : Arrays.asList(dirs)) if (dir != null) result.add(dir);
        return result;
    }

    /** Creates {@code game/disc1}-{@code disc4} and a README on each volume; returns the README paths written. */
    static List<String> createFolders(List<File> volumes) {
        List<String> written = new ArrayList<>();
        for (File volume : volumes) {
            File game = new File(volume, "game");
            for (String disc : DISCS) new File(game, disc).mkdirs();
            File readme = new File(game, README);
            if (readme.exists() || !game.isDirectory()) continue;
            try (OutputStream output = new FileOutputStream(readme)) {
                output.write(README_TEXT.getBytes(StandardCharsets.UTF_8));
                written.add(readme.getAbsolutePath());
            } catch (IOException ignored) {
                // A read-only or removed card keeps its folders without the README.
            }
        }
        return written;
    }

    /** Makes the game folders and shows them to a PC connected over USB. */
    static void prepare(Context context) {
        List<String> written = createFolders(volumes(context));
        if (!written.isEmpty())
            MediaScannerConnection.scanFile(context.getApplicationContext(),
                written.toArray(new String[0]), null, null);
    }

    /** The first volume whose {@code game/disc1} holds {@code default.xex}, else the first volume's. */
    static File selectDisc1(List<File> volumes, File fallback) {
        for (File volume : volumes) {
            File disc1 = new File(volume, "game/disc1");
            if (new File(disc1, "default.xex").isFile()) return disc1;
        }
        return new File(volumes.isEmpty() ? fallback : volumes.get(0), "game/disc1");
    }

    static File disc1(Context context) {
        return selectDisc1(volumes(context), context.getFilesDir());
    }
}
