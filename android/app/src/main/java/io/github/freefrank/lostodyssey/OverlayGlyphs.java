package io.github.freefrank.lostodyssey;
import android.content.Context;
import android.graphics.*;
import java.io.InputStream;
import java.util.HashMap;
/** Main-shape bounds exclude detached export pixels for alignment only; original canvases are drawn whole. */
final class OverlayGlyphs {
 static final class Glyph { Bitmap bitmap;int l,t,r,b;float familyW,familyH,height; }
 private final HashMap<String,Glyph> glyphs=new HashMap<>();
 OverlayGlyphs(Context c){
  add(c,"A","glyph_a_crystal.png",303,14,970,1178,0.70f);
  add(c,"B","glyph_b.png",14,34,259,261,0.70f);
  add(c,"X","glyph_x.png",9,4,282,235,0.70f);
  add(c,"Y","glyph_y.png",26,16,279,248,0.70f);
  add(c,"LB","glyph_lb.png",21,21,368,190,0.78f);
  add(c,"RB","glyph_rb.png",30,30,400,191,0.78f);
  add(c,"LT","glyph_lt.png",29,29,395,191,0.78f);
  add(c,"RT","glyph_rt_ring.png",71,21,1760,849,0.78f);
  add(c,"L3","glyph_l3.png",20,33,233,249,0.68f);
  add(c,"Stick","glyph_stick.png",83,85,1142,1138,0.68f);
  add(c,"R3","glyph_r3.png",19,20,240,240,0.68f);
  add(c,"Back","glyph_select_back.png",63,48,240,241,0.56f);
  add(c,"Start","glyph_start.png",21,49,197,241,0.56f);
  add(c,"Hide","glyph_hide_eye.png",29,29,511,282,0.6f);

  add(c,"D-pad","glyph_dpad.png",127,20,1228,1088,0.90f);
  family("LB","RB");/* RT ring artwork has its own complete silhouette. */family("L3","R3");family("Back","Start");
 }
 private void add(Context c,String label,String file,int l,int t,int r,int b,float height){
  try(InputStream in=c.getAssets().open("overlay_v3/"+file)){
   Glyph g=new Glyph();g.bitmap=BitmapFactory.decodeStream(in);if(g.bitmap==null)return;
   g.l=l;g.t=t;g.r=r;g.b=b;g.height=height;g.familyW=2*Math.max((l+r)*.5f,g.bitmap.getWidth()-(l+r)*.5f)/(b-t);g.familyH=2*Math.max((t+b)*.5f,g.bitmap.getHeight()-(t+b)*.5f)/(b-t);glyphs.put(label,g);
  }catch(java.io.IOException e){android.util.Log.e("LO.Glyphs","Missing v3 glyph "+file,e);}
 }
 private void family(String a,String b){Glyph x=glyphs.get(a),y=glyphs.get(b);if(x==null||y==null)return;x.familyW=y.familyW=Math.max(x.familyW,y.familyW);x.familyH=y.familyH=Math.max(x.familyH,y.familyH);}
 boolean draw(Canvas canvas,String label,RectF area,Paint paint){
  Glyph g=glyphs.get(label);if(g==null)return false;
  float vx=area.left,vy=area.top,vw=area.width(),vh=area.height();
  if("RT".equals(label)){vx-=vw*.15f;vy-=vh*.15f;vw*=1.30f;vh*=1.30f;}
  float[] p=GlyphLayoutPolicy.rect(g.bitmap.getWidth(),g.bitmap.getHeight(),g.l,g.t,g.r,g.b,vx,vy,vw,vh,g.height,g.familyW,g.familyH);
  paint.setFilterBitmap(true);canvas.drawBitmap(g.bitmap,null,new RectF(p[0],p[1],p[2],p[3]),paint);return true;
 }
}
