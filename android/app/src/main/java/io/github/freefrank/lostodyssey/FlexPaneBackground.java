package io.github.freefrank.lostodyssey;

import android.graphics.drawable.GradientDrawable;

/** Opaque, static backdrop for the separate Flex controls pane. */
final class FlexPaneBackground {
    private FlexPaneBackground() {}

    static GradientDrawable create() {
        // Keep the pane dark even when the user's control opacity is low.
        // Its opacity is independent of the touch overlay's opacity setting.
        return new GradientDrawable(GradientDrawable.Orientation.TOP_BOTTOM,
                new int[] { 0xff10161d, 0xff0b1520, 0xff080f17 });
    }
}
