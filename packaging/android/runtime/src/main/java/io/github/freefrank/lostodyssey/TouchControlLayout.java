package io.github.freefrank.lostodyssey;

/** Screen-relative positions for the fifteen movable parts of the virtual pad. */
final class TouchControlLayout {
    static final int LEFT_STICK = 0, RIGHT_STICK = 1, DPAD = 2;
    static final int A = 3, B = 4, X = 5, Y = 6, L3 = 7, R3 = 8;
    static final int LB = 9, LT = 10, BACK = 11, START = 12, RT = 13, RB = 14;
    static final int COUNT = 15;
    static final float MIN_SIZE = 0.60f, MAX_SIZE = 1.40f;
    static final float MIN_OPACITY = 0.25f, MAX_OPACITY = 1.0f;
    /** Editor id of the CTRL button; it is not part of the pad arrays. */
    static final int CTRL = COUNT;
    static final float CTRL_HIT_RADIUS = .50f;
    /** Top strip kept clear because the SDL view can extend behind the status bar. */
    static final float TOP_SAFE = .06f;

    final float[] x = new float[COUNT];
    final float[] y = new float[COUNT];
    final boolean[] visible = new boolean[COUNT];
    float ctrlX = .5f, ctrlY = .111f;
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
        result.ctrlX = ctrlX;
        result.ctrlY = ctrlY;
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
        float minY = Math.min(maxY, margin + height * TOP_SAFE);
        py = Math.max(minY, Math.min(maxY, py));
        // CTRL remains reachable even if a user drags a button into it (or CTRL onto
        // a button: clampAll then moves the button): take the nearest free spot
        // beside, below or above it.
        float settingsX = ctrlX * width, settingsY = ctrlY * height;
        float clearance = margin + unit * .52f;
        float dx = px - settingsX, dy = py - settingsY;
        if (dx * dx + dy * dy < clearance * clearance) {
            float below = settingsY + clearance, above = settingsY - clearance;
            float side = settingsX + (dx < 0f ? -clearance : clearance);
            float best = Float.POSITIVE_INFINITY, bestX = px, bestY = Math.min(maxY, below);
            if (below <= maxY) { best = Math.abs(below - py); bestY = below; }
            if (side >= margin && side <= width - margin && Math.abs(side - px) < best) {
                best = Math.abs(side - px); bestX = side; bestY = py;
            }
            if (above >= minY && Math.abs(above - py) < best) { bestX = px; bestY = above; }
            px = Math.max(margin, Math.min(width - margin, bestX));
            py = bestY;
        }
        x[id] = px / width;
        y[id] = py / height;
    }

    /** CTRL stays fully on screen, below the top strip and clear of the editor toolbar. */
    void moveCtrl(float px, float py, int width, int height, float unit,
                  float reservedBottom) {
        float margin = Math.min(CTRL_HIT_RADIUS * unit, Math.min(width, height) * .45f);
        float toolbarSpace = px + margin >= width * .327f
            && px - margin <= width * .683f ? reservedBottom : 0f;
        float maxY = Math.max(margin, height - margin - toolbarSpace);
        float minY = Math.min(maxY, margin + height * TOP_SAFE);
        ctrlX = Math.max(margin, Math.min(width - margin, px)) / width;
        ctrlY = Math.max(minY, Math.min(maxY, py)) / height;
    }

    void clampAll(int width, int height, float unit, float reservedBottom) {
        moveCtrl(ctrlX * width, ctrlY * height, width, height, unit, reservedBottom);
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
