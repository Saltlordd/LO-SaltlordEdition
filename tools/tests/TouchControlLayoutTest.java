package io.github.freefrank.lostodyssey;

/** Behavioral checks for saved layouts, reference placement and off-screen dragging. */
public final class TouchControlLayoutTest {
    private static void check(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }

    private static void close(float actual, float expected, String message) {
        check(Math.abs(actual - expected) < .001f, message + ": " + actual);
    }

    private static float unit(int width, int height) {
        return Math.min(width * .0715f, height * .1625f);
    }

    public static void main(String[] args) {
        TouchControlLayout saved = TouchControlLayout.defaults(2460, 1080, unit(2460, 1080));
        // The requested reference has the sticks below and inside the button clusters.
        close(saved.x[TouchControlLayout.LEFT_STICK], .229f, "reference left stick");
        close(saved.x[TouchControlLayout.RIGHT_STICK], .771f, "reference right stick");
        check(saved.x[TouchControlLayout.LT] < saved.x[TouchControlLayout.LB], "LT outside LB");
        check(saved.x[TouchControlLayout.RT] > saved.x[TouchControlLayout.RB], "RT outside RB");
        check(saved.x[TouchControlLayout.BACK] < .5f && saved.x[TouchControlLayout.START] > .5f,
              "top center menu controls");
        check(saved.x[TouchControlLayout.X] < saved.x[TouchControlLayout.A]
              && saved.x[TouchControlLayout.A] < saved.x[TouchControlLayout.B]
              && saved.y[TouchControlLayout.Y] < saved.y[TouchControlLayout.A], "face diamond");
        check(saved.y[TouchControlLayout.LEFT_STICK] > saved.y[TouchControlLayout.DPAD]
              && saved.x[TouchControlLayout.L3] < saved.x[TouchControlLayout.LEFT_STICK]
              && saved.y[TouchControlLayout.L3] > saved.y[TouchControlLayout.LEFT_STICK],
              "bottom stick and outer L3");

        TouchControlLayout draft = saved.copy();
        draft.move(TouchControlLayout.A, 1000, 600, 2560, 1600, unit(2560, 1600), 180);
        draft.visible[TouchControlLayout.LT] = false;
        draft.size = .8f;
        draft.opacity = .4f;
        check(saved.visible[TouchControlLayout.LT], "draft hiding must not mutate saved visibility");
        close(saved.size, 1f, "draft size must not mutate saved layout");
        check(saved.x[TouchControlLayout.A] != draft.x[TouchControlLayout.A], "draft drag is isolated");
        close(draft.pixelX(TouchControlLayout.A, 1280), 500f, "resize keeps relative horizontal position");
        close(draft.pixelY(TouchControlLayout.A, 800), 300f, "resize keeps relative vertical position");
        TouchControlLayout committed = draft.copy();
        draft.visible[TouchControlLayout.LT] = true;
        check(!committed.visible[TouchControlLayout.LT], "saved copy owns visibility");
        close(committed.size, .8f, "size survives copy");
        close(committed.opacity, .4f, "opacity survives copy");

        for (int[] viewport : new int[][] {{640, 360}, {2560, 1600}, {1080, 2460}}) {
            int width = viewport[0], height = viewport[1];
            for (float scale : new float[] {.6f, 1f, 1.4f}) {
                float radius = unit(width, height) * scale;
                TouchControlLayout layout = TouchControlLayout.defaults(width, height, radius);
                for (int id = 0; id < TouchControlLayout.COUNT; ++id) {
                    float margin = TouchControlLayout.hitRadius(id) * radius;
                    layout.move(id, -10000, -10000, width, height, radius, 0);
                    check(layout.pixelX(id, width) >= margin - .01f
                          && layout.pixelY(id, height) >= margin - .01f, "top/left drag bounds");
                    layout.move(id, 10000, 10000, width, height, radius, 0);
                    check(layout.pixelX(id, width) <= width - margin + .01f
                          && layout.pixelY(id, height) <= height - margin + .01f, "bottom/right drag bounds");
                    layout.move(id, width * .5f, height, width, height, radius, 80);
                    check(layout.pixelY(id, height) <= height - 80 - margin + .01f, "editor toolbar remains reachable");
                    layout.move(id, width * .327f - margin * .5f, height,
                                width, height, radius, 80);
                    check(layout.pixelY(id, height) <= height - 80 - margin + .01f,
                          "partially overlapping touch target clears toolbar");
                    layout.move(id, width * .5f, height * .111f,
                                width, height, radius, 0);
                    double separation = Math.hypot(layout.pixelX(id, width) - width * .5f,
                                                   layout.pixelY(id, height) - height * .111f);
                    check(separation >= margin + radius * .5f - .01f,
                          "settings stays accessible after dragging onto it");
                }
            }
        }
        TouchControlLayout reset = TouchControlLayout.defaults(2460, 1080, unit(2460, 1080));
        for (boolean visible : reset.visible) check(visible, "reset restores hidden controls");
        close(reset.size, 1f, "reset size");
        close(reset.opacity, 1f, "reset opacity");
        System.out.println("TouchControlLayout: reference placement, draft isolation, resize and drag bounds PASS");
    }
}
