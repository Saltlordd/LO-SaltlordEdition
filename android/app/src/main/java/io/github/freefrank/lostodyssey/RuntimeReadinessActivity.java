package io.github.freefrank.lostodyssey;

import android.app.AlertDialog;
import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.view.ViewGroup;
import android.content.ClipData;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;
import android.widget.Toast;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.LinearLayout;
import android.view.WindowManager;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.concurrent.CountDownLatch;
import org.libsdl.app.SDLActivity;

/** SDL owns lifecycle, native thread, window and controllers. */
public final class RuntimeReadinessActivity extends SDLActivity {
    /** Diagnostic snapshot only: does not change power, display or pacing settings. */
    public String performanceEnvironment() {
        android.os.PowerManager power=(android.os.PowerManager)getSystemService(POWER_SERVICE);
        int thermal=android.os.Build.VERSION.SDK_INT>=29&&power!=null?power.getCurrentThermalStatus():-1;
        android.view.Display display=getWindowManager().getDefaultDisplay();
        return "thermalStatus="+thermal+" powerSave="+(power!=null&&power.isPowerSaveMode())+" displayHz="+(display!=null?display.getRefreshRate():0);
    }
    private volatile CountDownLatch resultsClosed;
    private volatile CountDownLatch isoChosen;
    private volatile int selectedFd=-1;
    private TouchGamepadView touch;
    static native void nativeBorderColour(int choice);
    static native int nativeSaveAnywhere(int choice);
    static native String[] nativeControllers();
    static native void nativeControllerConfig(boolean active);
    static native int[] nativeControllerState(int instance);
    static native boolean nativeControllerMapping(int instance,int[] bindings);
    public void refreshControllerProfiles(){startupNotificationHandler.post(()->{if(!destroyed)ControllerProfiles.applyConnected(this);});}
    private int flexMode;
    private PerformanceMonitor performanceMonitor;
    PerformanceMonitor performanceMonitorForOptions(){return performanceMonitor;}
    void refreshMonitor(){if(performanceMonitor!=null)performanceMonitor.refresh();}
    private volatile boolean launcherVisible,launcherStarted;
    private boolean skipLauncher;
    private android.view.View launcher;
    boolean launcherOpen(){return launcherVisible;}
    void menuChime(){completionChime.play();}
    void refreshDisplay(){if(mLayout!=null)refreshDisplayTree(mLayout);}
    private static void refreshDisplayTree(android.view.View view){
        if(view instanceof android.view.ViewGroup){android.view.ViewGroup group=(android.view.ViewGroup)view;for(int i=0;i<group.getChildCount();i++)refreshDisplayTree(group.getChildAt(i));}
        view.requestLayout();view.invalidate();
    }
    void editControls(int mode){
        if(assetBusy||(!launcherVisible&&setupInProgress()))return;
        if(touch!=null)touch.release();
        final int oldOrientation=getRequestedOrientation();
        if(mode!=0)setRequestedOrientation(mode==2?ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT:ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
        TouchGamepadView preview=new TouchGamepadView(this,mode);
        final android.app.Dialog dialog=new android.app.Dialog(this);
        preview.preparePreview(dialog::dismiss);
        android.widget.FrameLayout root=new android.widget.FrameLayout(this){
            @Override protected void onMeasure(int ws,int hs){int w=MeasureSpec.getSize(ws),h=MeasureSpec.getSize(hs);int split=mode==0?0:h/2;getChildAt(0).measure(MeasureSpec.makeMeasureSpec(w,MeasureSpec.EXACTLY),MeasureSpec.makeMeasureSpec(split,MeasureSpec.EXACTLY));getChildAt(1).measure(MeasureSpec.makeMeasureSpec(w,MeasureSpec.EXACTLY),MeasureSpec.makeMeasureSpec(h-split,MeasureSpec.EXACTLY));setMeasuredDimension(w,h);}
            @Override protected void onLayout(boolean changed,int l,int t,int right,int bottom){int w=right-l,h=bottom-t,split=mode==0?0:h/2;getChildAt(0).layout(0,0,w,split);getChildAt(1).layout(0,split,w,h);}
        };
        root.setBackground(FlexPaneBackground.create());
        LinearLayout top=new LinearLayout(this);top.setOrientation(LinearLayout.VERTICAL);top.setGravity(android.view.Gravity.CENTER);top.setPadding(BrandUi.dp(this,16),0,BrandUi.dp(this,16),0);
        TextView instruction=new TextView(this);instruction.setText(mode==0?"Edit overlay":"Game viewport — controls stay in the lower half.\nDrag controls below; Grid changes spacing. Done saves and closes.");instruction.setTextColor(BrandUi.SECONDARY);instruction.setGravity(android.view.Gravity.CENTER);top.addView(instruction);
        android.widget.Button appearance=new android.widget.Button(this);appearance.setText("Control size & opacity");appearance.setOnClickListener(v->preview.showEditorAppearance());top.addView(appearance);BrandUi.style(top);
        root.addView(top);root.addView(preview);
        // Fullscreen overlay editors need a separate compact settings action above their canvas.
        if(mode==0){preview.setOnLongClickListener(v->{preview.showEditorAppearance();return true;});}
        dialog.setContentView(root);dialog.setOnDismissListener(d->{setRequestedOrientation(oldOrientation);if(touch!=null)touch.reloadLayouts();});dialog.show();
        if(dialog.getWindow()!=null){dialog.getWindow().setBackgroundDrawable(new android.graphics.drawable.ColorDrawable(BrandUi.SURFACE));dialog.getWindow().setLayout(-1,-1);}
    }
    private void flexControls(boolean copy){
        java.util.function.IntConsumer action=mode->{if(copy)confirmCopyCover(mode);else editControls(mode);};
        if(flexMode!=0){action.accept(flexMode);return;}
        AlertDialog d=new AlertDialog.Builder(this).setTitle(copy?"Copy cover layout to…":"Customise Flex controls")
            .setItems(new String[]{"Landscape fold controls","Portrait fold controls"},(which,index)->action.accept(index+1)).setNegativeButton("Back",null).create();d.show();BrandUi.finishDialog(d);
    }
    private void confirmCopyCover(int mode){
        AlertDialog d=new AlertDialog.Builder(this).setTitle("Replace this Flex layout?").setMessage("Copy the cover screen's positions, sizes and stick settings to this Flex layout. Its opacity and your fullscreen layouts are kept.")
            .setNegativeButton("Cancel",null).setPositiveButton("Copy",(which,index)->{if(FlexControlSettings.copyCover(this,mode)){if(touch!=null)touch.reloadLayouts();Toast.makeText(this,"Cover layout copied to Flex controls.",Toast.LENGTH_SHORT).show();}else Toast.makeText(this,"Could not save the layout. Please try again.",Toast.LENGTH_LONG).show();}).create();d.show();BrandUi.finishDialog(d);
    }
    private volatile boolean fastForwardRestored;
    int flexMode(){return flexMode;}
    void chooseFlexMode(){
        if(!DeviceUi.flexAvailable(this,flexMode)){Toast.makeText(this,DeviceUi.flexUnavailable(this),Toast.LENGTH_LONG).show();return;}
        if(assetBusy||(!launcherVisible&&setupInProgress())){Toast.makeText(this,"Finish setup or importing before changing the layout.",Toast.LENGTH_LONG).show();return;}
        LinearLayout panel=new LinearLayout(this);panel.setOrientation(LinearLayout.VERTICAL);int pad=BrandUi.dp(this,16);panel.setPadding(pad,pad,pad,pad);
        TextView help=new TextView(this);help.setText("Hold your open phone in landscape (wide) and look at the crease. Choose the matching picture below. Both modes put the game above the controls.\n\nAuto-Hide is temporarily off in Flex mode; your saved setting resumes in fullscreen. The manual Hide Overlay button still works.");panel.addView(help);
        for(int mode=1;mode<=2;mode++){
            final int selected=mode;android.widget.Button choice=new android.widget.Button(this);
            choice.setEnabled(DeviceUi.flexAvailable(this,0));choice.setAlpha(choice.isEnabled()?1f:0.45f);
            choice.setText(mode==1?"Landscape fold\nHorizontal crease when held wide":"Portrait fold\nVertical crease when held wide — the app turns upright");
            choice.setCompoundDrawablesWithIntrinsicBounds(new HingeExampleIcon(this,mode==2),null,null,null);choice.setCompoundDrawablePadding(BrandUi.dp(this,12));
            choice.setOnClickListener(v->confirmFlexMode(selected));panel.addView(choice);
        }
        android.widget.Button customise=new android.widget.Button(this);customise.setText("Customise Flex controls");customise.setOnClickListener(v->flexControls(false));panel.addView(customise);
        android.widget.Button copy=new android.widget.Button(this);copy.setText("Copy cover layout");copy.setOnClickListener(v->flexControls(true));panel.addView(copy);
        TextView settingsNote=new TextView(this);settingsNote.setTag("secondary");settingsNote.setText("Flex starts with 100% control opacity. Each fold layout has its own saved positions, sizes and opacity; editing is confined to the lower half.");panel.addView(settingsNote);
        TextView note=new TextView(this);note.setTag("secondary");note.setText("Pictures show the phone held wide. The split is centred manually. Closing and reopening the app returns to fullscreen.");panel.addView(note);
        if(flexMode!=0){android.widget.Button full=new android.widget.Button(this);full.setText("Return to fullscreen");full.setOnClickListener(v->confirmFlexMode(0));panel.addView(full);}
        android.widget.ScrollView scroll=new android.widget.ScrollView(this);scroll.addView(panel);
        AlertDialog d=new AlertDialog.Builder(this).setTitle("Flex mode (experimental)").setView(BrandUi.menu(this,scroll)).setNegativeButton("Back",null).create();d.show();BrandUi.finishDialog(d);
    }
    @Override protected org.libsdl.app.SDLSurface createSDLSurface(android.content.Context context){
        return flexMode==0?super.createSDLSurface(context):new org.libsdl.app.FlexSDLSurface(context);
    }
    private void confirmFlexMode(int mode){
        if(mode!=0&&!DeviceUi.flexAvailable(this,0)){Toast.makeText(this,DeviceUi.flexUnavailable(this),Toast.LENGTH_LONG).show();return;}
        AlertDialog d=new AlertDialog.Builder(this).setTitle(mode==0?"Return to fullscreen?":launcherVisible?"Start in Flex mode?":"Restart in Flex mode?")
            .setMessage((launcherVisible?"The app will switch layouts, then start setup or load your game.":"The game will restart and unsaved progress will be lost.")+(mode==0?"":"\n\nChoose the arrangement that matches your fold. Closing and reopening the app returns to fullscreen."))
            .setNegativeButton("Cancel",null).setPositiveButton(launcherVisible?"Start":"Restart",(dialog,which)->{if(mode!=0&&!DeviceUi.flexAvailable(this,0)){Toast.makeText(this,DeviceUi.flexUnavailable(this),Toast.LENGTH_LONG).show();return;}flexMode=mode;restartNow();}).create();d.show();BrandUi.finishDialog(d);
    }
    // Settings only: never persist a currently held trigger or active acceleration.
    public void persistFastForward(int enabled,int mode,int rate){
        if(!fastForwardRestored)return;
        android.content.SharedPreferences p=getSharedPreferences("fast-forward",MODE_PRIVATE);
        if(p.getBoolean("enabled",false)==(enabled!=0)&&p.getInt("mode",0)==mode&&p.getInt("rate",2)==rate&&p.contains("enabled"))return;
        p.edit().putBoolean("enabled",enabled!=0).putInt("mode",mode).putInt("rate",rate).apply();
    }
    private void restoreFastForward(){
        android.content.SharedPreferences p=getSharedPreferences("fast-forward",MODE_PRIVATE);
        int mode=p.getInt("mode",0),rate=p.getInt("rate",2),enabled=p.getBoolean("enabled",false)?1:0;
        fastForwardRestored=true;
        nativeFastForward(2,mode);nativeFastForward(3,rate);nativeFastForward(1,enabled);
    }
    private void saveFastForward(){
        if(!fastForwardRestored)return;
        int[] state=nativeFastForward(0,0);
        // Runtime shutdown disables acceleration for safety, not as a user preference.
        if(state.length>4&&state[4]!=0)return;
        getSharedPreferences("fast-forward",MODE_PRIVATE).edit().putBoolean("enabled",state[0]!=0).putInt("mode",state[1]).putInt("rate",state[2]).commit();
    }
    private AlertDialog importDialog;
    private ProgressBar importBar;
    private TextView importText;
    private volatile boolean destroyed=false;
    static native void nativeTouch(int buttons,int lt,int rt,int lx,int ly,int rx,int ry);
    static native void nativeCancelImport();
    static native void nativeDebugMenu();
    static native int[] nativeFastForward(int command,int value);
    static native int nativeTouchContext();
    static native void nativeAppMenu(boolean open);
    static native void nativeSurfaceReady(boolean ready);
    private boolean resumed=false,surfaceReady=false;
    private long lastBack=-1;
    private AlertDialog exitDialog;
    private void publishSurface(){nativeSurfaceReady(resumed&&surfaceReady);}
    @Override protected void onResume(){super.onResume();DeviceUi.observe(this);resumed=true;if(performanceMonitor!=null)performanceMonitor.setRunning(true);if(mSurface!=null)surfaceReady=mSurface.getHolder().getSurface().isValid()&&mSurface.getWidth()>0&&mSurface.getHeight()>0;publishSurface();ShaderPreparationService.attach(this);ControllerProfiles.applyConnected(this);}
    @Override public boolean dispatchKeyEvent(android.view.KeyEvent e){
        if(e.getKeyCode()==android.view.KeyEvent.KEYCODE_BACK){if(e.getAction()==android.view.KeyEvent.ACTION_DOWN&&e.getRepeatCount()==0)onBackPressed();return true;}
        return super.dispatchKeyEvent(e);
    }
    @Override public void onBackPressed(){
        if(launcherVisible){
            // Keep SDL's startup waiter attached to this resumable launcher.
            // Root Back backgrounds the task; Exit app is the explicit shutdown.
            if(touch!=null)touch.release();
            moveTaskToBack(true);return;
        }
        if(exitDialog!=null)return;
        long now=android.os.SystemClock.elapsedRealtime();
        if(lastBack<0||now-lastBack>2000){lastBack=now;Toast.makeText(this,"Press Back again to exit",Toast.LENGTH_SHORT).show();return;}
        lastBack=-1;if(touch!=null)touch.release();
        exitDialog=new AlertDialog.Builder(this).setTitle("Exit LO: Saltlord Edition?")
            .setMessage("Are you sure you would like to exit the app? Any unsaved progress will be lost.")
            .setNegativeButton("Keep playing",null).setPositiveButton("Exit",(d,w)->finishAndRemoveTask()).create();
        exitDialog.setOnDismissListener(d->exitDialog=null);exitDialog.show();BrandUi.finishDialog(exitDialog);
    }
    static native int nativeFrameRate(int fps);
    static native long[] nativePerformanceSnapshot();
    static native int nativeRenderResolution(int height);
    static native int[] nativeGraphicsPreset(int height,int aa);
    static native int nativeAntiAliasing(int choice);
    static native long nativePreparationProgress();
    static native boolean nativeGuestStarted();
    static native boolean nativePreparationStopped();
    static native boolean nativePreparationFinished();
    static native void nativePreparationServiceEvent(int event);
    private ShaderPreparationView preparationView;
    private final WizardChime completionChime=new WizardChime();
    String pendingContentStatus(){StringBuilder out=new StringBuilder();String status=nativeContentStatus();if(status!=null)for(String line:status.split("\n"))if(line.contains("ready"))out.append(line).append('\n');String dlc=ContentInventory.pendingDlc(getFilesDir());if(!dlc.isEmpty())out.append(dlc);return out.toString().trim();}
    void preparationChanged(long progress,boolean active){if(preparationView!=null){preparationView.update(progress);preparationView.setVisibility(active?android.view.View.VISIBLE:android.view.View.GONE);if(touch!=null)touch.setPreparing(active);}}
    public void importCompleted(){runOnUiThread(()->completionChime.play());}

    static native boolean nativeFinishTest();
    static native boolean nativeDriverReady();
    static native String nativeStageContent(String directory);
    private volatile boolean assetBusy=false;
    void discardPendingContent() {
        if(assetBusy||!nativeDriverReady()){new AlertDialog.Builder(this).setMessage("Wait for the game to finish starting before discarding queued imports.").setPositiveButton("OK",null).show();return;}
        new AlertDialog.Builder(this).setTitle("Discard pending imports?").setMessage("Removes only queued disc/DLC imports. Installed game content and saves are kept.").setNegativeButton("No",null).setPositiveButton("Yes",(d,w)->{assetBusy=true;new Thread(()->{ContentImportFiles.remove(new java.io.File(getFilesDir(),"pending-content"));assetBusy=false;runOnUiThread(()->android.widget.Toast.makeText(this,"Pending imports discarded",android.widget.Toast.LENGTH_SHORT).show());},"discard-content").start();}).show();
    }
    private final java.util.LinkedHashMap<String,Uri> contentQueue=new java.util.LinkedHashMap<>();
    private AlertDialog contentPicker;
    private LinearLayout contentRows;
    private TextView wizardContentStatus, wizardDriverStatus;
    static native String nativeContentStatus();
    static native boolean nativeHasDiscOne();
    boolean setupInProgress(){return startupClosed!=null&&!launcherVisible;}
    boolean readyForGame(){
        loadQueue();
        if(assetBusy){Toast.makeText(this,"Please wait for installation to finish.",Toast.LENGTH_LONG).show();return false;}
        if(!contentQueue.isEmpty()){Toast.makeText(this,"Open Add discs or DLC and install your added files first.",Toast.LENGTH_LONG).show();return false;}
        if(!nativeHasDiscOne()){Toast.makeText(this,"Add and install Disc 1 to start.",Toast.LENGTH_LONG).show();return false;}
        return true;
    }
    private android.content.SharedPreferences queuePrefs(){return getSharedPreferences("content-queue",MODE_PRIVATE);}
    private void loadQueue(){
        if(!contentQueue.isEmpty())return;
        try{org.json.JSONArray a=new org.json.JSONArray(queuePrefs().getString("uris","[]"));for(int i=0;i<a.length()&&i<64;i++){Uri u=Uri.parse(a.getString(i));contentQueue.put(u.toString(),u);}}catch(Exception e){Log.w("LO.Import","Invalid saved file list",e);}
    }
    private void saveQueue(){org.json.JSONArray a=new org.json.JSONArray();for(String u:contentQueue.keySet())a.put(u);queuePrefs().edit().putString("uris",a.toString()).apply();}
    private String fileName(Uri u){try(android.database.Cursor c=getContentResolver().query(u,new String[]{android.provider.OpenableColumns.DISPLAY_NAME},null,null,null)){if(c!=null&&c.moveToFirst())return c.getString(0);}catch(RuntimeException e){}return "Selected file";}
    ViewGroup setupFilesPanel(){
        LinearLayout panel=new LinearLayout(this);panel.setOrientation(LinearLayout.VERTICAL);
        wizardContentStatus=new TextView(this);wizardContentStatus.setText(contentSummary());panel.addView(wizardContentStatus);
        android.widget.Button add=new android.widget.Button(this);add.setText("Add discs or DLC");add.setOnClickListener(v->pickContent());panel.addView(add);return panel;
    }
    ViewGroup setupDriverPanel(){
        LinearLayout panel=new LinearLayout(this);panel.setOrientation(LinearLayout.VERTICAL);
        wizardDriverStatus=new TextView(this);panel.addView(wizardDriverStatus);refreshSetupStatus();
        android.widget.Button add=new android.widget.Button(this);add.setText("Import driver ZIP");add.setOnClickListener(v->pickGpuDriver());panel.addView(add);
        android.widget.Button system=new android.widget.Button(this);system.setText("Use system driver");system.setOnClickListener(v->{GpuDriverStore.select(this,GpuDriverStore.SYSTEM_DRIVER);refreshSetupStatus();});panel.addView(system);return panel;
    }
    private String contentSummary(){
        String summary=nativeContentStatus();if(summary==null)summary="No game files installed yet.";if(setupInProgress())summary=summary.replace("ready — install at next start","ready — install when setup finishes");return summary+"\n"+ContentInventory.dlc(getFilesDir(),setupInProgress());
    }
    private void refreshSetupStatus(){
        if(wizardContentStatus!=null)wizardContentStatus.setText(contentSummary());
        if(wizardDriverStatus!=null){GpuDriverStore.Installed d=GpuDriverStore.selectedDriver(this);wizardDriverStatus.setText("Selected: "+(d==null?"System driver":d.metadata.name)+(setupInProgress()?"\nUsed when setup finishes.":"\nChanges apply after restart."));}
    }
    void pickContent(){
        if(assetBusy){Toast.makeText(this,"Installation is still running.",Toast.LENGTH_SHORT).show();return;}
        loadQueue();if(contentPicker!=null&&contentPicker.isShowing()){refreshContentRows();return;}
        LinearLayout page=new LinearLayout(this);page.setOrientation(LinearLayout.VERTICAL);int pad=BrandUi.dp(this,16);page.setPadding(pad,pad,pad,pad);
        TextView help=new TextView(this);help.setText("Add files from any folder. Keep adding until your list is ready, then install them together.\n\nStart with Disc 1, or add all four matching discs. DLC is optional: select its ZIP or Xbox package files.");page.addView(help);
        contentRows=new LinearLayout(this);contentRows.setOrientation(LinearLayout.VERTICAL);page.addView(contentRows);
        android.widget.Button add=new android.widget.Button(this);add.setText("+ Add more files");add.setOnClickListener(v->startActivityForResult(new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*").addCategory(Intent.CATEGORY_OPENABLE).putExtra(Intent.EXTRA_ALLOW_MULTIPLE,true).addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION),73));page.addView(add);
        TextView restart=new TextView(this);restart.setText(setupInProgress()?"Your files will be installed before the game starts. Large discs take time. Leave this screen running; a crystal chime confirms successful completion.":"Restart is necessary to use newly installed files. After restart, this screen shows what is installed. Your saves and originals are kept.");page.addView(restart);
        android.widget.ScrollView scroll=new android.widget.ScrollView(this);scroll.addView(page);
        contentPicker=new AlertDialog.Builder(this).setTitle("Your discs & DLC").setView(BrandUi.menu(this,scroll)).setNegativeButton("Keep list & close",null).setPositiveButton("Continue & install",null).create();
        contentPicker.show();BrandUi.finishDialog(contentPicker);if(contentPicker.getWindow()!=null)contentPicker.getWindow().setLayout(Math.min(BrandUi.dp(this,900),Math.round(getResources().getDisplayMetrics().widthPixels*.94f)),Math.round(getResources().getDisplayMetrics().heightPixels*.86f));contentPicker.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{if(contentQueue.isEmpty()){Toast.makeText(this,"Add files first.",Toast.LENGTH_SHORT).show();return;}contentPicker.dismiss();installQueuedContent();});refreshContentRows();
    }
    private void refreshContentRows(){
        if(contentRows==null)return;contentRows.removeAllViews();TextView status=new TextView(this);status.setText(contentSummary());contentRows.addView(status);
        TextView title=new TextView(this);title.setText("\nAdded files ("+contentQueue.size()+") — not installed yet");contentRows.addView(title);
        for(Uri u:contentQueue.values()){
            LinearLayout row=new LinearLayout(this);TextView name=new TextView(this);name.setText(fileName(u));row.addView(name,new LinearLayout.LayoutParams(0,-2,1));
            android.widget.Button remove=new android.widget.Button(this);remove.setText("Remove");remove.setOnClickListener(v->{contentQueue.remove(u.toString());saveQueue();refreshContentRows();});row.addView(remove);contentRows.addView(row);
        }BrandUi.style(contentRows);
    }
    private void addContentFiles(Intent data){
        java.util.ArrayList<Uri> incoming=new java.util.ArrayList<>();if(data.getClipData()!=null)for(int i=0;i<data.getClipData().getItemCount();i++)incoming.add(data.getClipData().getItemAt(i).getUri());else if(data.getData()!=null)incoming.add(data.getData());
        for(Uri u:incoming){if(contentQueue.size()>=64&&!contentQueue.containsKey(u.toString())){Toast.makeText(this,"The list can hold 64 files. Install these before adding more.",Toast.LENGTH_LONG).show();break;}try{getContentResolver().takePersistableUriPermission(u,Intent.FLAG_GRANT_READ_URI_PERMISSION);}catch(SecurityException e){}contentQueue.put(u.toString(),u);}saveQueue();refreshContentRows();
    }
    void restartGame(){
        if(assetBusy){Toast.makeText(this,"Wait for the import to finish before restarting.",Toast.LENGTH_LONG).show();return;}
        AlertDialog confirm=new AlertDialog.Builder(this).setTitle("Restart LO: Saltlord Edition?")
            .setMessage("You will lose any unsaved progress. Continue?")
            .setNegativeButton("No",null).setPositiveButton("Yes",(d,w)->{
                restartNow();
            }).create();confirm.show();BrandUi.finishDialog(confirm);
    }
    private void restartNow(){
        if(assetBusy){Toast.makeText(this,"Wait for the transfer to finish before restarting.",Toast.LENGTH_LONG).show();return;}
        saveFastForward();
        if(touch!=null)touch.release();
        startActivity(new Intent(this,RestartActivity.class).putExtra("old-boot-process",android.os.Process.myPid()).putExtra("flex-mode",flexMode));
        finish(); // Same isolated-process handoff as Options > Restart.
    }
    public void awaitPreparationRestart(boolean compiled){
        // Successful native preparation validates this driver before a deliberate restart.
        GpuDriverStore.clearBootPending(this);
        runOnUiThread(this::trackGameStart);
        android.content.SharedPreferences prefs=getSharedPreferences("startup-options",MODE_PRIVATE);
        boolean returning=prefs.getBoolean("returning-after-preparation-restart",false);
        prefs.edit().remove("returning-after-preparation-restart").commit();
        if(!compiled||returning)return; // Cache-only starts and the immediate restart never loop.
        CountDownLatch choice=new CountDownLatch(1);
        runOnUiThread(()->{
            if(isFinishing()||isDestroyed()){choice.countDown();return;}
            preparationChanged(0,false);
            AlertDialog done=new AlertDialog.Builder(this).setTitle("Shader preparation complete")
                .setMessage("Your shader cache is ready. Restart before playing to begin with a fresh session using the saved cache.")
                .setPositiveButton("Restart now",(d,w)->{prefs.edit().putBoolean("returning-after-preparation-restart",true).commit();restartNow();})
                .setCancelable(false).create();done.show();BrandUi.finishDialog(done);
        });
        try{choice.await();}catch(InterruptedException e){Thread.currentThread().interrupt();}
    }
    private void installQueuedContent(){
        if(assetBusy)return;final java.util.ArrayList<Uri> uris=new java.util.ArrayList<>(contentQueue.values());if(uris.isEmpty())return;
        assetBusy=true;final AlertDialog busy=new AlertDialog.Builder(this).setTitle("Preparing discs & DLC").setMessage("Copying selected files…\nKeep this app open. Originals are kept.").setCancelable(false).create();busy.show();BrandUi.finishDialog(busy);
        new Thread(()->{
            File source=new File(getCacheDir(),"content-input-"+java.util.UUID.randomUUID());String message;boolean success=false;
            try{
                if(!source.mkdirs())throw new IOException("Cannot create import staging folder");
                int index=0;final long[] copied={0},last={0};
                for(Uri uri:uris){String name="";try(android.database.Cursor cursor=getContentResolver().query(uri,new String[]{android.provider.OpenableColumns.DISPLAY_NAME},null,null,null)){if(cursor!=null&&cursor.moveToFirst())name=cursor.getString(0);}
                    try(java.io.InputStream input=getContentResolver().openInputStream(uri)){
                        if(input==null)throw new IOException("Cannot read selected file");
                        index+=ContentImportFiles.copy(input,name==null?"":name,source,index,n->{copied[0]+=n;long now=android.os.SystemClock.elapsedRealtime();if(now-last[0]>500){last[0]=now;String text=String.format(java.util.Locale.US,"Copied %.2f GB\nKeep this app open.",copied[0]/1e9);runOnUiThread(()->{if(!destroyed)busy.setMessage(text);});}});
                    }
                }
                runOnUiThread(()->{if(!destroyed)busy.setMessage("Checking your files…\nLarge discs can take a few minutes.");});
                String result=nativeStageContent(source.getCanonicalPath());success=result!=null&&result.startsWith("OK:");message=result==null?"Importer returned no result":result.substring(result.indexOf(':')+1);
            }catch(IOException|RuntimeException e){message="Import failed: "+e.getMessage();}
            finally{ContentImportFiles.remove(source);assetBusy=false;}
            final String text=message;final boolean ok=success;runOnUiThread(()->{busy.dismiss();if(isFinishing()||isDestroyed())return;if(ok){completionChime.play();contentQueue.clear();saveQueue();}refreshSetupStatus();AlertDialog.Builder result=new AlertDialog.Builder(this).setTitle(ok?"Files added successfully":"Could not install files").setMessage(ok?(setupInProgress()?"Your files are ready. Finish setup to install them and prepare shaders before gameplay.":"Your files are ready. Restart to finish installation. You can add more files before restarting."):text).setNegativeButton("Back to file list",(d,w)->pickContent());if(ok&&!setupInProgress())result.setPositiveButton("Restart now",(d,w)->restartGame());else result.setPositiveButton("Continue setup",null);AlertDialog dialog=result.create();dialog.show();BrandUi.finishDialog(dialog);});
        },"content-import").start();
    }
    private volatile CountDownLatch startupClosed;
    private void showLauncher(CountDownLatch chosen){
        if(destroyed||isFinishing()||mLayout==null||touch==null){chosen.countDown();return;}
        launcherVisible=true;
        LauncherMenu menu=new LauncherMenu(this,()->{
            if(!launcherVisible||assetBusy)return;
            completionChime.play();launcherStarted=true;launcherVisible=false;mLayout.removeView(launcher);launcher=null;chosen.countDown();
        },()->{completionChime.play();touch.showLauncherOptions();},()->{
            if(assetBusy){Toast.makeText(this,"Wait for importing to finish before exiting.",Toast.LENGTH_LONG).show();return;}
            completionChime.play();launcherVisible=false;launcherStarted=false;launcher.setVisibility(android.view.View.GONE);startupNotificationHandler.postDelayed(()->{chosen.countDown();finishAndRemoveTask();},250);
        },()->{completionChime.play();chooseFlexMode();});
        launcher=menu;mLayout.addView(menu,new ViewGroup.LayoutParams(-1,-1));
    }
    private byte[] pendingLayoutBackup;
    private SaveBackup.Plan pendingSaveRestore;
    private File pendingSaveExport;
    public boolean chooseStartupAndWait(){
        try{SaveBackup.recover(SaveBackup.root(getFilesDir()));SaveBackup.remove(new File(getCacheDir(),"save-import"));}catch(IOException e){Log.e("LO.Saves","Save recovery failed",e);runOnUiThread(()->saveMessage("Could not recover an interrupted save restore. Restart before playing. Existing backup is retained."));return false;}
        // Storage is initialized here; a pre-bootstrap native query may have cached defaults.
        // Snapshot the complete tuple before JNI setters persist their merged configuration.
        try{int[] saved=SavedGraphics.read(getFilesDir());nativeRenderResolution(saved[0]);nativeAntiAliasing(saved[2]);nativeFrameRate(saved[1]);}
        catch(IOException e){Log.e("LO.Graphics","Cannot restore saved graphics",e);}
        restoreFastForward();
        if(!skipLauncher){
            CountDownLatch chosen=new CountDownLatch(1);startupClosed=chosen;
            runOnUiThread(()->showLauncher(chosen));
            try{chosen.await();}catch(InterruptedException e){Thread.currentThread().interrupt();return false;}finally{startupClosed=null;}
            if(!launcherStarted||destroyed||isFinishing())return false;
        }
        android.content.SharedPreferences startup=getSharedPreferences("startup-options",MODE_PRIVATE);
        if(!startup.getBoolean("wizard-complete",false)){
            CountDownLatch finished=new CountDownLatch(1);startupClosed=finished;
            runOnUiThread(()->{if(isFinishing()||isDestroyed()){finished.countDown();return;}if(touch==null){finished.countDown();return;}touch.startStartupWizard(()->{startup.edit().putBoolean("wizard-complete",true).commit();finished.countDown();});});
            try{finished.await();}catch(InterruptedException e){Thread.currentThread().interrupt();}finally{startupClosed=null;}
        }
        startup.edit().putBoolean("prepare-shaders",true).commit();
        if(destroyed||isFinishing())return false;
        applySelectedDriver();
        runOnUiThread(()->ShaderPreparationService.begin(this));
        return true;
    }
    private static final String SAVE_ANYWHERE_NOTE="Enables the original game's System → Save action outside normal save points. Close Options, then reopen the game's System menu. This is an experimental feature, not a save state; it cannot capture battles or cutscenes. The game's own save restriction is retained during split-party sections. Keep a separate normal save. These use the ordinary recomp save format, but unusual save locations may not reload safely. Xbox 360 compatibility has not been verified; raw recomp files are not Xbox 360 container files.";
    void changeSaveAnywhere(android.widget.Button button){
        if(nativeSaveAnywhere(-1)!=0){int result=nativeSaveAnywhere(0);button.setText("Save Anywhere (experimental): "+(nativeSaveAnywhere(-1)!=0?"on":"off"));if(result==-2)saveMessage("The setting changed for this session, but could not be saved. Try again before closing.");return;}
        AlertDialog d=new AlertDialog.Builder(this).setTitle("Enable Save Anywhere?").setMessage(SAVE_ANYWHERE_NOTE).setNegativeButton("Cancel",null).setPositiveButton("Enable",(dialog,which)->{int result=nativeSaveAnywhere(1);button.setText("Save Anywhere (experimental): "+(nativeSaveAnywhere(-1)!=0?"on":"off"));if(result==-2)saveMessage("The setting changed for this session, but could not be saved. Try again before closing.");}).create();d.show();BrandUi.finishDialog(d);
    }
    LinearLayout saveToolsPanel(boolean wizard){
        LinearLayout panel=new LinearLayout(this);panel.setOrientation(LinearLayout.VERTICAL);
        TextView note=new TextView(this);note.setTag("secondary");note.setText(wizard?"Restore saves and touch layouts before your first game. Both are optional. To move between builds, export your backups before uninstalling; uninstalling deletes app storage.":"Transfer ordinary game saves using a ZIP. Backup and restore are available from this app's start menu before Start game, so the game cannot write to slots during a transfer. Export before uninstalling. Touch layouts have a separate backup.");panel.addView(note);
        android.widget.Button restore=new android.widget.Button(this);restore.setText("Restore game saves from ZIP");restore.setOnClickListener(v->restoreSaves());panel.addView(restore);
        if(!wizard){android.widget.Button backup=new android.widget.Button(this);backup.setText("Backup game saves to ZIP");backup.setOnClickListener(v->exportSaves(false));panel.addView(backup);
            android.widget.Button previous=new android.widget.Button(this);previous.setText("Export backup from before last restore");previous.setEnabled(SaveBackup.previous(getFilesDir()).isFile());previous.setOnClickListener(v->exportSaves(true));panel.addView(previous);
            android.widget.Button anywhere=new android.widget.Button(this);anywhere.setText("Save Anywhere (experimental): "+(nativeSaveAnywhere(-1)!=0?"on":"off"));anywhere.setOnClickListener(v->changeSaveAnywhere(anywhere));panel.addView(anywhere);BrandUi.help(panel,"Save Anywhere",SAVE_ANYWHERE_NOTE);}
        TextView formats=new TextView(this);formats.setTag("secondary");formats.setText("Accepts Saltlord backups and ZIPs containing upstream recomp or extracted Xenia user00-style save folders. Slot names are kept. Original Xbox 360 container files must be converted first. Save data is checked structurally; loading and progression still need testing in game.");panel.addView(formats);return panel;
    }
    private boolean beginSaveTransfer(){
        if(assetBusy){saveMessage("Wait for the current import or backup to finish.");return false;}
        if(nativeGuestStarted()||(!launcherVisible&&!setupInProgress())){saveMessage("Restart to the app's start menu before transferring saves. Save your current game first. Restore is also available during initial setup.");return false;}
        assetBusy=true;return true;
    }
    void restoreSaves(){
        if(!beginSaveTransfer())return;
        try{startActivityForResult(new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*").addCategory(Intent.CATEGORY_OPENABLE),85);}catch(RuntimeException e){assetBusy=false;saveMessage("Cannot open the file picker: "+e.getMessage());}
    }
    void exportSaves(boolean previous){
        if(!beginSaveTransfer())return;
        File snapshot=new File(getCacheDir(),"save-export-"+java.util.UUID.randomUUID()+".zip");
        AlertDialog busy=new AlertDialog.Builder(this).setTitle("Save backup").setMessage("Preparing your backup…").setCancelable(false).create();busy.show();BrandUi.finishDialog(busy);
        new Thread(()->{try{
            if(previous){File old=SaveBackup.previous(getFilesDir());if(!old.isFile())throw new IOException("No earlier backup available");try(java.io.InputStream in=new FileInputStream(old);java.io.OutputStream out=new FileOutputStream(snapshot)){byte[] buffer=new byte[32768];int n;while((n=in.read(buffer))!=-1)out.write(buffer,0,n);}}
            else try(java.io.OutputStream out=new FileOutputStream(snapshot)){SaveBackup.export(SaveBackup.root(getFilesDir()),out);}
            runOnUiThread(()->{busy.dismiss();if(destroyed||isFinishing()){snapshot.delete();assetBusy=false;return;}pendingSaveExport=snapshot;try{startActivityForResult(new Intent(Intent.ACTION_CREATE_DOCUMENT).setType("application/zip").addCategory(Intent.CATEGORY_OPENABLE).putExtra(Intent.EXTRA_TITLE,previous?"LO-Saltlord-Saves-Before-Restore.zip":"LO-Saltlord-Saves-"+new java.text.SimpleDateFormat("yyyyMMdd-HHmmss",java.util.Locale.US).format(new java.util.Date())+".zip"),84);}catch(RuntimeException e){snapshot.delete();pendingSaveExport=null;assetBusy=false;saveMessage("Cannot open the file picker: "+e.getMessage());}});
        }catch(Exception e){snapshot.delete();runOnUiThread(()->{busy.dismiss();assetBusy=false;saveMessage("Backup failed: "+e.getMessage());});}},"save-backup").start();
    }
    private void saveDocument(int request,Intent data){
        Uri uri=data.getData();File snapshot=pendingSaveExport;if(request==84)pendingSaveExport=null;
        AlertDialog busy=new AlertDialog.Builder(this).setTitle(request==84?"Save backup":"Check save backup").setMessage(request==84?"Writing your backup…":"Checking slots before restoring…").setCancelable(false).create();busy.show();BrandUi.finishDialog(busy);
        new Thread(()->{try{
            if(request==84){if(snapshot==null)throw new IOException("No backup prepared");try(java.io.InputStream in=new FileInputStream(snapshot);java.io.OutputStream out=getContentResolver().openOutputStream(uri,"wt")){if(out==null)throw new IOException("Cannot write backup");byte[] buffer=new byte[32768];int n;while((n=in.read(buffer))!=-1)out.write(buffer,0,n);out.flush();}snapshot.delete();runOnUiThread(()->{busy.dismiss();assetBusy=false;saveMessage("Game saves backed up. Keep this ZIP outside app storage before reinstalling. It contains saves only; back up touch layouts separately.");});}
            else{final SaveBackup.Plan plan;try(java.io.InputStream in=getContentResolver().openInputStream(uri)){if(in==null)throw new IOException("Cannot read ZIP");plan=SaveBackup.prepare(in,SaveBackup.root(getFilesDir()),new File(getCacheDir(),"save-import"));}
                runOnUiThread(()->{busy.dismiss();if(destroyed||isFinishing()){closeSavePlan(plan);assetBusy=false;return;}pendingSaveRestore=plan;
                    String message="Import "+plan.slots.size()+" save slot(s): "+android.text.TextUtils.join(", ",plan.slots)+".\n\n"+(plan.overwritten.isEmpty()?"No existing slots will be replaced.":"Replaces: "+android.text.TextUtils.join(", ",plan.overwritten)+". An automatic backup of your existing saves is kept before replacement.")+" Other slots and all graphics, driver and overlay settings are kept.";
                    AlertDialog confirm=new AlertDialog.Builder(this).setTitle("Restore these game saves?").setMessage(message).setNegativeButton("Cancel",(d,w)->cancelSaveRestore()).setPositiveButton("Restore",(d,w)->installSavePlan(plan)).create();confirm.setOnCancelListener(d->cancelSaveRestore());confirm.show();BrandUi.finishDialog(confirm);
                });}
        }catch(Exception e){if(snapshot!=null)snapshot.delete();runOnUiThread(()->{busy.dismiss();assetBusy=false;saveMessage("Save transfer failed: "+e.getMessage()+". Existing saves were kept.");});}},"save-document").start();
    }
    private void cancelSaveRestore(){SaveBackup.Plan plan=pendingSaveRestore;pendingSaveRestore=null;assetBusy=false;closeSavePlan(plan);}
    private void closeSavePlan(SaveBackup.Plan plan){if(plan!=null)new Thread(()->{try{plan.close();}catch(IOException e){Log.w("LO.Saves","Staging cleanup failed",e);}},"save-cleanup").start();}
    private void installSavePlan(SaveBackup.Plan plan){
        pendingSaveRestore=null;AlertDialog busy=new AlertDialog.Builder(this).setTitle("Restore game saves").setMessage("Restoring checked saves…").setCancelable(false).create();busy.show();BrandUi.finishDialog(busy);
        new Thread(()->{String message;try{if(nativeGuestStarted())throw new IOException("Game started; restart before restoring");SaveBackup.install(plan,SaveBackup.root(getFilesDir()),SaveBackup.previous(getFilesDir()));message="Game saves restored. Use Continue or Load Game after Start game. Your other slots and settings are kept. Any previous saves were backed up; export that backup from Saves & backups before uninstalling.";}catch(Exception e){Log.e("LO.Saves","Save restore failed",e);message="Could not restore saves: "+e.getMessage()+". Restart before playing; the previous saves or their backup are retained.";}finally{closeSavePlan(plan);}final String result=message;runOnUiThread(()->{busy.dismiss();assetBusy=false;saveMessage(result);});},"save-restore").start();
    }
    private void saveMessage(String message){if(!destroyed&&!isFinishing()&&!isDestroyed()){AlertDialog d=new AlertDialog.Builder(this).setTitle("Saves & backups").setMessage(message).setPositiveButton("OK",null).create();d.show();BrandUi.finishDialog(d);}}
    void exportLayouts(){
        try{pendingLayoutBackup=OverlayBackup.encode(touch.backupLayouts());startActivityForResult(new Intent(Intent.ACTION_CREATE_DOCUMENT).setType("application/json").addCategory(Intent.CATEGORY_OPENABLE).putExtra(Intent.EXTRA_TITLE,"LO-Saltlord-Layouts.json"),81);}
        catch(Exception e){layoutMessage("Backup failed: "+e.getMessage());}
    }
    void restoreLayouts(){
        new AlertDialog.Builder(this).setTitle("Restore touch layouts?").setMessage("Replaces saved button positions, sizes, opacity, joystick settings and touch preferences from your backup. Game files, saves and graphics settings are kept.").setNegativeButton("Cancel",null).setPositiveButton("Choose backup",(d,w)->startActivityForResult(new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*").addCategory(Intent.CATEGORY_OPENABLE),82)).show();
    }
    private void layoutMessage(String message){if(!isFinishing()&&!isDestroyed()){AlertDialog d=new AlertDialog.Builder(this).setTitle("Touch layout backup").setMessage(message).setPositiveButton("OK",null).create();d.show();BrandUi.finishDialog(d);}}
    private void layoutDocument(int request,Intent data){
        final Uri uri=data.getData();final byte[] backup=pendingLayoutBackup;if(request==81)pendingLayoutBackup=null;
        new Thread(()->{try{
            if(request==81){if(backup==null)throw new IOException("No layout snapshot available");try(java.io.OutputStream out=getContentResolver().openOutputStream(uri,"wt")){if(out==null)throw new IOException("Cannot write backup");out.write(backup);out.flush();}runOnUiThread(()->layoutMessage(""+DeviceUi.backedUp(this)+" Keep this file outside app storage before reinstalling. This is a controls-only backup; it does not contain game saves."));}
            else{final java.util.Map<String,Object> restored;try(java.io.InputStream in=getContentResolver().openInputStream(uri)){if(in==null)throw new IOException("Cannot read backup");restored=OverlayBackup.decode(in);}runOnUiThread(()->{
                if(isFinishing()||isDestroyed())return;
                android.content.SharedPreferences prefs=getSharedPreferences("touch-options",MODE_PRIVATE);java.util.Map<String,?> original=prefs.getAll();android.content.SharedPreferences.Editor edit=prefs.edit();for(String key:original.keySet())if(OverlayBackup.kind(key)!=null)edit.remove(key);putLayoutSettings(edit,restored);for(String profile:new String[]{"flex-bottom","flex-side"})if(Boolean.TRUE.equals(restored.get(profile+"_initialized")))edit.putBoolean(profile+"_opacity-custom-v44",true);
                if(edit.commit()){touch.reloadLayouts();layoutMessage("Touch layouts restored successfully. Your current layout is applied immediately.");}
                else{android.content.SharedPreferences.Editor rollback=prefs.edit();for(String key:restored.keySet())rollback.remove(key);putLayoutSettings(rollback,original);rollback.commit();touch.reloadLayouts();layoutMessage("Could not save restored layouts. Previous settings were restored.");}
            });}
        }catch(Exception e){runOnUiThread(()->layoutMessage("Layout operation failed: "+e.getMessage()+". Existing layouts were kept."));}},"layout-backup").start();
    }
    private static void putLayoutSettings(android.content.SharedPreferences.Editor edit,java.util.Map<String,?> settings){
        for(java.util.Map.Entry<String,?> entry:settings.entrySet()){Object v=entry.getValue();String k=entry.getKey();if(v instanceof Boolean)edit.putBoolean(k,(Boolean)v);else if(v instanceof Float)edit.putFloat(k,(Float)v);else if(v instanceof Integer)edit.putInt(k,(Integer)v);else if(v instanceof Long)edit.putLong(k,(Long)v);else if(v instanceof String)edit.putString(k,(String)v);}
    }
    private android.widget.Button notificationPermissionButton,notificationSkipButton;
    private final android.os.Handler startupNotificationHandler=new android.os.Handler(android.os.Looper.getMainLooper());
    // Only observe the existing native startup flag while preparation hands off to gameplay.
    // This is not a gameplay monitor and does not keep the preparation service alive.
    private final Runnable startupNotification=new Runnable(){public void run(){
        if(destroyed||isFinishing()||isDestroyed())return;
        if(nativeGuestStarted()){ShaderPreparationService.running(RuntimeReadinessActivity.this);ControllerProfiles.applyConnected(RuntimeReadinessActivity.this);if(touch!=null)touch.invalidate();return;}
        if(!nativePreparationStopped())startupNotificationHandler.postDelayed(this,500);
    }};
    private void trackGameStart(){startupNotificationHandler.removeCallbacks(startupNotification);startupNotificationHandler.post(startupNotification);}
    @Override public void onRequestPermissionsResult(int code,String[] permissions,int[] grants){
        super.onRequestPermissionsResult(code,permissions,grants);
        if(code==83){
            boolean allowed=notificationsAllowed();
            if(notificationPermissionButton!=null){notificationPermissionButton.setText(allowed?"Progress notifications allowed":"Allow progress notifications");notificationPermissionButton.setEnabled(!allowed);}
            if(notificationSkipButton!=null)notificationSkipButton.setVisibility(allowed?android.view.View.GONE:android.view.View.VISIBLE);
            if(!allowed)Toast.makeText(this,"Notifications are optional. Reopen the app to check progress while minimised.",Toast.LENGTH_LONG).show();
        }
    }
    private boolean notificationsAllowed(){return android.os.Build.VERSION.SDK_INT<33||checkSelfPermission(android.Manifest.permission.POST_NOTIFICATIONS)==android.content.pm.PackageManager.PERMISSION_GRANTED;}
    android.view.View notificationSetupPanel(Runnable proceed){
        android.widget.LinearLayout panel=new android.widget.LinearLayout(this);panel.setOrientation(android.widget.LinearLayout.VERTICAL);
        android.widget.TextView why=new android.widget.TextView(this);
        why.setText("Optional progress notifications show shader preparation while the app is minimised and let you return with a tap. You can play without allowing them. Without notifications, reopen the app to check progress.");panel.addView(why);
        android.widget.Button allow=new android.widget.Button(this);allow.setText(notificationsAllowed()?"Progress notifications allowed":"Allow progress notifications");allow.setEnabled(!notificationsAllowed());
        allow.setOnClickListener(v->{getSharedPreferences("startup-options",MODE_PRIVATE).edit().putBoolean("notifications-explained",true).apply();if(android.os.Build.VERSION.SDK_INT>=33)requestPermissions(new String[]{android.Manifest.permission.POST_NOTIFICATIONS},83);});notificationPermissionButton=allow;panel.addView(allow);
        android.widget.Button skip=new android.widget.Button(this);skip.setText("Continue without notifications");skip.setVisibility(notificationsAllowed()?android.view.View.GONE:android.view.View.VISIBLE);
        skip.setOnClickListener(v->confirmProgressNotifications(proceed));notificationSkipButton=skip;panel.addView(skip);return panel;
    }
    void confirmProgressNotifications(Runnable proceed){
        if(notificationsAllowed()||getSharedPreferences("startup-options",MODE_PRIVATE).getBoolean("notifications-explained",false)){proceed.run();return;}
        AlertDialog d=new AlertDialog.Builder(this).setTitle("Continue without notifications?").setMessage("Preparation will still continue while minimised. You'll need to reopen the app to check progress. Notifications are optional.")
            .setPositiveButton("Continue",(dialog,w)->{getSharedPreferences("startup-options",MODE_PRIVATE).edit().putBoolean("notifications-explained",true).apply();proceed.run();}).setNegativeButton("Go back",null).create();d.show();BrandUi.finishDialog(d);
    }
    @Override protected void onCreate(Bundle state) {
        // Android can retain the original launch Intent after killing a process.
        // A consumed main-process token cannot silently restore Flex from that Intent.
        flexMode=FlexLaunchProvider.consume(this,getIntent());
        skipLauncher=getIntent().getBooleanExtra("resume-game",false);getIntent().removeExtra("resume-game");
        super.onCreate(state);
        DisplayAppearance.apply(this);
        ControllerProfiles.attach(this);
        setRequestedOrientation(flexMode==2?ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT:ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        if(mSurface!=null)mSurface.getHolder().addCallback(new android.view.SurfaceHolder.Callback(){
            public void surfaceCreated(android.view.SurfaceHolder h){surfaceReady=false;publishSurface();}
            public void surfaceChanged(android.view.SurfaceHolder h,int format,int w,int height){surfaceReady=w>0&&height>0&&h.getSurface().isValid();publishSurface();}
            public void surfaceDestroyed(android.view.SurfaceHolder h){surfaceReady=false;publishSurface();}
        });
        if(mLayout!=null) {
            touch=new TouchGamepadView(this,flexMode);
            preparationView=new ShaderPreparationView(this);preparationView.setVisibility(android.view.View.GONE);
            if(flexMode!=0&&mSurface!=null){
                mLayout.removeView(mSurface);
                mLayout.addView(new FlexPaneLayout(this,flexMode,mSurface,touch,preparationView),new ViewGroup.LayoutParams(-1,-1));
            }else {
                if(mSurface!=null){mLayout.removeView(mSurface);mLayout.addView(new AspectViewport(this,mSurface),0,new ViewGroup.LayoutParams(-1,-1));}
                mLayout.addView(preparationView,new ViewGroup.LayoutParams(-1,-1));
                mLayout.addView(touch,new ViewGroup.LayoutParams(-1,-1));
            }
            performanceMonitor=new PerformanceMonitor(this);mLayout.addView(performanceMonitor,new ViewGroup.LayoutParams(-1,-1));performanceMonitor.setRunning(resumed);
            completionChime.prepare(this);
            ShaderPreparationService.attach(this);

        }
    }
    @Override public void setOrientationBis(int w,int h,boolean resizable,String hint) {
        // SDL's resizable window otherwise promotes orientation to FULL_USER.
        setRequestedOrientation(flexMode==2?ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT:ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
    }
    @Override protected void onPause() {
        // The document picker pauses this activity; retain its prepared report until the result.
        DeviceUi.stopObserving();
        saveFastForward();
        resumed=false;if(performanceMonitor!=null)performanceMonitor.setRunning(false);lastBack=-1;publishSurface();
        if(touch!=null)touch.release();
        super.onPause();
    }
    @Override public void onWindowFocusChanged(boolean focused) {
        if(!focused&&touch!=null)touch.release();
        super.onWindowFocusChanged(focused);
    }
    // The native SDL thread waits; the Android UI remains free to run the picker.
    public int chooseGameIsoAndWait() {
        final CountDownLatch chosen=new CountDownLatch(1);
        selectedFd=-1; isoChosen=chosen;
        runOnUiThread(()->{
            if(isFinishing()||isDestroyed()){chosen.countDown();return;}
            new AlertDialog.Builder(this).setTitle("Add Disc 1")
                .setMessage("Choose your Disc 1 ISO. The app needs about 6 GB for its game files. Your original ISO stays where it is. Keep the app open until installation finishes.")
                .setPositiveButton("Choose ISO",(d,w)->{
                    try { startActivityForResult(new Intent(Intent.ACTION_OPEN_DOCUMENT)
                        .addCategory(Intent.CATEGORY_OPENABLE).setType("*/*"),71); }
                    catch(RuntimeException e){chosen.countDown();}
                }).setNegativeButton("Skip",(d,w)->chosen.countDown())
                .setOnCancelListener(d->chosen.countDown()).show();
        });
        try {chosen.await();}catch(InterruptedException e){Thread.currentThread().interrupt();}
        finally {isoChosen=null;}
        int fd=selectedFd; selectedFd=-1; return fd;
    }

    public void updateImportProgress(final long done,final long total,final String file) {
        runOnUiThread(()->{
            if(destroyed||isFinishing()||isDestroyed())return;
            if(file.isEmpty()) {
                if(importDialog!=null)importDialog.dismiss();
                importDialog=null;return;
            }
            if(importDialog==null) {
                LinearLayout layout=new LinearLayout(this);layout.setOrientation(LinearLayout.VERTICAL);
                int padding=(int)(24*getResources().getDisplayMetrics().density);
                layout.setPadding(padding,padding,padding,padding);
                layout.setBackground(BrandUi.backdrop(this));
                importText=new TextView(this);importText.setTextColor(BrandUi.PRIMARY);layout.addView(importText);TextView info=new TextView(this);info.setText("Multiple discs can take a while. Leave this screen running; a crystal chime confirms successful completion.");info.setTextColor(BrandUi.SECONDARY);layout.addView(info);
                importBar=new ProgressBar(this,null,android.R.attr.progressBarStyleHorizontal);
                importBar.setMax(1000);layout.addView(importBar);
                importDialog=new AlertDialog.Builder(this).setTitle("Installing game files")
                    .setView(layout).setNegativeButton("Cancel",(d,w)->nativeCancelImport()).create();
                importDialog.setCancelable(false);importDialog.show();BrandUi.finishDialog(importDialog);
            }
            importBar.setIndeterminate(total<=0);
            if(total>0)importBar.setProgress((int)(1000.0*done/total));
            importText.setText(total>0?String.format(java.util.Locale.US,"%.1f%% · %.2f / %.2f GB\n%s",100.0*done/total,done/1e9,total/1e9,file):file);
        });
    }
    @Override protected void onActivityResult(int request,int result,Intent data) {
        if(request==86){if(result==RESULT_OK&&data!=null&&data.getData()!=null)saveLogDocument(data.getData());else{if(pendingLogExport!=null)pendingLogExport.delete();pendingLogExport=null;logExportBusy=false;}return;}
        if(request==84||request==85){if(result==RESULT_OK&&data!=null&&data.getData()!=null){if(!assetBusy&&!beginSaveTransfer())return;saveDocument(request,data);}else{if(pendingSaveExport!=null)pendingSaveExport.delete();pendingSaveExport=null;assetBusy=false;}return;}
        if(request==81||request==82){if(result==RESULT_OK&&data!=null&&data.getData()!=null)layoutDocument(request,data);else if(request==81)pendingLayoutBackup=null;return;}
        if(request==73){if(result==RESULT_OK&&data!=null)addContentFiles(data);return;}
        if(request==72){
            if(result==RESULT_OK&&data!=null&&data.getData()!=null){
                final Uri uri=data.getData();AlertDialog importing=new AlertDialog.Builder(this).setTitle("Import GPU driver").setMessage("Checking driver ZIP…").setCancelable(false).show();
                new Thread(()->{String message;
                    try(java.io.InputStream input=getContentResolver().openInputStream(uri)){
                        if(input==null)throw new IOException("Cannot open driver ZIP");
                        File root=GpuDriverStore.root(this);if(!root.isDirectory()&&!root.mkdirs())throw new IOException("Cannot create driver folder");
                        GpuDriverStore.Installed driver=GpuDriverStore.installFromStream(input,new File(root,"import-"+java.util.UUID.randomUUID()));
                        GpuDriverStore.select(this,driver.id());message=driver.metadata.name+(launcherVisible?" selected. It will be used when you press Start game.":setupInProgress()?" selected. It will be used when setup finishes.":" selected. Restart the app to use it.");
                    }catch(IOException|RuntimeException e){message="Driver import failed: "+e.getMessage();}
                    final String resultMessage=message;runOnUiThread(()->{importing.dismiss();refreshSetupStatus();if(!isFinishing()&&!isDestroyed())new AlertDialog.Builder(this).setTitle("GPU driver").setMessage(resultMessage).setPositiveButton("OK",null).show();});
                },"GPU-driver-import").start();
            }return;
        }
        if(request!=71){super.onActivityResult(request,result,data);return;}
        CountDownLatch chosen=isoChosen;
        if(chosen==null)return;
        if(result==RESULT_OK&&data!=null&&data.getData()!=null) {
            try(ParcelFileDescriptor descriptor=getContentResolver().openFileDescriptor(data.getData(),"r")) {
                if(descriptor!=null)selectedFd=descriptor.detachFd();
            }catch(IOException|RuntimeException e) {
                Toast.makeText(this,"Couldn't open ISO: "+e.getMessage(),Toast.LENGTH_LONG).show();
            }
        }
        chosen.countDown();
    }

    @Override protected String[] getLibraries() {
        return new String[] { "SDL2", "LostOdysseyRecomp" };
    }
    void pickGpuDriver(){
        if(!GpuDriverStore.supported()){Toast.makeText(this,"Custom drivers require a supported Qualcomm Adreno device.",Toast.LENGTH_LONG).show();return;}
        Intent choose=new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*").addCategory(Intent.CATEGORY_OPENABLE);startActivityForResult(choose,72);
    }
    @Override protected String[] getArguments() {
        // Called once before the native boot thread starts, not on resume or fold changes.
        try {LaunchFrameRate.prepare(getFilesDir());}
        catch(IOException e){Log.e("LO.Launch","Could not prepare missing launch defaults; retained settings",e);}
        String failed=GpuDriverStore.takeFailedBoot(this);
        if(failed!=null)runOnUiThread(()->Toast.makeText(this,"Previous custom-driver launch did not finish initialising. Using the system driver.",Toast.LENGTH_LONG).show());
        nativeSetenv("LO_TRACE_LANGUAGE","1");
        nativeSetenv("LO_NATIVE_LIB_DIR",getApplicationInfo().nativeLibraryDir+"/");
        return new String[] { "--android-files", getFilesDir().getAbsolutePath(), "--android-cache", getCacheDir().getAbsolutePath(), "--android-first-boot" };
    }
    private void applySelectedDriver(){
        nativeSetenv("LO_CUSTOM_DRIVER_DIR","");nativeSetenv("LO_VK_CUSTOM_DRIVER","");
        GpuDriverStore.Installed driver=GpuDriverStore.selectedDriver(this);


        if(driver!=null){
            nativeSetenv("LO_CUSTOM_DRIVER_DIR",driver.directory.getAbsolutePath()+"/");nativeSetenv("LO_VK_CUSTOM_DRIVER",driver.metadata.libraryName);
            GpuDriverStore.markBootPending(this);
            runOnUiThread(()->{
                android.os.Handler h=new android.os.Handler(android.os.Looper.getMainLooper());
                h.postDelayed(new Runnable(){public void run(){if(isFinishing()||isDestroyed())return;if(nativeDriverReady())GpuDriverStore.clearBootPending(RuntimeReadinessActivity.this);else h.postDelayed(this,1000);}},1000);
            });
        }

    }

    // Called on SDL's native thread. Keep SDL_main alive while the share sheet
    // is open, otherwise SDLActivity would finish and interrupt the handoff.
    public void showProbeResultsAndWait(final String summary) {
        final CountDownLatch closed = new CountDownLatch(1);
        resultsClosed = closed;
        runOnUiThread(() -> {
            if (isFinishing() || isDestroyed()) { closed.countDown(); return; }
            AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle("Lost Odyssey runtime check")
                .setMessage(summary)
                .setPositiveButton("Save diagnostic report", null)
                .setNegativeButton("Continue", (d, which) -> closed.countDown())
                .setOnCancelListener(d -> closed.countDown())
                .create();
            dialog.show();BrandUi.finishDialog(dialog);
            // Retain the results dialog when sharing so the user can return or retry.
            dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v -> saveDiagnosticReport());
        });
        try { closed.await(); }
        catch (InterruptedException e) { Thread.currentThread().interrupt(); }
        finally { resultsClosed = null; }
    }

    private File pendingLogExport;
    private boolean logExportBusy;
    static final String REPORT_HELP="Email the saved report to saltlordstrikes@gmail.com. Include your device model, RAM, storage capacity, chipset, what happened, what you expected, and detailed steps to reproduce it. Say whether it happens every time or only on a first encounter, and whether you used the cover, inner or Flex layout. Screenshots or a short video are helpful. Automatically detected device details are included where available; please correct or complete them. Saves are not included.";
    void saveDiagnosticReport() {
        if(logExportBusy){Toast.makeText(this,"Finish or cancel the open file picker first.",Toast.LENGTH_SHORT).show();return;}
        logExportBusy=true;
        File source=new File(getFilesDir(),"state/logs/android-phase1.log");
        new Thread(()->{File snapshot=null;try{
            File directory=new File(getCacheDir(),"log-export");if(!directory.isDirectory()&&!directory.mkdirs())throw new IOException("Cannot create report folder");
            snapshot=File.createTempFile("diagnostic-",".txt",directory);
            try(FileOutputStream output=new FileOutputStream(snapshot)){output.write(DiagnosticReport.header(this).getBytes(java.nio.charset.StandardCharsets.UTF_8));try(FileInputStream input=new FileInputStream(source)){DiagnosticReport.copy(input,output);}output.getFD().sync();}
            final File ready=snapshot;runOnUiThread(()->{if(destroyed||isFinishing()){ready.delete();return;}pendingLogExport=ready;try{startActivityForResult(new Intent(Intent.ACTION_CREATE_DOCUMENT).setType("text/plain").addCategory(Intent.CATEGORY_OPENABLE).putExtra(Intent.EXTRA_TITLE,"LO-Saltlord-Report-"+new java.text.SimpleDateFormat("yyyyMMdd-HHmmss",java.util.Locale.US).format(new java.util.Date())+".txt"),86);}catch(RuntimeException e){ready.delete();pendingLogExport=null;logExportBusy=false;saveMessage("Cannot open file picker: "+e.getMessage());}});
        }catch(IOException|RuntimeException e){if(snapshot!=null)snapshot.delete();runOnUiThread(()->{logExportBusy=false;saveMessage("Could not prepare report: "+e.getMessage());});}},"diagnostic-snapshot").start();
    }
    private void saveLogDocument(Uri uri){File snapshot=pendingLogExport;pendingLogExport=null;if(snapshot==null||!snapshot.isFile()||snapshot.length()==0){logExportBusy=false;saveMessage("The prepared report is unavailable. Please retry Save diagnostic report.");return;}
        new Thread(()->{String message;try(java.io.InputStream input=new FileInputStream(snapshot);java.io.OutputStream output=getContentResolver().openOutputStream(uri,"wt")){if(output==null)throw new IOException("Cannot open destination");DiagnosticReport.copy(input,output);output.flush();message="Report saved.\n\n"+REPORT_HELP;}catch(IOException|RuntimeException e){message="Could not save report: "+e.getMessage()+". Retry Save diagnostic report.";}finally{snapshot.delete();}final String result=message;runOnUiThread(()->{logExportBusy=false;if(!destroyed&&!isFinishing())saveMessage(result);});},"diagnostic-save").start();
    }

    @Override protected void onDestroy() {
        DeviceUi.stopObserving();ControllerProfiles.detach(this);destroyed=true;if(startupClosed!=null)startupClosed.countDown();
        startupNotificationHandler.removeCallbacks(startupNotification);
        ShaderPreparationService.clearNotification(this);
        nativeCancelImport();nativeAppMenu(false);if(performanceMonitor!=null)performanceMonitor.setRunning(false);
        if(touch!=null)touch.release();
        CountDownLatch chosen=isoChosen;
        if(chosen!=null)chosen.countDown();
        CountDownLatch closed = resultsClosed;
        if (closed != null) closed.countDown();
        if(isFinishing())stopService(new Intent(this,ShaderPreparationService.class));
        completionChime.close();ShaderPreparationService.detach(this);
        super.onDestroy();
    }
}
