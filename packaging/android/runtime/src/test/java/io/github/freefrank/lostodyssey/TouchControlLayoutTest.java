package io.github.freefrank.lostodyssey;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class TouchControlLayoutTest {
    private static final int W = 2560, H = 1600;
    private static final float UNIT = 180f;

    private static float distanceToCtrl(TouchControlLayout layout, int id) {
        return (float) Math.hypot(layout.pixelX(id, W) - layout.ctrlX * W,
                                  layout.pixelY(id, H) - layout.ctrlY * H);
    }

    @Test
    public void ctrlStaysOnScreenAndBelowTheTopStrip() {
        TouchControlLayout layout = TouchControlLayout.defaults(W, H, UNIT);
        assertEquals(.5f, layout.ctrlX, 0f);
        layout.moveCtrl(-500, -500, W, H, UNIT, 0f);
        assertEquals(90f, layout.ctrlX * W, 1e-2f);
        assertEquals(90f + H * TouchControlLayout.TOP_SAFE, layout.ctrlY * H, 1e-2f);
        layout.moveCtrl(5000, 5000, W, H, UNIT, 0f);
        assertEquals(W - 90f, layout.ctrlX * W, 1e-2f);
        assertEquals(H - 90f, layout.ctrlY * H, 1e-2f);
    }

    @Test
    public void ctrlDroppedOnAButtonPushesTheButtonClear() {
        TouchControlLayout layout = TouchControlLayout.defaults(W, H, UNIT);
        int id = TouchControlLayout.A;
        layout.moveCtrl(layout.pixelX(id, W), layout.pixelY(id, H), W, H, UNIT, 0f);
        layout.clampAll(W, H, UNIT, 0f);
        float clearance = TouchControlLayout.hitRadius(id) * UNIT + UNIT * .52f;
        assertTrue(distanceToCtrl(layout, id) >= clearance - .5f);
    }

    @Test
    public void defaultsKeepBackAndStartClearOfCtrl() {
        TouchControlLayout layout = TouchControlLayout.defaults(W, H, UNIT * 1.4f);
        for (int id : new int[] { TouchControlLayout.BACK, TouchControlLayout.START }) {
            float clearance = TouchControlLayout.hitRadius(id) * UNIT * 1.4f + UNIT * 1.4f * .52f;
            assertTrue(distanceToCtrl(layout, id) >= clearance - .5f);
        }
    }
}
