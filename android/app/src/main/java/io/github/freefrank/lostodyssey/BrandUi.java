package io.github.freefrank.lostodyssey;

import android.app.AlertDialog;
import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.StateListDrawable;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.TextView;
import java.io.InputStream;
import java.io.IOException;

/** Presentation only: does not own input, settings, game lifecycle or installers. */
final class BrandUi {
    static void help(LinearLayout parent,String setting,String explanation){
        LinearLayout row=new LinearLayout(parent.getContext());row.setOrientation(LinearLayout.HORIZONTAL);row.setGravity(android.view.Gravity.CENTER_VERTICAL);
        android.widget.TextView label=new android.widget.TextView(parent.getContext());label.setText(setting);label.setTextColor(PRIMARY);label.setTextSize(16);row.addView(label,new LinearLayout.LayoutParams(0,-2,1));
        Button help=new Button(parent.getContext());help.setText("?");help.setContentDescription("Help with "+setting);
        help.setOnClickListener(v->{android.app.AlertDialog d=new android.app.AlertDialog.Builder(parent.getContext()).setTitle(setting).setMessage(explanation).setPositiveButton("Got it",null).create();d.show();finishDialog(d);});row.addView(help,new LinearLayout.LayoutParams(dp(parent.getContext(),48),dp(parent.getContext(),48)));parent.addView(row);
    }
    static final int SURFACE=0xff10161d,PRIMARY=0xffe5e9ec,SECONDARY=0xffb4c4d2,ACCENT=0xffa6d7f2;
    private static Bitmap full,compact,background;
    static int dp(Context c,float value){return Math.round(value*c.getResources().getDisplayMetrics().density);}
    static Bitmap load(Context c,String name){
        try(InputStream in=c.getAssets().open("branding/"+name)){return BitmapFactory.decodeStream(in);}
        catch(IOException e){android.util.Log.e("LO.Brand","Missing branding asset: "+name,e);return null;}
    }
    static Bitmap compact(Context c){if(compact==null)compact=load(c,"lo_saltlord_logo_compact.png");return compact;}
    static Bitmap full(Context c){if(full==null)full=load(c,"lo_saltlord_logo_full.png");return full;}
    private static Bitmap background(Context c){if(background==null)background=load(c,"lo_saltlord_menu_background.png");return background;}
    static void fitMark(Canvas canvas,Bitmap image,RectF area,Paint paint){
        if(image==null)return;
        float[] rect=BrandLayoutPolicy.imageRect(image.getWidth(),image.getHeight(),area.left,area.top,area.width(),area.height(),false);
        canvas.drawBitmap(image,null,new RectF(rect[0],rect[1],rect[2],rect[3]),paint);
    }
    static Drawable backdrop(Context c){
        final Bitmap image=background(c);
        return new Drawable(){final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
            public void draw(Canvas canvas){
                RectF area=new RectF(getBounds());canvas.drawColor(SURFACE);
                if(image!=null){float[] rect=BrandLayoutPolicy.imageRect(image.getWidth(),image.getHeight(),area.left,area.top,area.width(),area.height(),true);
                    canvas.drawBitmap(image,null,new RectF(rect[0],rect[1],rect[2],rect[3]),paint);}
                paint.setColor(0xcc10161d);canvas.drawRect(area,paint);
            }
            public void setAlpha(int value){}public void setColorFilter(ColorFilter filter){}public int getOpacity(){return PixelFormat.OPAQUE;}
        };
    }
    private static GradientDrawable buttonSurface(Context c,int fill){GradientDrawable d=new GradientDrawable();d.setColor(fill);d.setCornerRadius(dp(c,8));d.setStroke(dp(c,1),0xff516878);return d;}
    static void style(View view){
        Context c=view.getContext();
        if(view instanceof TextView){TextView t=(TextView)view;t.setTextColor("secondary".equals(t.getTag())?SECONDARY:PRIMARY);t.setLinkTextColor(ACCENT);}
        if(view instanceof Button){Button b=(Button)view;b.setAllCaps(false);boolean danger="danger".equals(b.getTag());boolean header="section-header".equals(b.getTag());boolean nested="subsection-header".equals(b.getTag());b.setTextColor(danger?0xffffe5e5:header?ACCENT:nested?0xffd3e2eb:PRIMARY);StateListDrawable states=new StateListDrawable();
            states.addState(new int[]{android.R.attr.state_pressed},buttonSurface(c,danger?0xff9b3d45:nested?0xff303e49:0xff354e60));states.addState(new int[]{},buttonSurface(c,danger?0xff78313b:header?0xff263d4e:nested?0xff1e2a34:0xff34383f));b.setBackground(states);
            b.setMinHeight(dp(c,48));b.setPadding(dp(c,12),dp(c,8),dp(c,12),dp(c,8));
            if(b.getLayoutParams() instanceof ViewGroup.MarginLayoutParams)((ViewGroup.MarginLayoutParams)b.getLayoutParams()).setMargins(0,dp(c,4),0,dp(c,4));}
        if(view instanceof SeekBar){SeekBar s=(SeekBar)view;
            GradientDrawable track=new GradientDrawable();track.setColor(0xff526577);track.setCornerRadius(dp(c,4));
            GradientDrawable fill=new GradientDrawable();fill.setColor(ACCENT);fill.setCornerRadius(dp(c,4));
            android.graphics.drawable.ClipDrawable progress=new android.graphics.drawable.ClipDrawable(fill,android.view.Gravity.LEFT,android.graphics.drawable.ClipDrawable.HORIZONTAL);
            android.graphics.drawable.LayerDrawable layers=new android.graphics.drawable.LayerDrawable(new Drawable[]{track,progress});
            layers.setId(0,android.R.id.background);layers.setId(1,android.R.id.progress);
            for(int i=0;i<2;i++){layers.setLayerHeight(i,dp(c,8));layers.setLayerGravity(i,android.view.Gravity.CENTER_VERTICAL);}
            s.setProgressDrawable(layers);s.setProgressTintList(null);s.setProgressBackgroundTintList(null);
            GradientDrawable thumb=new GradientDrawable();thumb.setShape(GradientDrawable.OVAL);thumb.setColor(ACCENT);thumb.setStroke(dp(c,2),0xffe5f4ff);thumb.setSize(dp(c,26),dp(c,26));s.setThumb(thumb);s.setThumbTintList(null);s.setThumbOffset(dp(c,13));s.setSplitTrack(false);s.setMinimumHeight(dp(c,48));s.setPadding(dp(c,16),dp(c,8),dp(c,16),dp(c,8));}
        if(view instanceof android.widget.Spinner){
            view.setBackgroundTintList(null);
            view.setBackground(new Drawable(){
                final GradientDrawable surface=buttonSurface(c,0xff283b4b);
                final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
                public void draw(Canvas canvas){
                    surface.setBounds(getBounds());surface.draw(canvas);
                    float x=getBounds().right-dp(c,22),y=getBounds().exactCenterY();
                    android.graphics.Path arrow=new android.graphics.Path();
                    arrow.moveTo(x-dp(c,5),y-dp(c,3));arrow.lineTo(x+dp(c,5),y-dp(c,3));arrow.lineTo(x,y+dp(c,3));arrow.close();
                    paint.setColor(ACCENT);canvas.drawPath(arrow,paint);
                }
                public void setAlpha(int a){surface.setAlpha(a);}public void setColorFilter(ColorFilter f){surface.setColorFilter(f);}public int getOpacity(){return PixelFormat.TRANSLUCENT;}
            });
            view.setMinimumHeight(dp(c,48));view.setPadding(dp(c,14),dp(c,8),dp(c,44),dp(c,8));
            view.setContentDescription("Choose an individual control to resize");
        }
        if(view instanceof ViewGroup){ViewGroup g=(ViewGroup)view;for(int i=0;i<g.getChildCount();i++)style(g.getChildAt(i));}
    }
    static FrameLayout menu(Context c,View content){FrameLayout shell=new FrameLayout(c);shell.setBackground(backdrop(c));shell.addView(content);style(content);return shell;}
    static View optionsHeader(Context c){
        LinearLayout header=new LinearLayout(c);header.setOrientation(LinearLayout.HORIZONTAL);
        header.setGravity(android.view.Gravity.CENTER_VERTICAL);header.setPadding(dp(c,20),dp(c,10),dp(c,20),dp(c,10));
        ImageView logo=new ImageView(c);logo.setImageBitmap(full(c));logo.setScaleType(ImageView.ScaleType.FIT_CENTER);
        logo.setContentDescription("LO: Saltlord Edition logo");
        header.addView(logo,new LinearLayout.LayoutParams(0,dp(c,56),1));
        TextView title=new TextView(c);title.setText("Options");title.setTextSize(20);title.setTextColor(PRIMARY);
        title.setTypeface(android.graphics.Typeface.create("sans-serif-condensed",android.graphics.Typeface.BOLD));
        title.setPadding(dp(c,16),0,0,0);header.addView(title,new LinearLayout.LayoutParams(-2,-2));
        header.setBackgroundColor(SURFACE);return header;
    }
    static void finishDialog(AlertDialog dialog){
        if(dialog.getWindow()!=null)dialog.getWindow().setBackgroundDrawable(new android.graphics.drawable.ColorDrawable(SURFACE));
        for(int id:new int[]{AlertDialog.BUTTON_POSITIVE,AlertDialog.BUTTON_NEGATIVE,AlertDialog.BUTTON_NEUTRAL}){Button b=dialog.getButton(id);if(b!=null)b.setTextColor(ACCENT);}
        TextView message=dialog.findViewById(android.R.id.message);if(message!=null){message.setTextColor(PRIMARY);message.setLinkTextColor(ACCENT);}
    }
    static void textPage(Context c,String title,String body,Runnable back,Runnable next,String nextLabel,Runnable licences){
        textPage(c,title,body,back,next,nextLabel,licences,null,null);
    }
    static void textPage(Context c,String title,String body,Runnable back,Runnable next,String nextLabel,Runnable licences,String actionLabel,Runnable action){
        textPage(c,title,body,back,next,nextLabel,licences,actionLabel,action,null,null);
    }
    static void textPage(Context c,String title,String body,Runnable back,Runnable next,String nextLabel,Runnable licences,String actionLabel,Runnable action,View extras,Runnable closed){
        textPage(c,title,body,back,next,nextLabel,licences,actionLabel,action,extras,closed,null);
    }
    static void textPage(Context c,String title,String body,Runnable back,Runnable next,String nextLabel,Runnable licences,String actionLabel,Runnable action,View extras,Runnable closed,java.util.function.BooleanSupplier canAdvance){
        int height=Math.round(c.getResources().getDisplayMetrics().heightPixels*.86f);
        LinearLayout column=new LinearLayout(c);column.setOrientation(LinearLayout.VERTICAL);column.setPadding(dp(c,18),dp(c,14),dp(c,18),dp(c,8));
        ImageView logo=new ImageView(c);logo.setImageBitmap(full(c));logo.setScaleType(ImageView.ScaleType.FIT_CENTER);logo.setContentDescription("LO: Saltlord Edition logo");
        int header=Math.min(dp(c,112),Math.max(dp(c,54),Math.round(height*.23f)));column.addView(logo,new LinearLayout.LayoutParams(-1,header));
        ScrollView scroll=new ScrollView(c);scroll.setFillViewport(false);LinearLayout bodyColumn=new LinearLayout(c);bodyColumn.setOrientation(LinearLayout.VERTICAL);bodyColumn.setPadding(0,dp(c,14),0,dp(c,14));
        TextView heading=new TextView(c);heading.setText(title);heading.setTextSize(22);heading.setTypeface(android.graphics.Typeface.create("sans-serif-condensed",android.graphics.Typeface.BOLD));bodyColumn.addView(heading);
        TextView copy=new TextView(c);copy.setText(body);copy.setTextSize(16);copy.setLineSpacing(dp(c,3),1);copy.setPadding(0,dp(c,12),0,dp(c,8));bodyColumn.addView(copy);
        if(extras!=null)bodyColumn.addView(extras);
        if(action!=null){Button button=new Button(c);button.setText(actionLabel);button.setOnClickListener(v->action.run());bodyColumn.addView(button);}
        if(licences!=null){Button notices=new Button(c);notices.setText("Open-source licences & copyright notices");notices.setOnClickListener(v->licences.run());bodyColumn.addView(notices);}
        scroll.addView(bodyColumn);column.addView(scroll,new LinearLayout.LayoutParams(-1,0,1));
        FrameLayout shell=menu(c,column);shell.setMinimumHeight(height);shell.setLayoutParams(new ViewGroup.LayoutParams(-1,height));
        AlertDialog.Builder builder=new AlertDialog.Builder(c).setView(shell).setPositiveButton(nextLabel,(d,w)->{if(next!=null)next.run();});
        if(!"Back".equals(nextLabel))builder.setNegativeButton("Close",(d,w)->{if(closed!=null)closed.run();});
        if(back!=null)builder.setNeutralButton("Back",(d,w)->back.run());
        AlertDialog dialog=builder.create();if(closed!=null)dialog.setOnCancelListener(d->closed.run());dialog.show();finishDialog(dialog);
        if(canAdvance!=null)dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{if(canAdvance.getAsBoolean()){dialog.dismiss();if(next!=null)next.run();}});
        if(dialog.getWindow()!=null){int width=Math.min(dp(c,900),Math.round(c.getResources().getDisplayMetrics().widthPixels*.94f));dialog.getWindow().setLayout(width,-2);}
    }
    static String assetText(Context c,String path){try(InputStream in=c.getAssets().open(path)){java.io.ByteArrayOutputStream out=new java.io.ByteArrayOutputStream();byte[] b=new byte[8192];int n;while((n=in.read(b))!=-1)out.write(b,0,n);return out.toString("UTF-8");}catch(IOException e){return "Notice unavailable: "+path;}}
    static void showNotices(Context c){textPage(c,"Open-source notices",assetText(c,"credits/open_source_notices.txt"),null,null,"Back",null);}
    static void showCredits(Context c){textPage(c,"Credits & acknowledgements",assetText(c,"credits/project_credits.txt"),null,null,"Back",()->showNotices(c));}
}
