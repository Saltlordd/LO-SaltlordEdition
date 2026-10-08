package io.github.freefrank.lostodyssey;

/** Exact pane bounds; coordinates remain local to each physical child view. */
final class FlexLayoutPolicy {
    static final int FULLSCREEN=0, TOP_BOTTOM=1, LEFT_RIGHT=2;
    static int valid(int mode) { return mode==TOP_BOTTOM||mode==LEFT_RIGHT?mode:FULLSCREEN; }
    static String profile(int mode) { return mode==LEFT_RIGHT?"flex-side":"flex-bottom"; }
    // Rectangles are [left, top, right, bottom], game first, controls second.
    static int[] bounds(int width,int height,int mode) {
        int w=Math.max(0,width),h=Math.max(0,height);
        // Both modes stack panes. Mode 2 uses portrait activity orientation.
        int y=h/2;return new int[]{0,0,w,y,0,y,w,h};
    }
}
