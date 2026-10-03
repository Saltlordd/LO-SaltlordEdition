package io.github.freefrank.lostodyssey;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.io.File;
import java.io.IOException;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

public class GameStorageTest {
    @Rule public TemporaryFolder temporary = new TemporaryFolder();

    @Test
    public void createsDiscFoldersAndReadmeOnEveryVolumeOnce() throws IOException {
        File internal = temporary.newFolder("internal", "files");
        File card = temporary.newFolder("card", "files");
        List<String> written = GameStorage.createFolders(Arrays.asList(internal, card));
        assertEquals(2, written.size());
        for (File volume : Arrays.asList(internal, card)) {
            for (String disc : GameStorage.DISCS) assertTrue(new File(volume, "game/" + disc).isDirectory());
            assertTrue(new File(volume, "game/" + GameStorage.README).isFile());
        }
        assertTrue(GameStorage.createFolders(Arrays.asList(internal, card)).isEmpty());
    }

    @Test
    public void picksTheVolumeThatHoldsDisc1() throws IOException {
        File internal = temporary.newFolder("internal", "files");
        File card = temporary.newFolder("card", "files");
        List<File> volumes = Arrays.asList(internal, card);
        GameStorage.createFolders(volumes);
        assertEquals(new File(internal, "game/disc1"), GameStorage.selectDisc1(volumes, internal));
        assertTrue(new File(card, "game/disc1/default.xex").createNewFile());
        assertEquals(new File(card, "game/disc1"), GameStorage.selectDisc1(volumes, internal));
        assertTrue(new File(internal, "game/disc1/default.xex").createNewFile());
        assertEquals(new File(internal, "game/disc1"), GameStorage.selectDisc1(volumes, internal));
    }

    @Test
    public void fallsBackToPrivateStorageWithoutVolumes() {
        File fallback = temporary.getRoot();
        assertEquals(new File(fallback, "game/disc1"), GameStorage.selectDisc1(Collections.emptyList(), fallback));
    }
}
