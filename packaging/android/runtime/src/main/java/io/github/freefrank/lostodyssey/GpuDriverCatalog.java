package io.github.freefrank.lostodyssey;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Driver packages offered for download: the GitHub release feeds of the Turnip
 * builders that the Eden emulator's driver fetcher lists, read with the same
 * per-source display rules, plus Eden's recommendation table by Adreno model.
 */
final class GpuDriverCatalog {
    static final String USER_AGENT = "LostOdysseyRecomp-Android";
    static final int RELEASES_PER_SOURCE = 20;
    static final long CACHE_FRESH_MS = 60L * 60L * 1000L;
    static final String CACHE_DIRECTORY = "gpu_driver_catalog";

    /** One GitHub repository whose releases carry driver zips. */
    static final class Source {
        final String name, path;
        /** Show the release tag instead of its title (titles are generic there). */
        final boolean useTagName;
        /** Order releases by publish time instead of the feed order. */
        final boolean sortByPublishTime;

        Source(String name, String path, boolean useTagName, boolean sortByPublishTime) {
            this.name = name;
            this.path = path;
            this.useTagName = useTagName;
            this.sortByPublishTime = sortByPublishTime;
        }

        String releasesUrl() {
            return "https://api.github.com/repos/" + path + "/releases?per_page=" + RELEASES_PER_SOURCE;
        }

        String cacheName() { return path.replace('/', '_') + ".json"; }
    }

    /** Same sources and flags as Eden's driver fetcher. */
    static final List<Source> SOURCES = Collections.unmodifiableList(Arrays.asList(
        new Source("Mr. Purple Turnip", "MrPurple666/purple-turnip", false, false),
        new Source("GameHub Adreno 8xx", "crueter/GameHub-8Elite-Drivers", false, false),
        new Source("KIMCHI Turnip", "K11MCH1/AdrenoToolsDrivers", true, true),
        new Source("Weab-Chan Freedreno", "Weab-chan/freedreno_turnip-CI", false, false),
        new Source("Whitebelyash Turnip", "whitebelyash/freedreno_turnip-CI", false, true)));

    static final class Asset {
        final String name, url;
        final long size;

        Asset(String name, String url, long size) {
            this.name = name;
            this.url = url;
            this.size = size;
        }
    }

    static final class Release {
        final String title, publishedAt;
        final boolean prerelease;
        final List<Asset> assets;
        boolean latest;

        Release(String title, String publishedAt, boolean prerelease, List<Asset> assets) {
            this.title = title;
            this.publishedAt = publishedAt;
            this.prerelease = prerelease;
            this.assets = assets;
        }

        String date() {
            return publishedAt.length() >= 10 ? publishedAt.substring(0, 10) : publishedAt;
        }
    }

    /** Result of reading one source: releases, or an error, possibly from the cache. */
    static final class Feed {
        final Source source;
        final List<Release> releases;
        final String error;
        final boolean fromCache;

        Feed(Source source, List<Release> releases, String error, boolean fromCache) {
            this.source = source;
            this.releases = releases;
            this.error = error;
            this.fromCache = fromCache;
        }
    }

    interface Progress { void update(long done, long total); }

    private GpuDriverCatalog() {}

    /**
     * Releases with at least one zip asset. Prereleases are listed but never
     * "latest"; the first stable release in the final order is marked latest.
     */
    static List<Release> parseReleases(String json, Source source) throws JSONException {
        JSONArray array = new JSONArray(json);
        List<Release> releases = new ArrayList<>();
        for (int i = 0; i < array.length(); ++i) {
            JSONObject object = array.getJSONObject(i);
            List<Asset> assets = new ArrayList<>();
            JSONArray assetArray = object.optJSONArray("assets");
            for (int j = 0; assetArray != null && j < assetArray.length(); ++j) {
                JSONObject asset = assetArray.getJSONObject(j);
                String name = asset.optString("name", "");
                String url = asset.optString("browser_download_url", "");
                if (!name.toLowerCase(Locale.ROOT).endsWith(".zip") || !url.startsWith("https://")) continue;
                assets.add(new Asset(name, url, asset.optLong("size", 0)));
            }
            if (assets.isEmpty()) continue;
            String title = source.useTagName ? object.optString("tag_name", "") : object.optString("name", "");
            if (title.trim().isEmpty()) title = object.optString("tag_name", "release");
            releases.add(new Release(title.trim(), object.optString("published_at", ""),
                object.optBoolean("prerelease", false), assets));
        }
        if (source.sortByPublishTime) {
            Collections.sort(releases, (a, b) -> b.publishedAt.compareTo(a.publishedAt));
        }
        for (Release release : releases) {
            if (!release.prerelease) { release.latest = true; break; }
        }
        return releases;
    }

    private static final Pattern ADRENO = Pattern.compile("Adreno\\D*(\\d+)", Pattern.CASE_INSENSITIVE);

    /** Adreno model number from "Adreno750v2" or "Adreno (TM) 750"; 0 when unknown. */
    static int adrenoModel(String gpuModel) {
        if (gpuModel == null) return 0;
        Matcher matcher = ADRENO.matcher(gpuModel);
        if (!matcher.find()) return 0;
        try {
            return Integer.parseInt(matcher.group(1));
        } catch (NumberFormatException e) {
            return 0;
        }
    }

