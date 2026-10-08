package io.github.freefrank.lostodyssey;
import android.content.Context;
import android.view.View;
import android.widget.FrameLayout;
/** The real SDL surface is fitted; touch and shader overlays keep their own geometry. */
final class AspectViewport extends FrameLayout {
 private final View surface;
 private int bufferWidth=-1,bufferHeight=-1;
 AspectViewport(Context c,View surface){super(c);this.surface=surface;addView(surface,new FrameLayout.LayoutParams(1,1));}
 @Override protected void onMeasure(int ws,int hs){int w=MeasureSpec.getSize(ws),h=MeasureSpec.getSize(hs);setBackgroundColor(DisplayAppearance.surround(getContext()));int[] b=ViewportFit.bounds(w,h,DisplayAppearance.locked16(getContext()));int bw=b[2]-b[0],bh=b[3]-b[1];
 if(bw>0&&bh>0&&(bw!=bufferWidth||bh!=bufferHeight)&&surface instanceof android.view.SurfaceView){bufferWidth=bw;bufferHeight=bh;((android.view.SurfaceView)surface).getHolder().setFixedSize(bw,bh);}
 surface.measure(MeasureSpec.makeMeasureSpec(b[2]-b[0],MeasureSpec.EXACTLY),MeasureSpec.makeMeasureSpec(b[3]-b[1],MeasureSpec.EXACTLY));setMeasuredDimension(w,h);}
 @Override protected void onLayout(boolean changed,int l,int t,int r,int b){int[] fit=ViewportFit.bounds(r-l,b-t,DisplayAppearance.locked16(getContext()));surface.layout(fit[0],fit[1],fit[2],fit[3]);}
}
