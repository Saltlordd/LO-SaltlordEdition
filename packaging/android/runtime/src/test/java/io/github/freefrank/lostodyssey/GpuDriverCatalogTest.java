package io.github.freefrank.lostodyssey;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.util.List;
import org.junit.Test;

public class GpuDriverCatalogTest {
    private static final GpuDriverCatalog.Source PLAIN =
        new GpuDriverCatalog.Source("Plain", "a/b", false, false);
    private static final GpuDriverCatalog.Source TAGGED =
        new GpuDriverCatalog.Source("Tagged", "a/c", true, true);

    private static String release(String name, String tag, String published, boolean prerelease, String... assets) {
        StringBuilder json = new StringBuilder("{\"name\":\"" + name + "\",\"tag_name\":\"" + tag
            + "\",\"published_at\":\"" + published + "\",\"prerelease\":" + prerelease + ",\"assets\":[");
        for (int i = 0; i < assets.length; ++i) {
            if (i > 0) json.append(',');
            json.append("{\"name\":\"").append(assets[i]).append("\",\"size\":1234,\"browser_download_url\":\"https://x/")
                .append(assets[i]).append("\"}");
        }
        return json.append("]}").toString();
    }

    @Test
    public void keepsOnlyZipAssetsAndDropsEmptyReleases() throws Exception {
        String json = "[" + release("Driver 2", "v2", "2026-02-01T00:00:00Z", false, "turnip.zip", "notes.txt")
            + "," + release("Source only", "v1", "2026-01-01T00:00:00Z", false, "source.tar.gz") + "]";
        List<GpuDriverCatalog.Release> releases = GpuDriverCatalog.parseReleases(json, PLAIN);
        assertEquals(1, releases.size());
        assertEquals("Driver 2", releases.get(0).title);
        assertEquals(1, releases.get(0).assets.size());
        assertEquals("turnip.zip", releases.get(0).assets.get(0).name);
        assertEquals("https://x/turnip.zip", releases.get(0).assets.get(0).url);
        assertEquals("2026-02-01", releases.get(0).date());
    }

    @Test
    public void latestIsTheFirstStableReleaseAfterSorting() throws Exception {
        String json = "[" + release("old", "v1", "2026-01-01T00:00:00Z", false, "a.zip")
            + "," + release("beta", "v3-rc", "2026-03-01T00:00:00Z", true, "b.zip")
            + "," + release("new", "v2", "2026-02-01T00:00:00Z", false, "c.zip") + "]";
        List<GpuDriverCatalog.Release> releases = GpuDriverCatalog.parseReleases(json, TAGGED);
        assertEquals("v3-rc", releases.get(0).title);
        assertEquals("v2", releases.get(1).title);
        assertEquals("v1", releases.get(2).title);
        assertFalse(releases.get(0).latest);
        assertTrue(releases.get(1).latest);
        assertFalse(releases.get(2).latest);
    }

    @Test
    public void plainSourcesKeepFeedOrderAndTitles() throws Exception {
        String json = "[" + release("", "v9", "2026-01-01T00:00:00Z", false, "a.zip")
            + "," + release("Named", "v8", "2026-05-01T00:00:00Z", false, "b.zip") + "]";
        List<GpuDriverCatalog.Release> releases = GpuDriverCatalog.parseReleases(json, PLAIN);
        assertEquals("v9", releases.get(0).title);
        assertEquals("Named", releases.get(1).title);
        assertTrue(releases.get(0).latest);
    }

    @Test
    public void rejectsNonHttpsAssets() throws Exception {
        String json = "[{\"name\":\"x\",\"tag_name\":\"x\",\"assets\":[{\"name\":\"a.zip\",\"browser_download_url\":\"http://x/a.zip\"}]}]";
        assertTrue(GpuDriverCatalog.parseReleases(json, PLAIN).isEmpty());
    }

    @Test
    public void parsesAdrenoModelNumbers() {
        assertEquals(750, GpuDriverCatalog.adrenoModel("Adreno750v2"));
        assertEquals(740, GpuDriverCatalog.adrenoModel("Adreno (TM) 740"));
        assertEquals(830, GpuDriverCatalog.adrenoModel("Adreno830"));
        assertEquals(0, GpuDriverCatalog.adrenoModel("Mali-G78"));
        assertEquals(0, GpuDriverCatalog.adrenoModel(null));
    }

    @Test
    public void mirrorsEdenRecommendationTable() {
        assertEquals("Unsupported", GpuDriverCatalog.edenRecommendation(0));
        assertEquals("KIMCHI Latest", GpuDriverCatalog.edenRecommendation(32));
        assertEquals("Mr. Purple EOL-24.3.4", GpuDriverCatalog.edenRecommendation(610));
        assertEquals("Mr. Purple T19", GpuDriverCatalog.edenRecommendation(650));
        assertEquals("KIMCHI 25.2.0_r5", GpuDriverCatalog.edenRecommendation(710));
        assertEquals("Mr. Purple T23", GpuDriverCatalog.edenRecommendation(750));
        assertEquals("GameHub Adreno 8xx", GpuDriverCatalog.edenRecommendation(830));
        assertEquals("Unsupported", GpuDriverCatalog.edenRecommendation(900));
    }

    @Test
    public void sourcesMatchEden() {
        assertEquals(5, GpuDriverCatalog.SOURCES.size());
        assertEquals("K11MCH1/AdrenoToolsDrivers", GpuDriverCatalog.SOURCES.get(2).path);
        assertTrue(GpuDriverCatalog.SOURCES.get(2).useTagName);
        assertEquals("https://api.github.com/repos/K11MCH1/AdrenoToolsDrivers/releases?per_page=20",
            GpuDriverCatalog.SOURCES.get(2).releasesUrl());
    }
}