    /** Eden's recommendation for an Adreno model number (its table, unchanged). */
    static String edenRecommendation(int model) {
        if (model < 10) return "Unsupported";
        if (model < 100) return "KIMCHI Latest";
        if (model < 600) return "Unsupported";
        if (model < 640) return "Mr. Purple EOL-24.3.4";
        if (model < 700) return "Mr. Purple T19";
        if (model <= 710) return "KIMCHI 25.2.0_r5";
        if (model < 800) return "Mr. Purple T23";
        if (model < 900) return "GameHub Adreno 8xx";
        return "Unsupported";
    }

    /** Reads one source, serving a cached copy when it is fresh or the network fails. */
    static Feed fetch(File cacheRoot, Source source, boolean forceRefresh) {
        File cacheDirectory = new File(cacheRoot, CACHE_DIRECTORY);
        File cache = new File(cacheDirectory, source.cacheName());
        boolean cacheFresh = cache.isFile() && System.currentTimeMillis() - cache.lastModified() < CACHE_FRESH_MS;
        if (cacheFresh && !forceRefresh) {
            Feed cached = fromCache(source, cache, null);
            if (cached != null) return cached;
        }
        String error;
        try {
            String json = get(source.releasesUrl(), "application/vnd.github+json");
            List<Release> releases = parseReleases(json, source);
            //noinspection ResultOfMethodCallIgnored
            cacheDirectory.mkdirs();
            try (OutputStream output = new FileOutputStream(cache)) {
                output.write(json.getBytes(StandardCharsets.UTF_8));
            }
            return new Feed(source, releases, null, false);
        } catch (IOException | JSONException e) {
            error = e.getMessage() == null ? e.getClass().getSimpleName() : e.getMessage();
        }
        Feed cached = fromCache(source, cache, error);
        return cached != null ? cached : new Feed(source, Collections.emptyList(), error, false);
    }

    private static Feed fromCache(Source source, File cache, String error) {
        if (!cache.isFile()) return null;
        try (InputStream input = new FileInputStream(cache)) {
            byte[] bytes = new byte[(int) Math.min(cache.length(), 8L << 20)];
            int offset = 0;
            for (int read; offset < bytes.length && (read = input.read(bytes, offset, bytes.length - offset)) > 0; ) offset += read;
            return new Feed(source, parseReleases(new String(bytes, 0, offset, StandardCharsets.UTF_8), source), error, true);
        } catch (IOException | JSONException e) {
            return null;
        }
    }

    private static String get(String url, String accept) throws IOException {
        HttpURLConnection connection = open(url, accept);
        try {
            int code = connection.getResponseCode();
            if (code / 100 != 2) throw new IOException(describe(connection, code));
            try (InputStream input = connection.getInputStream()) {
                StringBuilder text = new StringBuilder();
                byte[] chunk = new byte[16384];
                for (int read; (read = input.read(chunk)) > 0; ) {
                    text.append(new String(chunk, 0, read, StandardCharsets.UTF_8));
                    if (text.length() > (8 << 20)) throw new IOException("response too large");
                }
                return text.toString();
            }
        } finally {
            connection.disconnect();
        }
    }

    private static HttpURLConnection open(String url, String accept) throws IOException {
        HttpURLConnection connection = (HttpURLConnection) new URL(url).openConnection();
        connection.setInstanceFollowRedirects(true);
        connection.setConnectTimeout(15000);
        connection.setReadTimeout(30000);
        connection.setRequestProperty("User-Agent", USER_AGENT);
        connection.setRequestProperty("Accept", accept);
        return connection;
    }

    /** "HTTP 403, GitHub rate limit resets at 14:05" or "HTTP 500". */
    static String describe(HttpURLConnection connection, int code) {
        String remaining = connection.getHeaderField("X-RateLimit-Remaining");
        String reset = connection.getHeaderField("X-RateLimit-Reset");
        if ((code == 403 || code == 429) && "0".equals(remaining) && reset != null) {
            try {
                long seconds = Long.parseLong(reset.trim());
                return "HTTP " + code + ", GitHub rate limit resets at "
                    + String.format(Locale.ROOT, "%tR", seconds * 1000L);
            } catch (NumberFormatException ignored) {
                // fall through to the plain code
            }
        }
        return "HTTP " + code;
    }

    /** Downloads an asset into a file, reporting byte progress; the file is removed on failure. */
    static File download(Asset asset, File directory, Progress progress) throws IOException {
        //noinspection ResultOfMethodCallIgnored
        directory.mkdirs();
        File target = new File(directory, GpuDriverStore.slug(asset.name) + ".zip");
        HttpURLConnection connection = open(asset.url, "application/octet-stream");
        try {
            int code = connection.getResponseCode();
            if (code / 100 != 2) throw new IOException(describe(connection, code));
            long total = connection.getContentLengthLong();
            if (total <= 0) total = asset.size;
            long done = 0;
            try (InputStream input = connection.getInputStream();
                 OutputStream output = new FileOutputStream(target)) {
                byte[] chunk = new byte[65536];
                for (int read; (read = input.read(chunk)) > 0; ) {
                    output.write(chunk, 0, read);
                    done += read;
                    if (done > GpuDriverStore.MAX_LIBRARY_BYTES) throw new IOException("package too large");
                    if (progress != null) progress.update(done, total);
                }
            }
            if (done == 0) throw new IOException("empty download");
            return target;
        } catch (IOException | RuntimeException e) {
            //noinspection ResultOfMethodCallIgnored
            target.delete();
            throw e;
        } finally {
            connection.disconnect();
        }
    }
}
