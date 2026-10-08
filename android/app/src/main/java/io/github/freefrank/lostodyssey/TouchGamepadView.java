package io.github.freefrank.lostodyssey;

import android.content.Context;
import android.content.SharedPreferences;
import android.app.AlertDialog;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.SeekBar;
import android.widget.ScrollView;
import android.widget.Button;
import android.widget.RadioGroup;
import android.widget.RadioButton;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Handler;
import android.os.Looper;
import android.view.HapticFeedbackConstants;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.MotionEvent;
import android.view.View;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

/** Touch zones and editable button layouts. All fingers contribute to one complete native snapshot. */
final class TouchGamepadView extends View {
    static final class Control {
        final String label,description; final int mask, kind; final float defaultX,defaultY; float x,y,size=1f;
        final RectF box=new RectF(); float contextFade=1f,contextFrom=1f,contextTarget=1f;
        Control(String label,int mask,int kind,float x,float y) {
            this.label=label; this.description=controlDescription(label); this.mask=mask; this.kind=kind; this.x=x; this.y=y;this.defaultX=x;this.defaultY=y;
        }
    }
    private static String controlDescription(String label){
        switch(label){case "LB":return "Left Bumper";case "RB":return "Right Bumper";case "LT":return "Left Trigger";case "RT":return "Right Trigger";case "L":return "Left Stick";case "R":return "Right Stick";case "L3":return "Left Stick press";case "R3":return "Right Stick press";case "Hide":return "Hide controls";default:return label;}
    }
    private final ArrayList<Control> controls=new ArrayList<>();
    private final Map<Integer,Control> fingers=new HashMap<>();
    private final Map<Integer,float[]> positions=new HashMap<>();
    private final android.graphics.Bitmap optionsLogo;
    private final OverlayGlyphs glyphs;
    private final WizardChime wizardChime=new WizardChime();
    private int gameContext=0;private long contextChanged=0;
    private boolean contextEnabled=false;
    private final Runnable contextPoll=new Runnable(){public void run(){updateContext();idleHandler.postDelayed(this,100);}};
    private boolean preparing=false, wizardOpen=false;
    private boolean hideWithController=false,controllerConnected=false;
    private android.hardware.input.InputManager inputManager;
    private final android.hardware.input.InputManager.InputDeviceListener controllerListener=new android.hardware.input.InputManager.InputDeviceListener(){
        public void onInputDeviceAdded(int id){refreshControllerVisibility();}
        public void onInputDeviceRemoved(int id){refreshControllerVisibility();}
        public void onInputDeviceChanged(int id){refreshControllerVisibility();}
    };
    private boolean controllerHidden(){return hideWithController&&controllerConnected&&!editing&&!editorPreview;}
    private void refreshControllerVisibility(){
        boolean connected=false;
        for(int id:android.view.InputDevice.getDeviceIds()){android.view.InputDevice d=android.view.InputDevice.getDevice(id);if(d!=null&&!d.isVirtual()&&(d.supportsSource(android.view.InputDevice.SOURCE_GAMEPAD)||d.supportsSource(android.view.InputDevice.SOURCE_JOYSTICK))){connected=true;break;}}
        boolean was=controllerHidden();controllerConnected=connected;
        if(was!=controllerHidden()){release();cancelIdleFade();armIdle();invalidate();}
    }
    private boolean controlsLocked(){if(editorPreview)return false;return preparing||wizardOpen||!RuntimeReadinessActivity.nativeGuestStarted();}
    void setPreparing(boolean value){if(preparing!=value){release();preparing=value;invalidate();}}
    private boolean eligible(Control c){return !controlsLocked()&&(editing||(!controllerHidden()||c.kind>=6)&&TouchContextPolicy.eligible(contextEnabled?gameContext:0,c.kind,c.label));}
    private void updateContext(){
        int next=contextEnabled?RuntimeReadinessActivity.nativeTouchContext():0;
        if(next==gameContext)return;gameContext=next;contextChanged=android.os.SystemClock.uptimeMillis();
        for(Control c:controls){c.contextFrom=c.contextFade;c.contextTarget=TouchContextPolicy.eligible(contextEnabled?gameContext:0,c.kind,c.label)?1f:0f;}
        java.util.Iterator<Map.Entry<Integer,Control>> it=fingers.entrySet().iterator();
        while(it.hasNext()){Map.Entry<Integer,Control> e=it.next();if(!eligible(e.getValue())){origins.remove(e.getKey());it.remove();}}
        publish();invalidate(); // Eligibility never changes hidden/manualHidden or the idle deadline.
    }
    private void instantContext(){for(Control c:controls)c.contextFade=c.contextFrom=c.contextTarget=TouchContextPolicy.eligible(contextEnabled?gameContext:0,c.kind,c.label)?1f:0f;}
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final android.graphics.Path dpadCasing=new android.graphics.Path();
    private final SharedPreferences preferences;
    private float controlScale, opacity;
    private AlertDialog options;
    private String profile="";
    private boolean editing=false;
    private boolean haptics=true,stickHaptics=false,hidden=false,manualHidden=false,waking=false;
    private int leftMode=0,rightMode=0,autoHideSeconds=0;
    private float stickTravel=1f;
    private final Map<Integer,float[]> origins=new HashMap<>();
    private final Handler idleHandler=new Handler(Looper.getMainLooper());
    @Override protected void onAttachedToWindow(){super.onAttachedToWindow();wizardChime.prepare(getContext());inputManager=(android.hardware.input.InputManager)getContext().getSystemService(Context.INPUT_SERVICE);if(inputManager!=null)inputManager.registerInputDeviceListener(controllerListener,idleHandler);refreshControllerVisibility();/* Context filtering deferred until meaningful game states are verified. */armIdle();}
    private float idleFade=1f,drawFade=1f;
    private android.animation.ValueAnimator fadeAnimator;
    private void cancelIdleFade(){android.animation.ValueAnimator old=fadeAnimator;fadeAnimator=null;if(old!=null)old.cancel();idleFade=1f;invalidate();}
    private final Runnable hideIdle=()->{
        if(controllerHidden()||inFlexMode()||editing||options!=null||!positions.isEmpty())return;
        cancelIdleFade();manualHidden=false;
        fadeAnimator=android.animation.ValueAnimator.ofFloat(1f,0f);fadeAnimator.setDuration(350);
        fadeAnimator.addUpdateListener(a->{idleFade=(Float)a.getAnimatedValue();invalidate();});
        fadeAnimator.addListener(new android.animation.AnimatorListenerAdapter(){
            @Override public void onAnimationEnd(android.animation.Animator a){if(fadeAnimator!=a)return;fadeAnimator=null;hidden=true;idleFade=1f;release();}
        });fadeAnimator.start();
    };
    private int lastDpad=0;
    private void feedback(){if(haptics)performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);}
    private void feedbackForControl(int kind){
        if(kind==1||kind==2){if(stickHaptics)performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);}
        else feedback();
    }
    private boolean inFlexMode(){return flexMode!=0;}
    private void armIdle(){idleHandler.removeCallbacks(hideIdle);if(!controllerHidden()&&flexMode==0&&autoHideSeconds>0&&!editing&&options==null&&!hidden&&positions.isEmpty())idleHandler.postDelayed(hideIdle,autoHideSeconds*1000L);}
    @Override protected void onDetachedFromWindow(){idleHandler.removeCallbacks(hideIdle);idleHandler.removeCallbacks(contextPoll);if(inputManager!=null)inputManager.unregisterInputDeviceListener(controllerListener);wizardChime.close();cancelIdleFade();release();super.onDetachedFromWindow();}
    private int stickMode(Control c){return c.kind==1?leftMode:c.kind==2?rightMode:0;}
    private Control floatingStick(float x){boolean rtHeld=false;for(Control held:fingers.values())if(held.kind==5)rtHeld=true;
        for(Control c:controls)if(c.kind==TouchContextPolicy.floatingKind(rtHeld,x,getWidth())&&eligible(c)&&stickMode(c)>0&&!fingers.containsValue(c))return c;return null;}
    private float[] axes(int id,Control c,float[] p){
        float[] origin=origins.get(id);float cx=origin==null?c.box.centerX():origin[0],cy=origin==null?c.box.centerY():origin[1];
        float radius=c.box.width()/2*((c.kind==1||c.kind==2)?stickTravel:1f);
        return TouchLayoutPolicy.axes(p[0],p[1],cx,cy,radius);
    }
    private final RectF editorDone=new RectF(),editorGrid=new RectF();
    private int gridMode=2,gridFinger=-1;
    private String gridLabel(){return new String[]{"Grid: coarse","Grid: medium","Grid: fine","Grid: off"}[gridMode];}
    private int doneFinger=-1;
    private int editFinger=-1;
    private Control editControl;
    private final int flexMode;
    private boolean editorPreview;private Runnable editorFinished;
    TouchGamepadView(Context context){this(context,0);}
    TouchGamepadView(Context context,int flexMode) {
        super(context);this.flexMode=flexMode;
        optionsLogo=BrandUi.compact(context);glyphs=new OverlayGlyphs(context);
        preferences=context.getSharedPreferences("touch-options",Context.MODE_PRIVATE);
        DefaultTouchLayouts.seed(context,preferences);
        contextEnabled=false;preferences.edit().putBoolean("context-controls",false).apply();
        controlScale=Math.max(.60f,Math.min(1.10f,preferences.getInt("size",80)/100f));
        opacity=Math.max(.20f,Math.min(1f,preferences.getInt("opacity",70)/100f));
        controls.add(new Control("Options",0,6,.06f,.05f));
        controls.add(new Control("Hide",0,7,.15f,.05f));
        paint.setTypeface(Typeface.create("sans-serif-condensed",Typeface.BOLD));
        controls.add(new Control("L",0,1,.12f,.70f));
        controls.add(new Control("R",0,2,.70f,.70f));
        controls.add(new Control("D-pad",0,3,.29f,.70f));
        controls.add(new Control("A",0x1000,0,.87f,.82f));
        controls.add(new Control("B",0x2000,0,.94f,.66f));
        controls.add(new Control("X",0x4000,0,.80f,.66f));
        controls.add(new Control("Y",0x8000,0,.87f,.50f));
        controls.add(new Control("LB",0x100,0,.07f,.14f));
        controls.add(new Control("LT",0,4,.18f,.14f));
        controls.add(new Control("RB",0x200,0,.93f,.14f));
        controls.add(new Control("RT",0,5,.82f,.14f));
        controls.add(new Control("Back",0x20,0,.43f,.87f));
        controls.add(new Control("Start",0x10,0,.57f,.87f));
        controls.add(new Control("L3",0x40,0,.40f,.68f));
        controls.add(new Control("R3",0x80,0,.59f,.68f));
        setContentDescription("Touch Xbox gamepad: A, B, X, Y, Left Bumper, Right Bumper, Left Trigger, Right Trigger, Left Stick, Right Stick, Back, Start, Hide controls, Options. Options is shown as the LO logo.");
    }
    @Override protected void onSizeChanged(int w,int h,int ow,int oh) {
        activateProfile(w,h);
        layoutControls(w,h);
    }
    private String profileKey(String key) {return profile+"_"+key;}
    private void activateProfile(int w,int h) {
        if(w<=0||h<=0)return;
        int choice=preferences.getInt("profile-choice",0);
        String next=flexMode!=0?FlexLayoutPolicy.profile(flexMode):choice==1?"cover":choice==2?"inner":TouchLayoutPolicy.profileFor(w,h,getResources().getDisplayMetrics().density);
        if(next.equals(profile))return;
        release();editing=editorPreview;
        if(options!=null){AlertDialog old=options;options=null;old.dismiss();}
        profile=next;hidden=false;manualHidden=false;waking=false;
        if(!preferences.getBoolean(profileKey("initialized"),false)) {
            SharedPreferences.Editor saved=preferences.edit();
            // Flex has its own editable profile, seeded from the user's cover layout once.
            String seed=flexMode!=0?"cover_":"";
            if(flexMode!=0)for(Map.Entry<String,?> e:preferences.getAll().entrySet()){
                if(!e.getKey().startsWith("cover_"))continue;
                String key=profile+"_"+e.getKey().substring(6);Object v=e.getValue();
                if(v instanceof Float)saved.putFloat(key,(Float)v);
                else if(v instanceof Integer)saved.putInt(key,(Integer)v);
                else if(v instanceof Boolean)saved.putBoolean(key,(Boolean)v);
            }
            for(Control c:controls) {
                saved.putFloat(profileKey("x_"+c.label),preferences.getFloat(seed+"x_"+c.label,preferences.getFloat("x_"+c.label,c.defaultX)));
                saved.putFloat(profileKey("y_"+c.label),preferences.getFloat(seed+"y_"+c.label,preferences.getFloat("y_"+c.label,c.defaultY)));
            }
            saved.putInt(profileKey("size"),preferences.getInt(seed+"size",preferences.getInt("size",80)));
            saved.putInt(profileKey("opacity"),flexMode!=0?100:preferences.getInt(seed+"opacity",preferences.getInt("opacity",70)));
            saved.putBoolean(profileKey("initialized"),true).apply();
        }
        // Existing Flex profiles receive the new default once. Later custom opacity is retained.
        if(flexMode!=0&&!preferences.getBoolean(profileKey("opacity-custom-v44"),false))preferences.edit().putInt(profileKey("opacity"),100).putBoolean(profileKey("opacity-custom-v44"),true).apply();
        controlScale=Math.max(.60f,Math.min(1.10f,preferences.getInt(profileKey("size"),80)/100f));
        opacity=Math.max(.20f,Math.min(1f,preferences.getInt(profileKey("opacity"),70)/100f));
        if(!preferences.getBoolean(profileKey("compact-actions-v22"),false)){
            SharedPreferences.Editor moved=preferences.edit();for(Control c:controls)if(c.kind>=6){moved.putFloat(profileKey("x_"+c.label),c.defaultX);moved.putFloat(profileKey("y_"+c.label),c.defaultY);}
            moved.putBoolean(profileKey("compact-actions-v22"),true).apply();
        }
        for(Control c:controls) {
            float x=preferences.getFloat(profileKey("x_"+c.label),c.defaultX),y=preferences.getFloat(profileKey("y_"+c.label),c.defaultY);
            c.x=Float.isFinite(x)?Math.max(0,Math.min(1,x)):c.defaultX;
            c.y=Float.isFinite(y)?Math.max(0,Math.min(1,y)):c.defaultY;
        }
        if(!preferences.getBoolean(profileKey("zones-v21"),false))preferences.edit().putInt(profileKey("left-mode"),2).putInt(profileKey("right-mode"),2).putBoolean(profileKey("zones-v21"),true).apply();
        for(Control c:controls)c.size=Math.max(.4f,Math.min(2.0f,preferences.getInt(profileKey("scale_"+c.label),100)/100f));
        leftMode=Math.max(0,Math.min(2,preferences.getInt(profileKey("left-mode"),2)));
        rightMode=Math.max(0,Math.min(2,preferences.getInt(profileKey("right-mode"),2)));
        stickTravel=Math.max(.5f,Math.min(1.5f,preferences.getInt(profileKey("stick-travel"),100)/100f));
        hideWithController=preferences.getBoolean("hide-with-controller",false);
        haptics=preferences.getBoolean("haptics",true);
        stickHaptics=preferences.getBoolean("stick-haptics",false);
        autoHideSeconds=Math.max(0,Math.min(30,preferences.getInt("auto-hide",0)));
        armIdle();
        android.util.Log.i("LO.Touch","active layout profile="+profile+" width="+w+" height="+h);
    }
    private void layoutControls(int w,int h) {
        release();
        float dp=getResources().getDisplayMetrics().density;
        float right=w-12*dp,top=12*dp;
        editorDone.set(Math.max(0,right-80*dp),top,right,Math.min(h,top+48*dp));
        editorGrid.set(12*dp,top,Math.min(132*dp,Math.max(12*dp,editorDone.left-8*dp)),Math.min(h,top+48*dp));
        float unit=Math.min(h*.09f,w*.043f)*controlScale;
        for(Control c:controls) {
            float radius=unit*c.size*(c.kind>=1&&c.kind<=3?1.65f:1f);
            if(c.kind>=6)radius=Math.max(radius*.65f,18*getResources().getDisplayMetrics().density);
            boolean shoulder=c.kind==4||c.kind==5||c.label.equals("LB")||c.label.equals("RB");
            float halfW=radius*(shoulder?1.25f:1f),halfH=radius*(shoulder?.70f:1f);
            float cx=Math.max(halfW,Math.min(w-halfW,w*c.x)),cy=Math.max(halfH,Math.min(h-halfH,h*c.y));
            c.box.set(cx-halfW,cy-halfH,cx+halfW,cy+halfH);
        }
    }
    void release() {
        fingers.clear(); positions.clear();origins.clear();lastDpad=0;doneFinger=-1;gridFinger=-1;editFinger=-1;editControl=null;
        if(!editorPreview)RuntimeReadinessActivity.nativeTouch(0,0,0,0,0,0,0); invalidate();
    }
    private void publish() {
        if(editorPreview)return;
        int buttons=0,lt=0,rt=0,lx=0,ly=0,rx=0,ry=0;
        for(Map.Entry<Integer,Control> entry:fingers.entrySet()) {
            Control c=entry.getValue(); float[] p=positions.get(entry.getKey());
            if(p==null||!eligible(c))continue;
            float[] axis=axes(entry.getKey(),c,p);float x=axis[0],y=axis[1];
            if(c.kind==1){lx=Math.round(x*32767);ly=Math.round(y*32767);}
            else if(c.kind==2){rx=Math.round(x*32767);ry=Math.round(y*32767);}
            else if(c.kind==3){
                if(y>.3f)buttons|=1; if(y<-.3f)buttons|=2;
                if(x<-.3f)buttons|=4; if(x>.3f)buttons|=8;
            } else if(c.kind==4)lt=255;
            else if(c.kind==5)rt=255;
            else if(c.box.contains(p[0],p[1]))buttons|=c.mask;
        }
        // RT owns its pointer even outside its target. Its drag also steers the
        // right stick; other pointers and their ordinary controls remain owned.
        // A stationary RT leaves an independently held camera stick usable.
        for(Map.Entry<Integer,Control> entry:fingers.entrySet()) {
            if(entry.getValue().kind!=5||!eligible(entry.getValue()))continue;
            float[] p=positions.get(entry.getKey()),origin=origins.get(entry.getKey());
            if(p==null||origin==null||Math.hypot(p[0]-origin[0],p[1]-origin[1])<=3)continue;
            for(Control camera:controls)if(camera.kind==2){
                float[] drag=TouchLayoutPolicy.axes(p[0],p[1],origin[0],origin[1],camera.box.width()*.5f*stickTravel);
                rx=Math.round(drag[0]*32767);ry=Math.round(drag[1]*32767);break;
            }
        }
        int dpad=buttons&15;if(dpad!=lastDpad&&dpad!=0)feedback();lastDpad=dpad;
        RuntimeReadinessActivity.nativeTouch(buttons,lt,rt,lx,ly,rx,ry); invalidate();
    }
    @Override public boolean onTouchEvent(MotionEvent event) {
        if(controlsLocked()){release();return true;}
        if(editing)return editTouch(event);
        int action=event.getActionMasked(),index=event.getActionIndex();
        idleHandler.removeCallbacks(hideIdle);
        if(action==MotionEvent.ACTION_CANCEL){release();waking=false;armIdle();return true;}
        if(action==MotionEvent.ACTION_DOWN||action==MotionEvent.ACTION_POINTER_DOWN) {
            cancelIdleFade();
            float x=event.getX(index),y=event.getY(index);int id=event.getPointerId(index);
            Control selected=null;
            // Real buttons always take priority over the screen-wide stick spaces.
            for(Control c:controls)if(c.kind>=6&&c.box.contains(x,y)){selected=c;break;}
            if(selected==null&&controllerHidden())return true;
            if(selected==null&&hidden){
                if(manualHidden)return true;
                hidden=false;instantContext();release();invalidate();
            }
            if(selected==null)for(Control c:controls)if(c.kind<6&&eligible(c)&&!(c.kind<=2&&c.kind>=1&&stickMode(c)>0)&&c.box.contains(x,y)){selected=c;break;}
            if(selected==null)selected=floatingStick(x);
            if(selected!=null&&!fingers.containsValue(selected)){
                fingers.put(id,selected);
                if(selected.kind==5||((selected.kind==1||selected.kind==2)&&stickMode(selected)>0))origins.put(id,new float[]{x,y});
                if(selected.kind!=3)feedbackForControl(selected.kind);
            }
        }
        for(int i=0;i<event.getPointerCount();i++)positions.put(event.getPointerId(i),new float[]{event.getX(i),event.getY(i)});
        if(action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_POINTER_UP){
            int id=event.getPointerId(index);Control c=fingers.remove(id);positions.remove(id);origins.remove(id);
            if(c!=null&&c.kind>=6&&c.box.contains(event.getX(index),event.getY(index))){
                release();
                if(c.kind==6){showOptions();return true;}
                hidden=!hidden;manualHidden=hidden;invalidate();armIdle();return true;
            }
        }
        publish();armIdle();return true;
    }
    private float gridStep() {return TouchLayoutPolicy.gridStep(getResources().getDisplayMetrics().density,gridMode);}
    private float snapped(float point,float radius,float extent) {
        return TouchLayoutPolicy.snap(point,radius,extent,getResources().getDisplayMetrics().density,gridMode);
    }
    private boolean editTouch(MotionEvent event) {
        int action=event.getActionMasked(),index=event.getActionIndex();
        if(action==MotionEvent.ACTION_CANCEL){release();return true;}
        if(action==MotionEvent.ACTION_DOWN) {
            if(editorGrid.contains(event.getX(index),event.getY(index))){gridFinger=event.getPointerId(index);return true;}
            if(editorDone.contains(event.getX(index),event.getY(index))){doneFinger=event.getPointerId(index);return true;}
            for(Control c:controls)if(c.box.contains(event.getX(index),event.getY(index))) {
                editControl=c;editFinger=event.getPointerId(index);break;
            }
        }
        if((action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_POINTER_UP)&&event.getPointerId(index)==doneFinger){
            if(editorDone.contains(event.getX(index),event.getY(index))){editing=false;if(editorFinished!=null)editorFinished.run();}
            release();invalidate();armIdle();return true;
        }
        if((action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_POINTER_UP)&&event.getPointerId(index)==gridFinger){
            if(editorGrid.contains(event.getX(index),event.getY(index))){gridMode=(gridMode+1)%4;preferences.edit().putInt("editor-grid-mode",gridMode).apply();}
            release();invalidate();return true;
        }
        int at=event.findPointerIndex(editFinger);
        if(editControl!=null && at>=0 && (action==MotionEvent.ACTION_MOVE||action==MotionEvent.ACTION_UP)) {
            float x=snapped(event.getX(at),editControl.box.width()/2,getWidth());
            float y=snapped(event.getY(at),editControl.box.height()/2,getHeight());
            editControl.x=x/getWidth();editControl.y=y/getHeight();
            editControl.box.offsetTo(x-editControl.box.width()/2,y-editControl.box.height()/2);invalidate();
        }
        if((action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_POINTER_UP)&&event.getPointerId(index)==editFinger) {
            if(editControl!=null) {
                preferences.edit().putFloat(profileKey("x_"+editControl.label),editControl.x).putFloat(profileKey("y_"+editControl.label),editControl.y).apply();
            }
            release();invalidate();armIdle();
        }
        return true;
    }
    void showLauncherOptions(){showOptions();}
    private void showOptions() {
        boolean launcher=((RuntimeReadinessActivity)getContext()).launcherOpen();
        if((controlsLocked()&&!launcher)||options!=null)return;
        idleHandler.removeCallbacks(hideIdle);cancelIdleFade();release();RuntimeReadinessActivity.nativeAppMenu(true);
        LinearLayout layout=new LinearLayout(getContext());layout.setOrientation(LinearLayout.VERTICAL);
        int pad=(int)(20*getResources().getDisplayMetrics().density);layout.setPadding(pad,pad,pad,pad);
        TextView note=new TextView(getContext());note.setTag("secondary");note.setText("Your chosen frame-rate target is saved between launches. Steady 30 prioritises consistent motion; Dynamic 60 targets up to 60 FPS and may drop below it. Return to 30 if motion slows or becomes uneven. "+DeviceUi.layoutNote(getContext())+" "+(launcher?"Changes apply when you start the game.":"Gameplay and audio pause while Options is open; closing it resumes the previous game state."));layout.addView(note);
        LinearLayout touchSection=section(layout,"Overlay options",false);
        LinearLayout layoutTools=section(touchSection,"Layout & backups",false);
        LinearLayout appearance=section(touchSection,"Colour, size & opacity",false);
        LinearLayout sticks=section(touchSection,"Sticks",false);
        LinearLayout visibility=section(touchSection,"Feedback & visibility",false);
        LinearLayout graphicsSection=section(layout,"Graphics",false);
        LinearLayout displaySection=graphicsSection;
        Button surround=new Button(getContext());String[] borderNames={"Navy","Charcoal","Black"};
        Runnable refreshBorder=()->surround.setText("Viewport border: "+borderNames[DisplayAppearance.border(getContext())]);
        surround.setOnClickListener(v->{DisplayAppearance.setBorder(getContext(),(DisplayAppearance.border(getContext())+1)%3);refreshBorder.run();});refreshBorder.run();displaySection.addView(surround);
        Button aspect=new Button(getContext());Runnable refreshAspect=()->aspect.setText("Aspect ratio: "+(DisplayAppearance.locked16(getContext())?"Locked 16:9":"Adaptive"));
        aspect.setOnClickListener(v->{DisplayAppearance.setLocked16(getContext(),!DisplayAppearance.locked16(getContext()));refreshAspect.run();});refreshAspect.run();displaySection.addView(aspect);
        BrandUi.help(displaySection,"Aspect ratio","Lock 16:9 to keep a traditional widescreen game viewport on every display, including the upper Flex pane. The image is fitted without stretching or cropping. Adaptive uses the existing screen layout. Movies retain their original proportions.");
        BrandUi.help(displaySection,"Viewport border","Choose the colour of empty space around the game viewport, including 4:3 content. This does not change the game image or black bars contained inside a movie.");
        Button monochrome=new Button(getContext());Runnable refreshMono=()->monochrome.setText("Touch artwork: "+(preferences.getBoolean("monochrome",false)?"Monochrome":"Colour"));
        monochrome.setOnClickListener(v->{preferences.edit().putBoolean("monochrome",!preferences.getBoolean("monochrome",false)).apply();refreshMono.run();invalidate();});refreshMono.run();appearance.addView(monochrome);
        BrandUi.help(appearance,"Touch artwork","Monochrome removes colour from the touch controls while keeping their detail, transparency and your saved opacity.");
        Button controllers=new MenuSparkleButton(getContext(),3);controllers.setTag("section-header");controllers.setText("Controllers & profiles");controllers.setEnabled(!launcher);controllers.setOnClickListener(v->ControllerProfiles.show((RuntimeReadinessActivity)getContext()));controllers.setCompoundDrawablesWithIntrinsicBounds(new MenuSectionIcon(getContext(),"Controllers & profiles"),null,null,null);controllers.setCompoundDrawablePadding(BrandUi.dp(getContext(),8));layout.addView(controllers,layout.indexOfChild(touchSection)+1);
        if(launcher){TextView sessionNote=new TextView(getContext());sessionNote.setTag("secondary");sessionNote.setText("Controller remapping, debug tools and test reports become available after Start game is selected in the main menu.");layout.addView(sessionNote,layout.indexOfChild(controllers)+1);}

        Button flex=new MenuSparkleButton(getContext(),4);flex.setTag("section-header");flex.setText(flexMode==0?"Flex mode (experimental)":"Flex mode / Fullscreen");
        flex.setCompoundDrawablesWithIntrinsicBounds(new FoldPhoneIcon(getContext()),null,null,null);flex.setCompoundDrawablePadding(BrandUi.dp(getContext(),8));
        boolean flexAvailable=DeviceUi.flexAvailable(getContext(),flexMode);flex.setEnabled(flexAvailable);flex.setAlpha(flexAvailable?1f:0.45f);
        flex.setOnClickListener(v->((RuntimeReadinessActivity)getContext()).chooseFlexMode());layout.addView(flex);
        if(!flexAvailable){TextView unavailable=new TextView(getContext());unavailable.setTag("secondary");unavailable.setText(DeviceUi.flexUnavailable(getContext()));layout.addView(unavailable);}
        Button quickResolution=new Button(getContext());
        Button quickFrameRate=new Button(getContext());
        Runnable refreshGraphics=()->{
            int h=RuntimeReadinessActivity.nativeRenderResolution(0),fps=RuntimeReadinessActivity.nativeFrameRate(0);
            quickResolution.setText("Render resolution: "+(h==0?"Follow output":h+"p")+" — tap to switch");
            quickFrameRate.setText("Frame-rate target: "+(fps==60?"Dynamic 60 (experimental)":fps==30?"Steady 30":fps+" FPS (advanced)")+" — tap to change");
        };
        quickResolution.setOnClickListener(v->{int[] now=GraphicsChoices.current();GraphicsChoices.choose(getContext(),now[0]==720?1080:720,now[1],now[2],false,refreshGraphics);});
        quickFrameRate.setOnClickListener(v->{int[] now=GraphicsChoices.current();GraphicsChoices.choose(getContext(),now[0],now[1]==30?60:30,now[2],false,refreshGraphics);});
        refreshGraphics.run();graphicsSection.addView(quickResolution);graphicsSection.addView(quickFrameRate);
        BrandUi.help(graphicsSection,"Frame-rate target",GraphicsChoices.HELP);
        String[] quickAaNames={"Off","FXAA","SMAA"};Button quickAa=new Button(getContext());
        Runnable refreshAa=()->{int a=RuntimeReadinessActivity.nativeAntiAliasing(-1);quickAa.setText("Anti-aliasing: "+(a>=0&&a<3?quickAaNames[a]:"Advanced (in-game setting)")+" — tap to change");};
        quickAa.setOnClickListener(v->GraphicsChoices.selectAa(getContext(),refreshAa));refreshAa.run();graphicsSection.addView(quickAa);
        GraphicsChoices.presets(getContext(),graphicsSection,()->{refreshGraphics.run();refreshAa.run();});
        BrandUi.help(graphicsSection,"Graphics guidance",GraphicsChoices.HELP);
        BrandUi.help(graphicsSection,"Anti-aliasing",GraphicsChoices.HELP);
        BrandUi.help(graphicsSection,"Render resolution","720p at 30 FPS with anti-aliasing Off is the recommended starting point. 1080p draws more pixels and may slow down demanding scenes. Switch while playing to suit your phone. This changes the internal scene resolution immediately. Android presents it to the fitted game surface at its actual size; the desktop-style output resolution shown in the in-game menu does not describe that Android surface. 1080p still adds rendering work even when the image is scaled for display.");
        LinearLayout importsSection=section(layout,"Files & GPU drivers",false);
        LinearLayout contentSection=section(importsSection,"Discs & DLC",false);
        LinearLayout driverSection=section(importsSection,"GPU drivers",false);
        TextView contentNote=new TextView(getContext());contentNote.setTag("secondary");contentNote.setText("Add discs and DLC from different folders, review your list, then install them together. Restart to use newly added content. Your saves are kept.");contentSection.addView(contentNote);
        Button importContent=new Button(getContext());importContent.setText("Manage discs & DLC");importContent.setOnClickListener(v->{options.dismiss();((RuntimeReadinessActivity)getContext()).pickContent();});contentSection.addView(importContent);
        Button discardContent=new Button(getContext());discardContent.setText("Remove files waiting for restart");discardContent.setOnClickListener(v->{options.dismiss();((RuntimeReadinessActivity)getContext()).discardPendingContent();});String pending=((RuntimeReadinessActivity)getContext()).pendingContentStatus();TextView pendingNote=new TextView(getContext());pendingNote.setText(pending.isEmpty()?"Nothing waiting for restart":"Files waiting for restart:\n"+pending);contentSection.addView(pendingNote);if(!pending.isEmpty())contentSection.addView(discardContent);
        LinearLayout savesSection=section(layout,"Saves & backups",false);savesSection.addView(((RuntimeReadinessActivity)getContext()).saveToolsPanel(false));
        LinearLayout advancedSection=section(layout,"Debug, cheats & Fast Forward",false);
        LinearLayout monitorSection=section(advancedSection,"Performance monitor",false);PerformanceMonitor.addOptions((RuntimeReadinessActivity)getContext(),monitorSection);
        LinearLayout speedSection=section(advancedSection,"Fast Forward",false);
        final Button enableSpeed=new Button(getContext()),modeSpeed=new Button(getContext()),rateSpeed=new Button(getContext());
        Runnable refreshSpeed=()->{int[] state=RuntimeReadinessActivity.nativeFastForward(0,0);enableSpeed.setText("Fast Forward: "+(state[0]!=0?"on":"off"));modeSpeed.setText("Activation mode: "+(state[1]==0?"Hold LT":"Toggle with LT"));rateSpeed.setText("Speed: "+state[2]+"×");};
        enableSpeed.setOnClickListener(v->{int[] state=RuntimeReadinessActivity.nativeFastForward(0,0);RuntimeReadinessActivity.nativeFastForward(1,state[0]==0?1:0);refreshSpeed.run();});
        modeSpeed.setOnClickListener(v->{int[] state=RuntimeReadinessActivity.nativeFastForward(0,0);RuntimeReadinessActivity.nativeFastForward(2,state[1]==0?1:0);refreshSpeed.run();});
        rateSpeed.setOnClickListener(v->{int[] state=RuntimeReadinessActivity.nativeFastForward(0,0);int[] rates={2,3,4,6,8};int next=2;for(int i=0;i<rates.length;i++)if(rates[i]==state[2])next=rates[(i+1)%rates.length];RuntimeReadinessActivity.nativeFastForward(3,next);refreshSpeed.run();});
        speedSection.addView(enableSpeed);speedSection.addView(modeSpeed);speedSection.addView(rateSpeed);refreshSpeed.run();
        TextView speedHelp=new TextView(getContext());speedHelp.setTag("secondary");speedHelp.setText("Uses the existing guest-clock Fast Forward. Close Options and release LT before activating. LT + RT does not boost. Audio is not time-stretched. These are the same settings used by the debug menu.");BrandUi.help(speedSection,"Fast Forward",speedHelp.getText().toString());
        LinearLayout debugSection=section(advancedSection,"Debug & cheats",false);
        TextView warning=new TextView(getContext());warning.setText("Experimental features\n\nOptions in this section are intended for testing and have not been validated across a complete playthrough. They may cause unexpected behaviour, progression issues or other bugs. Use with caution.");debugSection.addView(warning);
        Button debug=new Button(getContext());debug.setText("Open debug menu (LB + RB)");
        debug.setEnabled(!launcher);debug.setOnClickListener(v->{options.dismiss();release();RuntimeReadinessActivity.nativeDebugMenu();});debugSection.addView(debug);
        TextView debugHelp=new TextView(getContext());debugHelp.setTag("secondary");debugHelp.setText("Use the D-pad or sticks to navigate, A to select and B to go back. LB/RB change tabs; LT/RT change categories. Memory-editing cheats can change your save; back up saves first.");debugSection.addView(debugHelp);
        LinearLayout aboutSection=section(layout,"Setup, features & reports",false);
        Button wizard=new Button(getContext());wizard.setText("Preview startup wizard");
        wizard.setEnabled(!launcher);wizard.setOnClickListener(v->showWizard(0));aboutSection.addView(wizard);
        Button features=new Button(getContext());features.setText("About LO: Saltlord Edition");
        features.setOnClickListener(v->BrandUi.showCredits(getContext()));aboutSection.addView(features);
        Button profileButton=new Button(getContext());
        String mode=preferences.getInt("profile-choice",0)==0?"automatic":"manual";
        profileButton.setText(DeviceUi.foldable(getContext())?"Layout: "+profile+" ("+mode+") — tap to change":"Touch layout");
        profileButton.setOnClickListener(v->{preferences.edit().putInt("profile-choice",(preferences.getInt("profile-choice",0)+1)%3).apply();
            AlertDialog old=options;options=null;old.dismiss();activateProfile(getWidth(),getHeight());layoutControls(getWidth(),getHeight());showOptions();});if(flexMode==0&&DeviceUi.foldable(getContext()))layoutTools.addView(profileButton);
        Button controllerHide=new Button(getContext());
        Runnable refreshControllerHide=()->controllerHide.setText("Hide overlay when a controller is connected: "+(hideWithController?"on":"off"));
        controllerHide.setOnClickListener(v->{release();hideWithController=!hideWithController;preferences.edit().putBoolean("hide-with-controller",hideWithController).apply();refreshControllerVisibility();cancelIdleFade();armIdle();invalidate();refreshControllerHide.run();});refreshControllerHide.run();visibility.addView(controllerHide);
        BrandUi.help(visibility,"Controller overlay visibility","Hide gameplay touch controls while any physical gamepad is connected. Options and Hide/Show remain available. Your normal manual and auto-hide settings resume after all controllers disconnect. Layout editors always show controls. This preference also applies in Flex mode.");
        Button buzz=new Button(getContext());buzz.setText("Button haptics: "+(haptics?"on":"off"));
        buzz.setOnClickListener(v->{haptics=!haptics;preferences.edit().putBoolean("haptics",haptics).apply();buzz.setText("Button haptics: "+(haptics?"on":"off"));feedback();});visibility.addView(buzz);
        Button stickBuzz=new Button(getContext());stickBuzz.setText("Joystick haptics: "+(stickHaptics?"on":"off"));
        stickBuzz.setOnClickListener(v->{stickHaptics=!stickHaptics;preferences.edit().putBoolean("stick-haptics",stickHaptics).apply();stickBuzz.setText("Joystick haptics: "+(stickHaptics?"on":"off"));feedbackForControl(1);});visibility.addView(stickBuzz);
        String[] modes={"fixed visible","floating visible","floating invisible"};
        Button left=new Button(getContext());left.setText("Left stick: "+modes[leftMode]);
        left.setOnClickListener(v->{release();leftMode=(leftMode+1)%3;preferences.edit().putInt(profileKey("left-mode"),leftMode).apply();left.setText("Left stick: "+modes[leftMode]);invalidate();});sticks.addView(left);
        Button right=new Button(getContext());right.setText("Right stick: "+modes[rightMode]);
        right.setOnClickListener(v->{release();rightMode=(rightMode+1)%3;preferences.edit().putInt(profileKey("right-mode"),rightMode).apply();right.setText("Right stick: "+modes[rightMode]);invalidate();});sticks.addView(right);
        TextView travel=new TextView(getContext());travel.setText("Stick travel: "+Math.round(stickTravel*100)+"%");sticks.addView(travel);
        SeekBar travelBar=new SeekBar(getContext());travelBar.setMax(100);travelBar.setProgress(Math.round(stickTravel*100)-50);sticks.addView(travelBar);
        travelBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){
            public void onProgressChanged(SeekBar b,int value,boolean user){if(user){release();stickTravel=(value+50)/100f;travel.setText("Stick travel: "+(value+50)+"%");preferences.edit().putInt(profileKey("stick-travel"),value+50).apply();}}
            public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){}
        });
        TextView idle=new TextView(getContext());idle.setText(flexMode!=0?"Auto-hide is off in Flex mode. Hide Overlay still works manually.":"Auto-hide: "+(autoHideSeconds==0?"off":autoHideSeconds+" seconds"));visibility.addView(idle);
        SeekBar idleBar=new SeekBar(getContext());idleBar.setMax(30);idleBar.setProgress(autoHideSeconds);idleBar.setEnabled(flexMode==0);if(flexMode==0)visibility.addView(idleBar);
        idleBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){
            public void onProgressChanged(SeekBar b,int value,boolean user){if(user){autoHideSeconds=value;preferences.edit().putInt("auto-hide",value).apply();idle.setText("Auto-hide: "+(value==0?"off":value+" seconds"));}}
            public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){}
        });
        TextView help=new TextView(getContext());help.setTag("secondary");help.setText("Floating sticks appear where you place your thumb instead of staying in a fixed position.");sticks.addView(help);BrandUi.help(sticks,"Floating sticks","Buttons take priority over the stick zones. Hold RT and drag the same finger to zoom and steer the camera. Lower stick travel needs less dragging. After Auto-Hide, your first touch both reveals and activates the controls.");
        Button saveReport=new Button(getContext());saveReport.setText("Save diagnostic report");saveReport.setOnClickListener(v->((RuntimeReadinessActivity)getContext()).saveDiagnosticReport());aboutSection.addView(saveReport);BrandUi.help(aboutSection,"Reporting a bug",RuntimeReadinessActivity.REPORT_HELP);
        Button report=new Button(getContext());report.setText("Finish test & save diagnostic report");
        report.setEnabled(!launcher);report.setOnClickListener(v->{if(RuntimeReadinessActivity.nativeFinishTest()){options.dismiss();android.widget.Toast.makeText(getContext(),"Finishing test — save the report from the results screen.",android.widget.Toast.LENGTH_LONG).show();}else android.widget.Toast.makeText(getContext(),"The game has not started yet.",android.widget.Toast.LENGTH_SHORT).show();});aboutSection.addView(report);
        Button editor=new Button(getContext());editor.setText("Edit touch layout");
        editor.setOnClickListener(v->{options.dismiss();if(launcher){((RuntimeReadinessActivity)getContext()).editControls(flexMode);return;}beginEditing(null);});layoutTools.addView(editor);
        Button exportLayouts=new Button(getContext());exportLayouts.setText("Backup Layouts");exportLayouts.setOnClickListener(v->{options.dismiss();((RuntimeReadinessActivity)getContext()).exportLayouts();});layoutTools.addView(exportLayouts);
        Button restoreLayouts=new Button(getContext());restoreLayouts.setText("Restore Layouts");restoreLayouts.setOnClickListener(v->{options.dismiss();((RuntimeReadinessActivity)getContext()).restoreLayouts();});layoutTools.addView(restoreLayouts);
        Button reset=new Button(getContext());reset.setText("Reset control positions");
        reset.setOnClickListener(v->{SharedPreferences.Editor saved=preferences.edit();
            for(Control c:controls){c.x=c.defaultX;c.y=c.defaultY;saved.putFloat(profileKey("x_"+c.label),c.defaultX).putFloat(profileKey("y_"+c.label),c.defaultY);}
            saved.apply();layoutControls(getWidth(),getHeight());});layoutTools.addView(reset);
        Button importDriver=new Button(getContext());importDriver.setText(launcher?"Import GPU driver ZIP — before Start game":"Import GPU driver ZIP — next launch");
        importDriver.setOnClickListener(v->{options.dismiss();((RuntimeReadinessActivity)getContext()).pickGpuDriver();});driverSection.addView(importDriver);
        Button systemDriver=new Button(getContext());systemDriver.setText("Use System Driver");
        TextView selectedDriver=new TextView(getContext());selectedDriver.setText("Selected for next launch: "+GpuDriverStore.selectedName(getContext()));driverSection.addView(selectedDriver);
        systemDriver.setOnClickListener(v->{GpuDriverStore.select(getContext(),GpuDriverStore.SYSTEM_DRIVER);selectedDriver.setText("Selected for next launch: System GPU driver");android.widget.Toast.makeText(getContext(),"System driver selected for the next game launch.",android.widget.Toast.LENGTH_SHORT).show();});driverSection.addView(systemDriver);BrandUi.help(driverSection,"System driver","Uses the GPU driver supplied by your device instead of an imported custom driver. The change takes effect after restarting.");
        TextView individual=new TextView(getContext());individual.setText("Individual control size");appearance.addView(individual);
        android.widget.Spinner controlChoice=new android.widget.Spinner(getContext());
        final ArrayList<Control> sizeControls=new ArrayList<>();
        for(String name:new String[]{"A","B","X","Y","D-pad","L","R","LB","RB","LT","RT","L3","R3","Back","Start","Options","Hide"})for(Control c:controls)if(c.label.equals(name))sizeControls.add(c);
        ArrayList<String> labels=new ArrayList<>();for(Control c:sizeControls)labels.add(c.label);
        android.widget.ArrayAdapter<String> adapter=new android.widget.ArrayAdapter<>(getContext(),android.R.layout.simple_spinner_item,labels);adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);controlChoice.setAdapter(adapter);appearance.addView(controlChoice);
        TextView controlPercent=new TextView(getContext());appearance.addView(controlPercent);
        SeekBar controlBar=new SeekBar(getContext());controlBar.setMax(160);appearance.addView(controlBar);
        controlChoice.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener(){
            public void onItemSelected(android.widget.AdapterView<?> parent,View v,int at,long id){Control c=sizeControls.get(at);controlBar.setProgress(Math.round(c.size*100)-40);controlPercent.setText(c.label+": "+Math.round(c.size*100)+"%");}
            public void onNothingSelected(android.widget.AdapterView<?> parent){}
        });
        controlBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){
            public void onProgressChanged(SeekBar b,int value,boolean user){if(user){Control c=sizeControls.get(controlChoice.getSelectedItemPosition());c.size=(value+40)/100f;preferences.edit().putInt(profileKey("scale_"+c.label),value+40).apply();controlPercent.setText(c.label+": "+(value+40)+"%");layoutControls(getWidth(),getHeight());}}
            public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){}
        });controlChoice.setSelection(0);
        TextView size=new TextView(getContext());appearance.addView(size);
        SeekBar sizeBar=new SeekBar(getContext());sizeBar.setMax(50);sizeBar.setProgress(Math.round(controlScale*100)-60);appearance.addView(sizeBar);
        size.setText("Overall control size: "+Math.round(controlScale*100)+"%");
        sizeBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){
            public void onProgressChanged(SeekBar b,int value,boolean user){if(!user)return;controlScale=(value+60)/100f;
                size.setText("Overall control size: "+(value+60)+"%");layoutControls(getWidth(),getHeight());preferences.edit().putInt(profileKey("size"),value+60).apply();}
            public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){}
        });
        TextView alpha=new TextView(getContext());appearance.addView(alpha);
        SeekBar alphaBar=new SeekBar(getContext());alphaBar.setMax(80);alphaBar.setProgress(Math.round(opacity*100)-20);appearance.addView(alphaBar);
        alpha.setText("Control opacity: "+Math.round(opacity*100)+"%");
        alphaBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){
            public void onProgressChanged(SeekBar b,int value,boolean user){if(!user)return;opacity=(value+20)/100f;
                alpha.setText("Control opacity: "+(value+20)+"%");invalidate();preferences.edit().putInt(profileKey("opacity"),value+20).apply();}
            public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){}
        });
        Button restart=new Button(getContext());restart.setText("Restart LO: Saltlord Edition");restart.setTag("danger");restart.setEnabled(!launcher);restart.setOnClickListener(v->((RuntimeReadinessActivity)getContext()).restartGame());layout.addView(restart);
        ScrollView scroll=new ScrollView(getContext());scroll.addView(layout);
        options=new AlertDialog.Builder(getContext()).setCustomTitle(BrandUi.optionsHeader(getContext())).setView(BrandUi.menu(getContext(),scroll)).setPositiveButton("Return",(d,w)->{}).create();
        options.setOnDismissListener(d->{if(options==d)options=null;RuntimeReadinessActivity.nativeAppMenu(false);release();armIdle();});options.show();BrandUi.finishDialog(options);
    }
    void beginEditing(Runnable done){gridMode=Math.max(0,Math.min(3,preferences.getInt("editor-grid-mode",2)));editing=true;editorFinished=done;hidden=false;manualHidden=false;idleHandler.removeCallbacks(hideIdle);release();invalidate();}
    void preparePreview(Runnable done){editorPreview=true;beginEditing(done);}
    void showEditorAppearance(){
        LinearLayout body=new LinearLayout(getContext());body.setOrientation(LinearLayout.VERTICAL);body.setPadding(BrandUi.dp(getContext(),16),0,BrandUi.dp(getContext(),16),0);
        TextView title=new TextView(getContext());title.setText(flexMode==0?"This screen's overlay only":"This Flex layout only");body.addView(title);
        addEditorSlider(body,"Overall size",Math.round(controlScale*100),60,110,value->{controlScale=value/100f;preferences.edit().putInt(profileKey("size"),value).apply();layoutControls(getWidth(),getHeight());});
        addEditorSlider(body,"Opacity",Math.round(opacity*100),20,100,value->{opacity=value/100f;preferences.edit().putInt(profileKey("opacity"),value).apply();invalidate();});
        android.widget.Spinner choice=new android.widget.Spinner(getContext());ArrayList<String> names=new ArrayList<>();for(Control c:controls)names.add(c.description);android.widget.ArrayAdapter<String> adapter=new android.widget.ArrayAdapter<>(getContext(),android.R.layout.simple_spinner_item,names);adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);choice.setAdapter(adapter);body.addView(choice);
        TextView percent=new TextView(getContext());body.addView(percent);SeekBar scale=new SeekBar(getContext());scale.setMax(160);body.addView(scale);
        choice.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener(){public void onItemSelected(android.widget.AdapterView<?> p,View v,int i,long id){Control c=controls.get(i);scale.setProgress(Math.round(c.size*100)-40);percent.setText(c.description+": "+Math.round(c.size*100)+"%");}public void onNothingSelected(android.widget.AdapterView<?> p){}});
        scale.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onProgressChanged(SeekBar b,int n,boolean user){if(!user)return;Control c=controls.get(choice.getSelectedItemPosition());c.size=(n+40)/100f;preferences.edit().putInt(profileKey("scale_"+c.label),n+40).apply();percent.setText(c.description+": "+(n+40)+"%");layoutControls(getWidth(),getHeight());}public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){}});
        ScrollView scroll=new ScrollView(getContext());scroll.addView(body);AlertDialog d=new AlertDialog.Builder(getContext()).setTitle("Control size & opacity").setView(BrandUi.menu(getContext(),scroll)).setPositiveButton("Back",null).create();d.show();BrandUi.finishDialog(d);
    }
    private void addEditorSlider(LinearLayout body,String name,int initial,int min,int max,java.util.function.IntConsumer change){TextView label=new TextView(getContext());label.setText(name+": "+initial+"%");body.addView(label);SeekBar slider=new SeekBar(getContext());slider.setMax(max-min);slider.setProgress(initial-min);body.addView(slider);slider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onProgressChanged(SeekBar b,int n,boolean user){if(user){label.setText(name+": "+(n+min)+"%");change.accept(n+min);}}public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){}});}
    private LinearLayout section(LinearLayout root,String title,boolean open){
        boolean nested="section-body".equals(root.getTag());Button header=nested?new Button(getContext()):new MenuSparkleButton(getContext(),root.getChildCount());header.setTag(nested?"subsection-header":"section-header");if(!nested){header.setCompoundDrawablesWithIntrinsicBounds(new MenuSectionIcon(getContext(),title),null,null,null);header.setCompoundDrawablePadding(BrandUi.dp(getContext(),8));}LinearLayout body=new LinearLayout(getContext());body.setTag("section-body");body.setOrientation(LinearLayout.VERTICAL);
        body.setPadding(BrandUi.dp(getContext(),12),BrandUi.dp(getContext(),8),BrandUi.dp(getContext(),12),BrandUi.dp(getContext(),12));body.setBackgroundColor(0xcc171b20);body.setVisibility(open?VISIBLE:GONE);header.setText(title+(open?" ▾":" ▸"));
        header.setOnClickListener(v->{boolean expand=body.getVisibility()!=VISIBLE;body.setVisibility(expand?VISIBLE:GONE);header.setText(title+(expand?" ▾":" ▸"));});root.addView(header);root.addView(body);return body;
    }
    private Runnable wizardFinished;
    void startStartupWizard(Runnable complete){wizardFinished=complete;showWizard(0);}
    private void finishWizard(){if(wizardFinished!=null&&!((RuntimeReadinessActivity)getContext()).readyForGame()){showWizard(1);return;}wizardOpen=false;invalidate();Runnable done=wizardFinished;wizardFinished=null;if(done!=null)done.run();}
    java.util.Map<String,Object> backupLayouts(){
        java.util.Map<String,Float> defaults=new java.util.HashMap<>();for(Control c:controls){defaults.put("x_"+c.label,c.defaultX);defaults.put("y_"+c.label,c.defaultY);}
        return OverlayBackup.snapshot(preferences.getAll(),defaults);
    }
    void reloadLayouts(){release();editing=false;contextEnabled=false;preferences.edit().putBoolean("context-controls",false).apply();profile="";activateProfile(getWidth(),getHeight());layoutControls(getWidth(),getHeight());invalidate();}
    private void showWizard(int step){
        wizardOpen=true;release();invalidate();
        RuntimeReadinessActivity app=(RuntimeReadinessActivity)getContext();
        String[] titles={"Welcome to LO: Saltlord Edition","Add your game & DLC","Choose a GPU driver","Restore your backups","Your controls & performance","Prepare shaders","Ready to play"};
        String[] text={"Let’s get you ready to play.\n\n1. Add your game files and optional DLC.\n2. Choose a driver.\n3. Restore saves and controls if you have backups.\n4. Choose graphics settings, then prepare shaders.\n\nYou can return to this setup from Options at any time.",
            "Start with Disc 1, or add all four discs now. Use matching USA/Europe discs from your own copy.\n\nDLC is optional. Add its ZIP or original Xbox packages from any folder. Tap Add more files until your list is ready, then install everything together.",
            "The system driver works without extra setup.\n\nAlready have a compatible custom driver? Import its ZIP below. You can change drivers later in Options.",
            "Moving from another Saltlord Edition build or upstream recomp? Restore game saves and touch layouts here before playing. You can skip this page on a fresh install. Game files, drivers and graphics preferences are kept separate.",
            "Choose a profile below or adjust each setting. Our preferred performance balance on a Z Fold 8 Ultra is 720p, SMAA and Dynamic 60. For sharper detail on the inner screen, Quality uses 1080p, SMAA and Steady 30; our latest tested scene held approximately 30 FPS. Low-spec is the conservative initial default. Results vary by scene, device, driver and cooling. These are test-based choices, not promises of a stable entire playthrough. Saved choices are retained across launches.\n\n"+DeviceUi.layoutNote(getContext())+" Customise controls later in Overlay options.",
            ShaderPreparationView.WARNING,
            "Setup is ready. Start game will finish installing any files waiting in your list and prepare shaders.\n\nLO: Saltlord Edition is an unofficial community project. You provide your own game files. Credits and full notices are available below and in About."};
        LinearLayout extras=new LinearLayout(getContext());extras.setOrientation(LinearLayout.VERTICAL);
        if(step==1)extras.addView(app.setupFilesPanel());
        if(step==2)extras.addView(app.setupDriverPanel());
        if(step==5)extras.addView(app.notificationSetupPanel(()->{showWizard(step+1);wizardChime.play();}));
        if(step==3){extras.addView(app.saveToolsPanel(true));Button restore=new Button(getContext());restore.setText("Restore overlay layouts");restore.setOnClickListener(v->app.restoreLayouts());extras.addView(restore);}
        if(step==4){
            Button resolution=new Button(getContext()),target=new Button(getContext()),antialiasing=new Button(getContext());String[] aaNames={"Off","FXAA","SMAA"};
            Runnable refresh=()->{int[] now=GraphicsChoices.current();resolution.setText("Render resolution: "+now[0]+"p — tap to change");target.setText("Frame-rate target: "+(now[1]==60?"Dynamic 60 (experimental)":"Steady 30")+" — tap to change");antialiasing.setText("Anti-aliasing: "+aaNames[Math.max(0,Math.min(2,now[2]))]);};
            resolution.setOnClickListener(v->{int[] now=GraphicsChoices.current();GraphicsChoices.choose(getContext(),now[0]==720?1080:720,now[1],now[2],false,refresh);});extras.addView(resolution);
            target.setOnClickListener(v->{int[] now=GraphicsChoices.current();GraphicsChoices.choose(getContext(),now[0],now[1]==30?60:30,now[2],false,refresh);});extras.addView(target);
            antialiasing.setOnClickListener(v->GraphicsChoices.selectAa(getContext(),refresh));extras.addView(antialiasing);refresh.run();
            TextView more=new TextView(getContext());more.setTag("secondary");more.setText(GraphicsChoices.HELP);extras.addView(more);GraphicsChoices.presets(getContext(),extras,refresh);
        }
        if(step==6){extras.addView(app.setupFilesPanel());Button credits=new Button(getContext());credits.setText("Credits & acknowledgements");credits.setOnClickListener(v->BrandUi.showCredits(getContext()));extras.addView(credits);}
        BrandUi.textPage(getContext(),titles[step]+" ("+(step+1)+"/7)",text[step],step>0?()->showWizard(step-1):null,step<6?()->{Runnable advance=()->{showWizard(step+1);wizardChime.play();};if(step==5)app.confirmProgressNotifications(advance);else advance.run();}:this::finishWizard,step==6?(wizardFinished!=null?"Start game":"Done"):"Next",step==6?()->BrandUi.showNotices(getContext()):null,null,null,extras,this::finishWizard,step==1&&wizardFinished!=null?app::readyForGame:null);
    }
    private void color(int value) { paint.setColor(value);paint.setAlpha(Math.round(Color.alpha(value)*opacity*drawFade)); }
    @Override protected void onDraw(Canvas canvas) {
        if(controlsLocked())return;
        paint.setColorFilter(preferences.getBoolean("monochrome",false)?DisplayAppearance.MONOCHROME:null);
        if(editing&&gridMode!=3){paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(1);paint.setColor(0x66ffffff);float step=gridStep();
            for(float x=0;x<getWidth();x+=step)canvas.drawLine(x,0,x,getHeight(),paint);
            for(float y=0;y<getHeight();y+=step)canvas.drawLine(0,y,getWidth(),y,paint);paint.setStyle(Paint.Style.FILL);}
        for(Control c:controls){
            if(!editing)c.contextFade=TouchContextPolicy.fade(c.contextFrom,c.contextTarget,android.os.SystemClock.uptimeMillis()-contextChanged);
            if(!editing&&Math.abs(c.contextFade-c.contextTarget)>.001f)postInvalidateOnAnimation();
            if(!editing&&(hidden||controllerHidden())&&c.kind<6)continue;
            drawFade=editing?1f:(c.kind<6?idleFade*c.contextFade:1f);
            if(drawFade<=0f)continue;
            float dx=0,dy=0;float cx=c.box.centerX(),cy=c.box.centerY();boolean held=fingers.containsValue(c);
            for(Map.Entry<Integer,Control> finger:fingers.entrySet())if(finger.getValue()==c){
                float[] p=positions.get(finger.getKey());if(p!=null){float[] axis=axes(finger.getKey(),c,p);dx=axis[0];dy=-axis[1];}
                float[] origin=origins.get(finger.getKey());if(origin!=null){cx=origin[0];cy=origin[1];}
            }
            float radius=c.box.width()/2;
            if(c.kind==1||c.kind==2){
                int mode=stickMode(c);if(!editing&&(mode==2||(mode==1&&!held)))continue;
                color(0x66555566);canvas.drawCircle(cx,cy,radius,paint);
                paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(2);color(0x99c6ccd5);canvas.drawCircle(cx,cy,radius,paint);paint.setStyle(Paint.Style.FILL);
                color(held?0xcc909aa8:0xbb888899);canvas.drawCircle(cx+dx*radius*.65f,cy+dy*radius*.65f,radius*.35f,paint);
                drawLabel(canvas,c.label,cx+dx*radius*.65f,cy+dy*radius*.65f,radius*.42f,0xffe1e4eb);continue;
            }
            if(c.kind==3){
                paint.setColor(Color.WHITE);paint.setAlpha(Math.round(255*opacity*drawFade));
                glyphs.draw(canvas,"D-pad",c.box,paint);
                // Artwork is independent of existing directional hit regions and input math.
                if(held){
                    paint.setColor(0x24a6d7f2);paint.setAlpha(Math.round(36*opacity*drawFade));
                    float x=c.box.centerX(),y=c.box.centerY(),arm=c.box.width()*.43f,half=c.box.width()*.145f;
                    dpadCasing.reset();dpadCasing.moveTo(x-half,y-arm);dpadCasing.lineTo(x+half,y-arm);
                    dpadCasing.lineTo(x+half,y-half);dpadCasing.lineTo(x+arm,y-half);
                    dpadCasing.lineTo(x+arm,y+half);dpadCasing.lineTo(x+half,y+half);
                    dpadCasing.lineTo(x+half,y+arm);dpadCasing.lineTo(x-half,y+arm);
                    dpadCasing.lineTo(x-half,y+half);dpadCasing.lineTo(x-arm,y+half);
                    dpadCasing.lineTo(x-arm,y-half);dpadCasing.lineTo(x-half,y-half);
                    dpadCasing.close();canvas.drawPath(dpadCasing,paint);
                }
                continue;
            }
            int face=c.label.equals("A")?0xff74bf62:c.label.equals("B")?0xffdf6464:c.label.equals("X")?0xff669edc:c.label.equals("Y")?0xffe7ce64:0xffe1e4eb;
            color(held?0xbb626b7a:0x8840404c);
            boolean shoulder=c.kind==4||c.kind==5||c.label.equals("LB")||c.label.equals("RB");
            if(shoulder)canvas.drawRoundRect(c.box,radius*.32f,radius*.32f,paint);else canvas.drawOval(c.box,paint);
            paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(1.5f);color(0x77a4acbb);
            if(shoulder)canvas.drawRoundRect(c.box,radius*.32f,radius*.32f,paint);else canvas.drawOval(c.box,paint);paint.setStyle(Paint.Style.FILL);
            String label=c.kind==7?(hidden?"Show":"Hide"):c.label;
            if(c.kind==6&&optionsLogo!=null){
                RectF mark=new RectF(c.box);mark.inset(c.box.width()*.15f,c.box.height()*.15f);
                paint.setAlpha(Math.round(255*opacity*drawFade));paint.setFilterBitmap(true);BrandUi.fitMark(canvas,optionsLogo,mark,paint);
            }else {paint.setColor(Color.WHITE);paint.setAlpha(Math.round(255*opacity*drawFade));
                if(!glyphs.draw(canvas,c.label,c.box,paint))drawLabel(canvas,label,cx,cy,c.box.height()*(c.kind>=6?.21f:.30f),face);}

        }
        drawFade=1f;
        if(editing){color(0xdd263d4e);canvas.drawRoundRect(editorGrid,8,8,paint);drawLabel(canvas,gridLabel(),editorGrid.centerX(),editorGrid.centerY(),editorGrid.height()*.30f,0xffa6d7f2);color(0xdd414651);canvas.drawRoundRect(editorDone,8,8,paint);drawLabel(canvas,"Done",editorDone.centerX(),editorDone.centerY(),editorDone.height()*.42f,0xffe1e4eb);}
    }
    private void drawLabel(Canvas canvas,String text,float x,float y,float size,int value){color(value);paint.setTextAlign(Paint.Align.CENTER);paint.setTextSize(size);canvas.drawText(text,x,y-(paint.ascent()+paint.descent())/2,paint);}
}
