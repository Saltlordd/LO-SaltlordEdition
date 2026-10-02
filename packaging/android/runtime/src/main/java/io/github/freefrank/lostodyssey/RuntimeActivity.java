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
        if (touchControls != null) touchControls.clearTouches();
        super.onPause();
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
            for (String name : new String[] { "LO_VS_DEBUG", "LO_PS_DEBUG",
                    "LO_NO_ALPHATEST", "LO_DEBUG_CAPTURE_SWAP", "LO_TRACE_INPUT",
                    "LO_CLEAR_RT", "LO_NO_SHADER_PREPARE", "LO_DRAW_TRACE", "LO_DRAW_TRACE_COUNT" }) {
                String value = getIntent().getStringExtra(name);
                if (value != null) nativeSetenv(name, value);
            }
        }
        return new String[] { "--game", game.getAbsolutePath(), "--quiet-kernel" };
    }
}
