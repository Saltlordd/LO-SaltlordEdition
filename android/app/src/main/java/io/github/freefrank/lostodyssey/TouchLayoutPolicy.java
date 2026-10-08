package io.github.freefrank.lostodyssey;
final class TouchLayoutPolicy {
    static String profileFor(int w,int h,float density) {
        return Math.min(w,h)/Math.max(.1f,density)>=600 ? "inner" : "cover";
    }
    static float gridStep(float density) {return 8*Math.max(.1f,density);}
    static float snap(float point,float radius,float extent,float density) {
        float step=gridStep(density),lo=(float)Math.ceil(radius/step),hi=(float)Math.floor((extent-radius)/step);
        return hi<lo?extent/2:Math.max(lo,Math.min(hi,Math.round(point/step)))*step;
    }
    static float gridStep(float density,int mode){return mode==3?0:(mode==0?32:mode==1?16:8)*Math.max(.1f,density);}
    static float snap(float point,float radius,float extent,float density,int mode){
        float step=gridStep(density,mode);
        if(step==0)return extent<2*radius?extent/2:Math.max(radius,Math.min(extent-radius,point));
        float lo=(float)Math.ceil(radius/step),hi=(float)Math.floor((extent-radius)/step);
        return hi<lo?extent/2:Math.max(lo,Math.min(hi,Math.round(point/step)))*step;
    }
    static float[] axes(float x,float y,float cx,float cy,float radius){
        if(!(radius>0)||!Float.isFinite(radius))return new float[]{0,0};
        float dx=(x-cx)/radius,dy=(cy-y)/radius;
        float length=(float)Math.sqrt(dx*dx+dy*dy);
        if(length>1){dx/=length;dy/=length;}
        return new float[]{dx,dy};
    }
}
