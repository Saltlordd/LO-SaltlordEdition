package io.github.freefrank.lostodyssey;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.system.Os;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.security.MessageDigest;

/** Select only the small executable needed for the loader diagnostic. */
public final class TestLauncherActivity extends Activity {
    private static final int SELECT_XEX = 41;
    private static final long EXPECTED_BYTES = 6623232;
    private static final String EXPECTED_SHA256 = "175ae53d109d480a83bebbd186e7b6871f7b03ce80af69ab388db2f747640de3";
    private TextView status;
    private Button choose;
    private Button run;

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        int padding = Math.round(24 * getResources().getDisplayMetrics().density);
        layout.setPadding(padding, padding, padding, padding);
        TextView title = new TextView(this);
        title.setText("Lost Odyssey XEX Test");
        title.setTextSize(24);
        layout.addView(title);
        TextView explanation = new TextView(this);
        explanation.setText("Choose the Disc 1 default.xex you extracted. This test loads and verifies the executable; it cannot play the game. No ISO or other game assets are needed.");
        explanation.setTextSize(16);
        layout.addView(explanation);
        status = new TextView(this);
        status.setText(xexFile().isFile() ? "Disc 1 executable is ready for testing." : "Disc 1 executable has not been selected.");
        layout.addView(status);
        choose = new Button(this);
        choose.setText("Choose Disc 1 default.xex");
        choose.setOnClickListener(v -> {
            Intent pick = new Intent(Intent.ACTION_OPEN_DOCUMENT)
                .addCategory(Intent.CATEGORY_OPENABLE).setType("*/*");
            startActivityForResult(pick, SELECT_XEX);
        });
        layout.addView(choose);
        run = new Button(this);
        run.setText("Run tests");
        run.setOnClickListener(v -> runTests());
        layout.addView(run);
        setContentView(layout);
    }

    private File xexFile() { return new File(getFilesDir(), "game/disc1/default.xex"); }
    private void runTests() { startActivity(new Intent(this, ProbeActivity.class)); }

    @Override protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request != SELECT_XEX || result != RESULT_OK || data == null || data.getData() == null) return;
        final Uri uri = data.getData();
        choose.setEnabled(false);
        run.setEnabled(false);
        status.setText("Checking and copying the Disc 1 executable…");
        new Thread(() -> {
            File temp = null;
            String error = null;
            try {
                File directory = xexFile().getParentFile();
                if (!directory.isDirectory() && !directory.mkdirs()) throw new Exception("Cannot create the test folder.");
                temp = File.createTempFile("default-xex-", ".tmp", directory);
                MessageDigest digest = MessageDigest.getInstance("SHA-256");
                long total = 0;
                try (InputStream input = getContentResolver().openInputStream(uri);
                     FileOutputStream output = new FileOutputStream(temp)) {
                    if (input == null) throw new Exception("Cannot open the selected file.");
                    byte[] buffer = new byte[32768];
                    int count;
                    while ((count = input.read(buffer)) != -1) {
                        total += count;
                        if (total > EXPECTED_BYTES) throw new Exception("This is larger than the Disc 1 executable. Select default.xex, not the ISO.");
                        digest.update(buffer, 0, count);
                        output.write(buffer, 0, count);
                    }
                    output.getFD().sync();
                }
                StringBuilder hash = new StringBuilder();
                for (byte value : digest.digest()) hash.append(String.format(java.util.Locale.ROOT, "%02x", value & 255));
                if (total != EXPECTED_BYTES || !EXPECTED_SHA256.equals(hash.toString()))
                    throw new Exception("This file does not match the Disc 1 default.xex supplied for this test.");
                // Same-directory atomic replacement keeps the previous good file
                // intact when selection, verification or copying fails.
                Os.rename(temp.getAbsolutePath(), xexFile().getAbsolutePath());
            } catch (Exception e) { error = e.getMessage() != null ? e.getMessage() : "Could not copy the executable."; }
            finally { if (temp != null && temp.exists()) temp.delete(); }
            final String failure = error;
            runOnUiThread(() -> {
                if (isFinishing() || isDestroyed()) return;
                choose.setEnabled(true);
                run.setEnabled(true);
                status.setText(failure == null ? "Disc 1 executable verified. Starting tests…" : failure);
                if (failure == null) runTests();
            });
        }, "XEX-selection").start();
    }
}
