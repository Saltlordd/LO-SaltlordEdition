package io.github.freefrank.lostodyssey;
/** Shared geometry for unchanged-aspect logos and cover backgrounds. Pure Java for host verification. */
final class BrandLayoutPolicy {
    static float[] imageRect(float iw,float ih,float left,float top,float width,float height,boolean cover){
        if(iw<=0||ih<=0||width<=0||height<=0)return new float[]{left,top,left,top};
        float scale=cover?Math.max(width/iw,height/ih):Math.min(width/iw,height/ih);
        float w=iw*scale,h=ih*scale,cx=left+width/2,cy=top+height/2;
        return new float[]{cx-w/2,cy-h/2,cx+w/2,cy+h/2};
    }
}
