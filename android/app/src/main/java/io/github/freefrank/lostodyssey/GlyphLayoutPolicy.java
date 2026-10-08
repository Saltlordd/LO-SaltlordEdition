package io.github.freefrank.lostodyssey;
/** Fit full original canvases, centre by visible content; family constraints give equal content height. */
final class GlyphLayoutPolicy {
 static float[] rect(int w,int h,int l,int t,int right,int bottom,float x,float y,float bw,float bh,float height,float familyW,float familyH){
  float ch=bottom-t;float visible=Math.min(height*bh,Math.min(bw*.90f/familyW,bh*.90f/familyH));
  float scale=visible/ch,cx=(l+right)*.5f,cy=(t+bottom)*.5f;
  return new float[]{x+bw*.5f-cx*scale,y+bh*.5f-cy*scale,x+bw*.5f+(w-cx)*scale,y+bh*.5f+(h-cy)*scale};
 }
}
