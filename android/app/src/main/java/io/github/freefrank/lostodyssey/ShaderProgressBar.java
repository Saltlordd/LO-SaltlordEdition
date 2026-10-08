package io.github.freefrank.lostodyssey;
import android.content.Context;
import android.graphics.*;
import android.view.View;
import android.widget.ProgressBar;
/** A small shimmer on the filled progress only, at 12.5 Hz. No renderer/compiler work. */
final class ShaderProgressBar extends ProgressBar {
 private final Paint light=new Paint(Paint.ANTI_ALIAS_FLAG),spark=new Paint(Paint.ANTI_ALIAS_FLAG);
 private final Matrix matrix=new Matrix();private boolean shimmer,visible,ticking;private long frame;
 private final Runnable pulse=new Runnable(){public void run(){ticking=false;if(!active())return;frame++;invalidate();schedule();}};
 ShaderProgressBar(Context c){super(c,null,android.R.attr.progressBarStyleHorizontal);spark.setColor(0xb0e5f8ff);spark.setStrokeWidth(BrandUi.dp(c,1));}
 private boolean active(){return shimmer&&visible&&!isIndeterminate()&&getProgress()>0&&isAttachedToWindow();}
 private void schedule(){if(active()&&!ticking){ticking=true;postDelayed(pulse,80);}else if(!active()){removeCallbacks(pulse);ticking=false;}}
 void setShimmer(boolean enabled){shimmer=enabled;schedule();}
 @Override public void onVisibilityAggregated(boolean shown){super.onVisibilityAggregated(shown);visible=shown;schedule();}
 @Override protected void onDetachedFromWindow(){removeCallbacks(pulse);ticking=false;super.onDetachedFromWindow();}
 @Override protected void onSizeChanged(int w,int h,int ow,int oh){super.onSizeChanged(w,h,ow,oh);if(w>0)light.setShader(new LinearGradient(0,0,Math.max(1,w*.15f),0,new int[]{0x00d6f2ff,0x75d6f2ff,0x00d6f2ff},null,Shader.TileMode.CLAMP));}
 @Override protected synchronized void onDraw(Canvas canvas){super.onDraw(canvas);if(!active()||getWidth()==0)return;
  float width=getWidth()-getPaddingLeft()-getPaddingRight(),filled=width*getProgress()/Math.max(1,getMax()),band=width*.15f;
  float x=((frame%50)/49f)*(filled+band)-band,y=getHeight()*.5f;
  int save=canvas.save();canvas.clipRect(getPaddingLeft(),getPaddingTop(),getPaddingLeft()+filled,getHeight()-getPaddingBottom());matrix.setTranslate(getPaddingLeft()+x,0);if(light.getShader()!=null)light.getShader().setLocalMatrix(matrix);canvas.drawRect(getPaddingLeft(),getPaddingTop(),getPaddingLeft()+filled,getHeight()-getPaddingBottom(),light);
  float sx=getPaddingLeft()+x+band*.5f,r=Math.min(getHeight()*.20f,BrandUi.dp(getContext(),3));canvas.drawLine(sx-r,y,sx+r,y,spark);canvas.drawLine(sx,y-r,sx,y+r,spark);canvas.restoreToCount(save);
 }
}
