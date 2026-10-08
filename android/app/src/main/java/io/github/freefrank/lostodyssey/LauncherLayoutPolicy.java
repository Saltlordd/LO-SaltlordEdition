package io.github.freefrank.lostodyssey;
/** Complete non-scrolling menu geometry. Every child is measured to its allocated rectangle. */
final class LauncherLayoutPolicy {
 static final class Plan {final int[][] rects=new int[8][4];boolean horizontal;}
 static Plan plan(int width,int height,float density){
  Plan p=new Plan();int w=Math.max(0,width),h=Math.max(0,height);if(w==0||h==0)return p;
  float d=Float.isFinite(density)&&density>0?density:1;
  // Very small multi-window bounds reduce spacing rather than cropping a child.
  float u=Math.min(d,Math.min(w/240f,h/300f));
  int pad=Math.max(1,Math.round(12*u)),gap=Math.max(1,Math.round(8*u));
  int footerH=Math.round(44*u),versionH=Math.round(20*u),icon=Math.round(56*u);
  int versionTop=h-pad-versionH,footerBottom=versionTop-gap,footerTop=footerBottom-footerH;
  int fw=Math.min(w-2*pad,Math.round(640*u)),fx=(w-fw)/2,half=(fw-gap)/2;
  box(p,4,fx,footerTop,fx+half,footerBottom);box(p,5,fx+half+gap,footerTop,fx+fw,footerBottom);
  box(p,6,pad,versionTop,Math.min(w-pad,pad+Math.round(180*u)),h-pad);
  box(p,7,w-pad-icon,pad,w-pad,pad+icon);
  int top=pad+icon+gap,bottom=footerTop-Math.round(16*u),available=Math.max(0,bottom-top);
  int buttonH=Math.round(52*u),buttonGap=Math.round(10*u),logoGap=Math.round(16*u);
  int logoH=Math.min(Math.round(170*u),Math.round(h*.24f));
  int vertical=logoH+logoGap+3*buttonH+2*buttonGap;
  p.horizontal=(h/d<390&&w>h*1.4f)||vertical>available;
  int primaryH=p.horizontal?buttonH:3*buttonH+2*buttonGap;
  logoH=Math.max(0,Math.min(logoH,available-primaryH-logoGap));
  int coreH=logoH+logoGap+primaryH,y=top+Math.max(0,(available-coreH)/2);
  int bw=Math.min(w-2*pad,Math.round((p.horizontal?640:560)*u)),bx=(w-bw)/2;
  int lw=Math.min(bw,Math.round(420*u));box(p,0,(w-lw)/2,y,(w+lw)/2,y+logoH);
  int buttonsY=y+logoH+logoGap;
  for(int i=0;i<3;i++)if(p.horizontal){int aw=(bw-2*buttonGap)/3;int x=bx+i*(aw+buttonGap);box(p,i+1,x,buttonsY,i==2?bx+bw:x+aw,buttonsY+buttonH);}else box(p,i+1,bx,buttonsY+i*(buttonH+buttonGap),bx+bw,buttonsY+i*(buttonH+buttonGap)+buttonH);
  for(int[] b:p.rects){b[0]=Math.max(0,Math.min(w,b[0]));b[1]=Math.max(0,Math.min(h,b[1]));b[2]=Math.max(b[0],Math.min(w,b[2]));b[3]=Math.max(b[1],Math.min(h,b[3]));}
  return p;
 }
 private static void box(Plan p,int i,int l,int t,int r,int b){p.rects[i]=new int[]{l,t,r,b};}
}
