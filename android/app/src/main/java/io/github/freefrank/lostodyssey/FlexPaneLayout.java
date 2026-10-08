package io.github.freefrank.lostodyssey;

import android.content.Context;
import android.view.View;
import android.widget.FrameLayout;

/** SDL renders into a real smaller surface, rather than under an opaque overlay. */
final class FlexPaneLayout extends FrameLayout {
    private final int mode;
    private final View game;
    private final FrameLayout controls;
    FlexPaneLayout(Context context,int mode,View surface,TouchGamepadView touch,ShaderPreparationView preparation) {
        super(context);this.mode=mode;
        FrameLayout viewport=new FrameLayout(context);this.game=viewport;
        viewport.addView(new AspectViewport(context,surface),new FrameLayout.LayoutParams(-1,-1));
        viewport.addView(preparation,new FrameLayout.LayoutParams(-1,-1));
        setBackgroundColor(0xff000000);
        controls=new FrameLayout(context);controls.setBackground(FlexPaneBackground.create());
        addView(viewport,new FrameLayout.LayoutParams(1,1));
        controls.addView(touch,new FrameLayout.LayoutParams(-1,-1));
        addView(controls,new FrameLayout.LayoutParams(1,1));
    }
    @Override protected void onMeasure(int widthSpec,int heightSpec) {
        int w=MeasureSpec.getSize(widthSpec),h=MeasureSpec.getSize(heightSpec);
        int[] b=FlexLayoutPolicy.bounds(w,h,mode);
        measurePane(game,b,0);measurePane(controls,b,4);
        setMeasuredDimension(w,h);
    }
    @Override protected void onLayout(boolean changed,int left,int top,int right,int bottom) {
        int[] b=FlexLayoutPolicy.bounds(right-left,bottom-top,mode);
        game.layout(b[0],b[1],b[2],b[3]);controls.layout(b[4],b[5],b[6],b[7]);
    }
    private void measurePane(View view,int[] b,int i) {
        view.measure(MeasureSpec.makeMeasureSpec(b[i+2]-b[i],MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(b[i+3]-b[i+1],MeasureSpec.EXACTLY));
    }
}
