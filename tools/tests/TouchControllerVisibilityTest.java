package io.github.freefrank.lostodyssey;

/** Connection transitions must preserve the user's stored touch choice. */
public final class TouchControllerVisibilityTest {
    private static void check(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }

    public static void main(String[] args) {
        TouchControllerVisibility state = new TouchControllerVisibility(true);
        check(state.visible(), "touch is visible by default");
        check(state.setControllerCount(1) && !state.visible(), "first pad auto-hides touch");
        check(state.enabledPreference(), "auto-hide does not change preference");
        check(!state.setControllerCount(2) && !state.visible(), "second pad stays hidden");
        check(!state.setControllerCount(1) && !state.visible(), "one unplug stays hidden");
        state.applyShow(true);
        check(state.visible(), "touch can be re-enabled beside a pad");
        check(!state.setControllerCount(2) && state.visible(), "second pad keeps override");
        check(!state.setControllerCount(1) && state.visible(), "first unplug keeps override");
        state.applyShow(false);
        check(!state.visible() && state.enabledPreference(), "hiding beside pad is temporary");
        check(state.setControllerCount(0) && state.visible(), "last unplug restores preference");
        check(state.setControllerCount(1) && !state.visible(), "next connection auto-hides again");

        TouchControllerVisibility initiallyOff = new TouchControllerVisibility(false);
        initiallyOff.setControllerCount(1);
        check(!initiallyOff.visible(), "preference off stays off");
        initiallyOff.applyShow(true);
        check(initiallyOff.visible() && initiallyOff.enabledPreference(), "explicit Show enables touch");
        initiallyOff.setControllerCount(0);
        check(initiallyOff.visible(), "explicit Show remains enabled after unplug");
        initiallyOff.applyShow(false);
        check(!initiallyOff.enabledPreference() && !initiallyOff.visible(), "manual off persists without pad");
        System.out.println("TouchControllerVisibility: auto-hide, coexistence and restore PASS");
    }
}
