package io.github.freefrank.lostodyssey;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class TouchCtrlHandleTest {
    private static final long T0 = 10_000;

    private static TouchCtrlHandle fresh() {
        TouchCtrlHandle handle = new TouchCtrlHandle();
        handle.reset(T0);
        return handle;
    }

    @Test
    public void retractsAfterIdleDelayAndSlides() {
        TouchCtrlHandle handle = fresh();
        assertEquals(0f, handle.progress(T0 + 3999), 0f);
        assertEquals(1, handle.nextWakeMs(T0 + 3999));
        long start = T0 + TouchCtrlHandle.HIDE_DELAY_MS;
        assertEquals(0f, handle.progress(start), 0f);
        assertTrue(handle.retracted());
        assertEquals(0, handle.nextWakeMs(start + 100));
        assertEquals(.5f, handle.progress(start + 100), 1e-4f);
        assertEquals(1f, handle.progress(start + TouchCtrlHandle.SLIDE_MS), 0f);
        assertEquals(-1, handle.nextWakeMs(start + TouchCtrlHandle.SLIDE_MS));
    }

    @Test
    public void activityRestoresAndRestartsTimer() {
        TouchCtrlHandle handle = fresh();
        handle.progress(T0 + 4000);
        long hidden = T0 + 4000 + TouchCtrlHandle.SLIDE_MS;
        assertEquals(1f, handle.progress(hidden), 0f);
        handle.activity(hidden);
        assertFalse(handle.retracted());
        assertEquals(0f, handle.progress(hidden + TouchCtrlHandle.SLIDE_MS), 0f);
        assertEquals(0f, handle.progress(hidden + 3999), 0f);
        handle.progress(hidden + 4000);
        assertTrue(handle.retracted());
    }

    @Test
    public void pinnedAndDisabledStayVisible() {
        TouchCtrlHandle handle = fresh();
        handle.setPinned(true, T0);
        assertEquals(0f, handle.progress(T0 + 60_000), 0f);
        assertEquals(-1, handle.nextWakeMs(T0 + 60_000));
        handle.setPinned(false, T0 + 60_000);
        assertEquals(0f, handle.progress(T0 + 63_999), 0f);
        handle.progress(T0 + 64_000);
        assertTrue(handle.retracted());

        TouchCtrlHandle off = fresh();
        off.setEnabled(false, T0);
        assertEquals(0f, off.progress(T0 + 60_000), 0f);
        assertFalse(off.retracted());
    }

    @Test
    public void picksNearestEdge() {
        assertEquals(TouchCtrlHandle.EDGE_TOP, TouchCtrlHandle.nearestEdge(1280, 178, 2560, 1600));
        assertEquals(TouchCtrlHandle.EDGE_BOTTOM, TouchCtrlHandle.nearestEdge(1280, 1500, 2560, 1600));
        assertEquals(TouchCtrlHandle.EDGE_LEFT, TouchCtrlHandle.nearestEdge(50, 800, 2560, 1600));
        assertEquals(TouchCtrlHandle.EDGE_RIGHT, TouchCtrlHandle.nearestEdge(2500, 800, 2560, 1600));
    }

    @Test
    public void retractedTabGeometryAndTouchTarget() {
        TouchCtrlHandle handle = new TouchCtrlHandle();
        // Top edge, radius 90: a third of the diameter (60 px) stays visible.
        handle.layout(1280, 178, 90, 2560, 1600, 48, 1f);
        assertEquals(-30f, handle.drawY, 1e-3f);
        assertEquals(30f, handle.labelY, 1e-3f);
        assertEquals(TouchCtrlHandle.TAB_OPACITY, handle.opacity, 0f);
        assertEquals(0f, handle.hitTop, 0f);
        assertEquals(60f, handle.hitBottom, 0f);
        assertEquals(1190f, handle.hitLeft, 0f);
        assertEquals(1370f, handle.hitRight, 0f);
        assertTrue(handle.hitTab(1280, 5));
        assertFalse(handle.hitTab(1280, 120));

        // A small tab still gets a 48 dp (here 126 px) target; full layout is unmoved.
        handle.layout(1280, 178, 30, 2560, 1600, 126, 0f);
        assertEquals(178f, handle.drawY, 0f);
        assertEquals(126f, handle.hitBottom - handle.hitTop, 0f);
        assertEquals(126f, handle.hitRight - handle.hitLeft, 0f);

        // Right edge: the tab hugs x = width.
        handle.layout(2500, 800, 90, 2560, 1600, 48, 1f);
        assertEquals(2590f, handle.drawX, 1e-3f);
        assertEquals(2500f, handle.hitLeft, 0f);
        assertEquals(2560f, handle.hitRight, 0f);
    }
}
