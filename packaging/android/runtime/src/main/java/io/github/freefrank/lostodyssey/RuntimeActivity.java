package io.github.freefrank.lostodyssey;

import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.widget.RelativeLayout;
import java.io.File;
import org.libsdl.app.SDLActivity;

/** Experimental game runtime. User-supplied discs live in app-owned storage. */
public final class RuntimeActivity extends SDLActivity {
    /** A start that stays in the foreground this long counts as reached for the driver choice. */
    private static final long BOOT_SETTLED_MS = 15000;

    private TouchControlsView touchControls;
    private final Handler bootHandler = new Handler(Looper.getMainLooper());
    private final Runnable bootSettled = () -> GpuDriverStore.clearBootPending(this);

    static native void nativeSetTouchInput(int buttons, int leftTrigger, int rightTrigger,
                                           int leftX, int leftY, int rightX, int rightY);
    static native boolean nativeHasConnectedController();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        GameStorage.prepare(this);
        GpuDriverStore.markBootPending(this);
        if (mLayout != null && !mBrokenLibraries) {
            touchControls = new TouchControlsView(this);
            mLayout.addView(touchControls, new RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.MATCH_PARENT,
                RelativeLayout.LayoutParams.MATCH_PARENT));
            nativeSetTouchInput(0, 0, 0, 0, 0, 0, 0);
        }
    }

    /** Opens the GPU driver page over the game (CTRL dialog). */
    void openGpuDriverPage() {
        Intent intent = new Intent(this, GpuDriverActivity.class);
        intent.putExtra(GpuDriverActivity.EXTRA_FROM_GAME, true);
        startActivity(intent);
    }

    @Override
    protected void onPause() {
        if (touchControls != null) touchControls.onHostPause();
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (touchControls != null) touchControls.onHostResume();
        bootHandler.removeCallbacks(bootSettled);
        bootHandler.postDelayed(bootSettled, BOOT_SETTLED_MS);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        if (!hasFocus && touchControls != null) touchControls.clearTouches();
        super.onWindowFocusChanged(hasFocus);
    }

    @Override
    protected void onDestroy() {
        if (touchControls != null) touchControls.clearTouches();
        bootHandler.removeCallbacks(bootSettled);
        // A normal exit is not a failed start, however short it was.
        GpuDriverStore.clearBootPending(this);
        super.onDestroy();
    }

    @Override
    protected String[] getLibraries() {
        return new String[] { "c++_shared", "SDL2", "main" };
    }

    @Override
    protected String[] getArguments() {
        File game = GameStorage.disc1(this);
        // Custom Vulkan driver (libadrenotools): the hook libraries sit in the
        // extracted native library directory, the chosen package in its own folder.
        nativeSetenv("LO_NATIVE_LIB_DIR", getApplicationInfo().nativeLibraryDir + "/");
        GpuDriverStore.Installed driver = GpuDriverStore.selectedDriver(this);
        if (driver != null) {
            nativeSetenv("LO_CUSTOM_DRIVER_DIR", driver.directory.getAbsolutePath() + "/");
            nativeSetenv("LO_VK_CUSTOM_DRIVER", driver.metadata.libraryName);
        }
        // Debug launches can select an isolated fixture without changing saves.
        if ((getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0) {
            String override = getIntent().getStringExtra("game_root");
            if (override != null && !override.isEmpty()) game = new File(override);
            // Expose the existing renderer diagnostics to ADB on development APKs.
            // A fresh process is required when changing these native switches.
            // Every LO_* extra is forwarded: `am start --es LO_DEBUG_CAPTURE_SWAP 3000`.
            Bundle extras = getIntent().getExtras();
            if (extras != null) {
                for (String name : extras.keySet()) {
                    if (!name.startsWith("LO_")) continue;
                    Object value = extras.get(name);
                    if (value != null) nativeSetenv(name, String.valueOf(value));
                }
            }
        }
        return new String[] { "--game", game.getAbsolutePath(), "--quiet-kernel" };
    }
}
