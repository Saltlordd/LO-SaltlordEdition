package io.github.freefrank.lostodyssey;
import android.app.*;
import android.content.*;
import android.os.*;
import java.lang.ref.WeakReference;
/** Same isolated runtime process: keeps user-requested local cache preparation alive. */
public final class ShaderPreparationService extends Service {
 private static final String CHANNEL="shader-preparation";private static final int ID=34;
 private static WeakReference<RuntimeReadinessActivity> activity=new WeakReference<>(null);private static ShaderPreparationService instance;
 private final Handler handler=new Handler(Looper.getMainLooper());private final WizardChime chime=new WizardChime();
 private PowerManager.WakeLock wake;private boolean hadWork,ended;private long last=-1;
 static void clearNotification(Context c){c.getSystemService(NotificationManager.class).cancel(ID);}
 static void running(Context c){
  NotificationManager manager=c.getSystemService(NotificationManager.class);
  NotificationChannel channel=new NotificationChannel(CHANNEL,"Shader preparation",NotificationManager.IMPORTANCE_LOW);channel.setSound(null,null);manager.createNotificationChannel(channel);
  Intent open=new Intent(c,RuntimeReadinessActivity.class).addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP|Intent.FLAG_ACTIVITY_CLEAR_TOP);
  PendingIntent pi=PendingIntent.getActivity(c,0,open,PendingIntent.FLAG_UPDATE_CURRENT|PendingIntent.FLAG_IMMUTABLE);
  Notification n=new Notification.Builder(c,CHANNEL).setSmallIcon(android.R.drawable.stat_sys_download_done).setContentTitle("LO: Saltlord Edition").setContentText("App running — return to app").setSubText("Running").setContentIntent(pi).setOnlyAlertOnce(true).setAutoCancel(true).build();manager.notify(ID,n);
 }
 static void attach(RuntimeReadinessActivity a){activity=new WeakReference<>(a);if(instance!=null)instance.show();}
 static void detach(RuntimeReadinessActivity a){if(activity.get()==a)activity.clear();}
 static void begin(RuntimeReadinessActivity a){attach(a);try{a.startForegroundService(new Intent(a,ShaderPreparationService.class));}catch(RuntimeException e){android.util.Log.e("LO.Preparation","Could not start background preparation service",e);}}
 public void onCreate(){super.onCreate();instance=this;chime.prepare(this);NotificationManager manager=getSystemService(NotificationManager.class);NotificationChannel channel=new NotificationChannel(CHANNEL,"Shader preparation",NotificationManager.IMPORTANCE_LOW);channel.setSound(null,null);manager.createNotificationChannel(channel);}
 public int onStartCommand(Intent intent,int flags,int id){
  if(ended){stopSelf();return START_NOT_STICKY;}
  if(Build.VERSION.SDK_INT>=29)startForeground(ID,notification(0,false),android.content.pm.ServiceInfo.FOREGROUND_SERVICE_TYPE_DATA_SYNC);else startForeground(ID,notification(0,false));
  if(wake==null){wake=getSystemService(PowerManager.class).newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,"LO:ShaderPreparation");wake.acquire(6*60*60*1000L);}
  RuntimeReadinessActivity.nativePreparationServiceEvent(1);handler.removeCallbacks(poll);handler.post(poll);return START_NOT_STICKY;
 }
 private Notification notification(long state,boolean complete){
  int done=(int)(state&0xfffffffL),total=(int)((state>>>28)&0xfffffffL),stage=(int)((state>>>56)&15),unit=(int)(state>>>60);
  String[] stages={"Preparing shaders","Preparing pipelines","Checking shader cache","Extracting shaders","Scanning game files","Loading cached shaders"};String[] units={"shaders","pipelines","files","MiB","entries"};
  Intent open=new Intent(this,RuntimeReadinessActivity.class).addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP|Intent.FLAG_ACTIVITY_CLEAR_TOP);
  PendingIntent pi=PendingIntent.getActivity(this,0,open,PendingIntent.FLAG_UPDATE_CURRENT|PendingIntent.FLAG_IMMUTABLE);
  String text=complete?"Preparation complete — return to the app":total>0?String.format(java.util.Locale.US,"%,d / %,d %s · %d%%",done,total,unit<units.length?units[unit]:"items",100L*Math.min(done,total)/total):hadWork?"Finishing preparation…":"Preparing game files…";
  Notification.Builder b=new Notification.Builder(this,CHANNEL).setSmallIcon(android.R.drawable.stat_sys_download).setContentTitle("LO: Saltlord Edition").setContentText(text).setSubText(complete?"Ready":stage<stages.length?stages[stage]:"Preparing resources").setContentIntent(pi).setOnlyAlertOnce(true).setOngoing(!complete).setAutoCancel(complete);
  if(!complete)b.setProgress(1000,total>0?(int)(1000L*Math.min(done,total)/total):0,total==0);return b.build();
 }
 private void show(){RuntimeReadinessActivity a=activity.get();if(a!=null&&!a.isDestroyed())a.preparationChanged(last<0?0:last,!ended&&hadWork);}
 private final Runnable poll=new Runnable(){public void run(){
  if(ended)return;
  try{long state=RuntimeReadinessActivity.nativePreparationProgress();boolean complete=RuntimeReadinessActivity.nativePreparationFinished();
   if(state!=0)hadWork=true;if(state!=last){last=state;getSystemService(NotificationManager.class).notify(ID,notification(state,false));}show();
   if(RuntimeReadinessActivity.nativePreparationStopped()){ended=true;show();stopForeground(STOP_FOREGROUND_REMOVE);stopSelf();return;}
   if(complete){RuntimeReadinessActivity.nativePreparationServiceEvent(2);ended=true;show();if(hadWork)chime.play();if(RuntimeReadinessActivity.nativeGuestStarted())running(ShaderPreparationService.this);else getSystemService(NotificationManager.class).notify(ID,notification(state,true));stopForeground(STOP_FOREGROUND_DETACH);stopSelf();return;}
   handler.postDelayed(this,1000);
  }catch(UnsatisfiedLinkError e){android.util.Log.e("LO.Preparation","Native progress unavailable",e);stopForeground(STOP_FOREGROUND_REMOVE);stopSelf();}
 }};
 @Override public void onTimeout(int startId,int type){RuntimeReadinessActivity.nativePreparationServiceEvent(3);ended=true;show();stopForeground(STOP_FOREGROUND_REMOVE);stopSelf();RuntimeReadinessActivity.nativeCancelImport();}
 public void onDestroy(){ended=true;show();handler.removeCallbacks(poll);if(wake!=null&&wake.isHeld())wake.release();wake=null;instance=null;handler.postDelayed(chime::close,1500);super.onDestroy();}
 public IBinder onBind(Intent intent){return null;}
}
