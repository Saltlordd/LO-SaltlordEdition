package io.github.freefrank.lostodyssey;

/** Keeps automatic controller hiding separate from the user's saved preference. */
final class TouchControllerVisibility {
    private boolean enabledPreference;
    private int controllerCount;
    private boolean connectionOverride;

    TouchControllerVisibility(boolean enabledPreference) {
        this.enabledPreference = enabledPreference;
    }

    boolean visible() {
        return enabledPreference && (controllerCount == 0 || connectionOverride);
    }

    boolean enabledPreference() { return enabledPreference; }
    boolean controllerPresent() { return controllerCount > 0; }

    boolean setControllerCount(int count) {
        if (count < 0) throw new IllegalArgumentException("count");
        boolean changed = controllerPresent() != (count > 0);
        controllerCount = count;
        if (!changed) return false;
        connectionOverride = false;
        return true;
    }

    void applyShow(boolean show) {
        if (controllerPresent()) {
            // Hiding an auto-hidden pad is temporary; disconnect restores the
            // user's saved choice. Explicit Show also enables a previously off pad.
            if (show) enabledPreference = true;
            connectionOverride = show;
        } else {
            enabledPreference = show;
        }
    }
}
