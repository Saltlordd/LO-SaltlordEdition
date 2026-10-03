package io.github.freefrank.lostodyssey;

import android.content.pm.ApplicationInfo;
import android.os.Bundle;
import android.widget.RelativeLayout;
import java.io.File;
import org.libsdl.app.SDLActivity;

/** Experimental game runtime. User-supplied discs live in app-owned storage. */
public final class RuntimeActivity extends SDLActivity {
    private TouchControlsView touchControls;

    static native void nativeSetTouchInput(int buttons, int leftTrigger, int rightTrigger,
                                           int leftX, int leftY, int rightX, int rightY);
    static native boolean nativeHasConnectedController();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (mLayout != null && !mBrokenLibraries) {
            touchControls = new TouchControlsView(this);
            mLayout.addView(touchControls, new RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.MATCH_PARENT,
                RelativeLayout.LayoutParams.MATCH_PARENT));
            nativeSetTouchInput(0, 0, 0, 0, 0, 0, 0);
        }
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
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        if (!hasFocus && touchControls != null) touchControls.clearTouches();
        super.onWindowFocusChanged(hasFocus);
    }

    @Override
    protected void onDestroy() {
        if (touchControls != null) touchControls.clearTouches();
        super.onDestroy();
    }

    @Override
    protected String[] getLibraries() {
        return new String[] { "c++_shared", "SDL2", "main" };
    }

    @Override
    protected String[] getArguments() {
        File external = getExternalFilesDir(null);
        File game = new File(external != null ? external : getFilesDir(), "game/disc1");
        // Debug launches can select an isolated fixture without changing saves.
        if ((getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0) {
            String override = getIntent().getStringExtra("game_root");
            if (override != null && !override.isEmpty()) game = new File(override);
            // Expose the existing renderer diagnostics to ADB on development APKs.
            // A fresh process is required when changing these native switches.
            // Custom Vulkan driver loading (libadrenotools) needs these paths.
            nativeSetenv("LO_NATIVE_LIB_DIR", getApplicationInfo().nativeLibraryDir + "/");
            nativeSetenv("LO_CUSTOM_DRIVER_DIR", getFilesDir().getAbsolutePath() + "/gpu_driver/");
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
