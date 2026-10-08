package io.github.freefrank.lostodyssey;

import android.app.AlertDialog;
import android.content.ClipData;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;
import android.widget.Toast;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.concurrent.CountDownLatch;
import org.libsdl.app.SDLActivity;

/** SDL owns lifecycle, native thread, window and controllers. */
public final class ProbeActivity extends SDLActivity {
    private volatile CountDownLatch resultsClosed;

    @Override protected String[] getLibraries() {
        return new String[] { "SDL2", "lo_android_probe" };
    }
    @Override protected String[] getArguments() {
        return new String[] { getFilesDir().getAbsolutePath(), getCacheDir().getAbsolutePath() };
    }

    // Called on SDL's native thread. Keep SDL_main alive while the share sheet
    // is open, otherwise SDLActivity would finish and interrupt the handoff.
    public void showProbeResultsAndWait(final String summary) {
        final CountDownLatch closed = new CountDownLatch(1);
        resultsClosed = closed;
        runOnUiThread(() -> {
            if (isFinishing() || isDestroyed()) { closed.countDown(); return; }
            AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle("Lost Odyssey XEX Test results")
                .setMessage(summary)
                .setPositiveButton("Share log", null)
                .setNegativeButton("Close", (d, which) -> closed.countDown())
                .setOnCancelListener(d -> closed.countDown())
                .create();
            dialog.show();
            // Retain the results dialog when sharing so the user can return or retry.
            dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v -> shareLog());
        });
        try { closed.await(); }
        catch (InterruptedException e) { Thread.currentThread().interrupt(); }
        finally { resultsClosed = null; }
    }

    private void shareLog() {
        File source = new File(getFilesDir(), "state/logs/android-phase1.log");
        File directory = new File(getCacheDir(), "log-export");
        File snapshot = new File(directory, "android-phase1.log");
        try {
            if (!directory.isDirectory() && !directory.mkdirs())
                throw new IOException("Cannot create log export folder");
            // Share a file snapshot, not a large Intent text payload or private file:// URI.
            try (FileInputStream input = new FileInputStream(source);
                 FileOutputStream output = new FileOutputStream(snapshot)) {
                byte[] buffer = new byte[8192];
                int count;
                while ((count = input.read(buffer)) != -1) output.write(buffer, 0, count);
            }
            Uri uri = LogProvider.logUri(this);
            Intent send = new Intent(Intent.ACTION_SEND)
                .setType("text/plain")
                .putExtra(Intent.EXTRA_STREAM, uri)
                .putExtra(Intent.EXTRA_SUBJECT, "Lost Odyssey Android diagnostic log")
                .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            send.setClipData(ClipData.newRawUri("Lost Odyssey diagnostic log", uri));
            startActivity(Intent.createChooser(send, "Share diagnostic log"));
        } catch (IOException | RuntimeException e) {
            Log.e("LostOdysseyRecomp", "Log sharing failed", e);
            Toast.makeText(this, "Couldn't share the log: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }
    }

    @Override protected void onDestroy() {
        CountDownLatch closed = resultsClosed;
        if (closed != null) closed.countDown();
        super.onDestroy();
    }
}
