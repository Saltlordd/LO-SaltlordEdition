package io.github.freefrank.lostodyssey;

/** Screen-relative positions for the fifteen movable parts of the virtual pad. */
final class TouchControlLayout {
    static final int LEFT_STICK = 0, RIGHT_STICK = 1, DPAD = 2;
    static final int A = 3, B = 4, X = 5, Y = 6, L3 = 7, R3 = 8;
    static final int LB = 9, LT = 10, BACK = 11, START = 12, RT = 13, RB = 14;
    static final int COUNT = 15;
    static final float MIN_SIZE = 0.60f, MAX_SIZE = 1.40f;
    static final float MIN_OPACITY = 0.25f, MAX_OPACITY = 1.0f;

    final float[] x = new float[COUNT];
    final float[] y = new float[COUNT];
    final boolean[] visible = new boolean[COUNT];
    float size = 1.0f;
    float opacity = 1.0f;

    static TouchControlLayout defaults(int width, int height, float unit) {
        TouchControlLayout result = new TouchControlLayout();
        result.set(LEFT_STICK, .229f, .803f);
        result.set(RIGHT_STICK, .771f, .803f);
        result.set(DPAD, .113f, .538f);
        result.set(LT, .052f, .220f);
        result.set(LB, .174f, .220f);
        result.set(RB, .826f, .220f);
        result.set(RT, .948f, .220f);
        result.set(L3, .052f, .882f);
        result.set(R3, .948f, .882f);
        result.set(BACK, .418f, .111f);
        result.set(START, .582f, .111f);
        float dx = .855f * unit / width;
        float dy = .855f * unit / height;
        result.set(A, .887f, .538f + dy);
        result.set(B, .887f + dx, .538f);
        result.set(X, .887f - dx, .538f);
        result.set(Y, .887f, .538f - dy);
        result.clampAll(width, height, unit, 0f);
        return result;
    }

    private void set(int id, float px, float py) {
        x[id] = px;
        y[id] = py;
        visible[id] = true;
    }

    TouchControlLayout copy() {
        TouchControlLayout result = new TouchControlLayout();
        System.arraycopy(x, 0, result.x, 0, COUNT);
        System.arraycopy(y, 0, result.y, 0, COUNT);
        System.arraycopy(visible, 0, result.visible, 0, COUNT);
        result.size = size;
        result.opacity = opacity;
        return result;
    }

    float pixelX(int id, int width) { return x[id] * width; }
    float pixelY(int id, int height) { return y[id] * height; }

    /** Keep the entire touch target visible; the editor reserves its bottom toolbar. */
    void move(int id, float px, float py, int width, int height, float unit,
              float reservedBottom) {
        float radius = hitRadius(id) * unit;
        float margin = Math.min(radius, Math.min(width, height) * .45f);
        float toolbarSpace = px + margin >= width * .327f
            && px - margin <= width * .683f ? reservedBottom : 0f;
        float maxY = Math.max(margin, height - margin - toolbarSpace);
        px = Math.max(margin, Math.min(width - margin, px));
        // The SDL view can extend behind the Android status bar.
        float minY = Math.min(maxY, margin + height * .06f);
        py = Math.max(minY, Math.min(maxY, py));
        // CTRL remains reachable even if a user drags a button into its top-center area.
        float settingsX = width * .5f, settingsY = height * .111f;
        float clearance = margin + unit * .52f;
        float dx = px - settingsX, dy = py - settingsY;
        if (dx * dx + dy * dy < clearance * clearance) {
            float below = settingsY + clearance;
            float side = settingsX + (dx < 0f ? -clearance : clearance);
            if (side >= margin && side <= width - margin
                    && (below > maxY || Math.abs(side - px) < Math.abs(below - py)))
                px = side;
            else py = Math.min(maxY, below);
            px = Math.max(margin, Math.min(width - margin, px));
        }
        x[id] = px / width;
        y[id] = py / height;
    }

    void clampAll(int width, int height, float unit, float reservedBottom) {
        for (int id = 0; id < COUNT; ++id) {
            move(id, pixelX(id, width), pixelY(id, height), width, height,
                 unit, reservedBottom);
        }
    }

    static float hitRadius(int id) {
        if (id == LEFT_STICK || id == RIGHT_STICK) return 1.04f;
        if (id == DPAD) return 1.36f;
        if (id == BACK || id == START) return .49f;
        return .50f;
    }
}
