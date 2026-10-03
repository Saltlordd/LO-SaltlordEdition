package io.github.freefrank.lostodyssey;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;
import org.json.JSONException;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

public class GpuDriverStoreTest {
    @Rule public TemporaryFolder temporary = new TemporaryFolder();

    private static final String META = "{\"schemaVersion\":1,\"name\":\"Turnip R8\",\"description\":\"d\","
        + "\"author\":\"KIMCHI\",\"packageVersion\":\"1\",\"vendor\":\"Mesa\",\"driverVersion\":\"Vulkan 1.4\","
        + "\"minApi\":0,\"libraryName\":\"vulkan.ad07xx.so\"}";

    private static byte[] zip(String[][] entries) throws IOException {
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        try (ZipOutputStream zip = new ZipOutputStream(bytes)) {
            for (String[] entry : entries) {
                zip.putNextEntry(new ZipEntry(entry[0]));
                zip.write(entry[1].getBytes(StandardCharsets.UTF_8));
                zip.closeEntry();
            }
        }
        return bytes.toByteArray();
    }

    private GpuDriverStore.Installed install(byte[] zip, String name) throws IOException {
        return GpuDriverStore.installFromStream(new ByteArrayInputStream(zip), new File(temporary.getRoot(), name));
    }

    @Test
    public void parsesMetadataAndRequiresNameAndLibrary() throws Exception {
        GpuDriverStore.Metadata metadata = GpuDriverStore.Metadata.parse(META);
        assertEquals("Turnip R8", metadata.name);
        assertEquals("vulkan.ad07xx.so", metadata.libraryName);
        assertEquals("Vulkan 1.4 · KIMCHI\nd", metadata.summary());
        for (String bad : new String[] {
                "{\"libraryName\":\"a.so\"}", "{\"name\":\"x\"}", "{\"name\":\"\",\"libraryName\":\"a.so\"}",
                "{\"name\":\"x\",\"libraryName\":\"../a.so\"}", "not json" }) {
            try {
                GpuDriverStore.Metadata.parse(bad);
                fail(bad);
            } catch (JSONException expected) {
                // rejected
            }
        }
    }

    @Test
    public void extractsAValidPackage() throws Exception {
        GpuDriverStore.Installed driver = install(zip(new String[][] {
            { "meta.json", META }, { "vulkan.ad07xx.so", "ELF" } }), "pkg");
        assertEquals("Turnip R8", driver.metadata.name);
        assertEquals("pkg", driver.id());
        assertTrue(driver.library().isFile());
        assertTrue(new File(driver.directory, GpuDriverStore.META_FILE).isFile());
        assertEquals("Turnip R8", GpuDriverStore.read(driver.directory).metadata.name);
    }

    @Test
    public void rejectsPackagesWithoutTheNamedLibraryAndCleansUp() throws Exception {
        File directory = new File(temporary.getRoot(), "bad");
        try {
            install(zip(new String[][] { { "meta.json", META }, { "other.so", "ELF" } }), "bad");
            fail();
        } catch (GpuDriverStore.InvalidPackageException expected) {
            assertTrue(expected.getMessage().contains("vulkan.ad07xx.so"));
        }
        assertFalse(directory.exists());
    }

    @Test
    public void rejectsNestedEntriesAndMissingMetadata() throws Exception {
        try {
            install(zip(new String[][] { { "sub/meta.json", META }, { "vulkan.ad07xx.so", "ELF" } }), "nested");
            fail();
        } catch (GpuDriverStore.InvalidPackageException expected) {
            assertTrue(expected.getMessage().contains("unexpected entry"));
        }
        try {
            install(zip(new String[][] { { "vulkan.ad07xx.so", "ELF" } }), "nometa");
            fail();
        } catch (GpuDriverStore.InvalidPackageException expected) {
            assertTrue(expected.getMessage().contains("meta.json"));
        }
    }

    @Test
    public void rejectsPackagesNeedingANewerApi() throws Exception {
        // Build.VERSION.SDK_INT is 0 in the JVM stubs, so any positive minApi is too new.
        String meta = META.replace("\"minApi\":0", "\"minApi\":27");
        try {
            install(zip(new String[][] { { "meta.json", meta }, { "vulkan.ad07xx.so", "ELF" } }), "api");
            fail();
        } catch (GpuDriverStore.InvalidPackageException expected) {
            assertTrue(expected.getMessage().contains("API 27"));
        }
    }

    @Test
    public void slugsAreFilesystemSafe() {
        assertEquals("Turnip_v26.0.0_R8", GpuDriverStore.slug("Turnip_v26.0.0_R8.zip"));
        assertEquals("a_b_c", GpuDriverStore.slug("dir/a b:c.ZIP"));
        assertEquals("hidden", GpuDriverStore.slug("..hidden.zip"));
        assertEquals("driver", GpuDriverStore.slug(".zip"));
        assertEquals(64, GpuDriverStore.slug(new String(new char[100]).replace('\0', 'x') + ".zip").length());
    }

    @Test
    public void arm64Detection() {
        assertTrue(GpuDriverStore.isArm64(new String[] { "arm64-v8a", "armeabi-v7a" }));
        assertFalse(GpuDriverStore.isArm64(new String[] { "armeabi-v7a" }));
        assertFalse(GpuDriverStore.isArm64(null));
    }
}
