package io.github.freefrank.lostodyssey;
import android.content.*;
import android.os.*;
import android.view.*;
import android.widget.*;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import java.io.*;
import java.util.Locale;
import java.util.concurrent.*;
/** In-app only; one sample/second, no permission prompts, uploads or per-frame polling. */
final class PerformanceMonitor extends FrameLayout {
 private final RuntimeReadinessActivity app;private final TextView label;private final SharedPreferences prefs;
 private ScheduledExecutorService worker;private volatile int generation;private boolean running;
 private static final class SampleState {long lastTime=-1,lastCpu=-1,lastPresents=-1;boolean previouslyPaused;}
 private final String[] keys={"fps","cpu","gpu","thermal","battery","memory","render"};
 PerformanceMonitor(RuntimeReadinessActivity app){super(app);this.app=app;prefs=app.getSharedPreferences("performance-monitor",0);setClickable(false);setFocusable(false);
  label=new TextView(app);label.setTextColor(BrandUi.ACCENT);label.setTypeface(Typeface.MONOSPACE);label.setIncludeFontPadding(false);label.setClickable(false);label.setFocusable(false);label.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);int p=BrandUi.dp(app,7);label.setPadding(p,p,p,p);
  GradientDrawable bg=new GradientDrawable(GradientDrawable.Orientation.TL_BR,new int[]{0xe61c2a35,0xe60d151e});bg.setCornerRadius(BrandUi.dp(app,9));bg.setStroke(BrandUi.dp(app,1),0x80648196);label.setBackground(bg);addView(label,new FrameLayout.LayoutParams(-2,-2));setVisibility(GONE);
 }
 void setRunning(boolean value){running=value;refresh();}
 void refresh(){generation++;if(worker!=null){worker.shutdownNow();worker=null;}setVisibility(GONE);
  if(!running||!prefs.getBoolean("enabled",false)||!isAttachedToWindow())return;
  boolean any=false;for(String key:keys)any|=metric(key);if(!any)return;
  label.setAlpha(Math.max(0.1f,Math.min(1f,prefs.getInt("opacity",100)/100f)));
  label.setTextSize(new float[]{10,12,15}[PerformancePolicy.size(prefs.getInt("size",1))]);int ticket=generation;
  worker=Executors.newSingleThreadScheduledExecutor(r->{Thread t=new Thread(r,"LO performance monitor");t.setDaemon(true);return t;});
  SampleState state=new SampleState();worker.scheduleWithFixedDelay(()->sample(ticket,state),0,1,TimeUnit.SECONDS);
 }
 @Override protected void onAttachedToWindow(){super.onAttachedToWindow();refresh();}
 @Override protected void onDetachedFromWindow(){running=false;refresh();super.onDetachedFromWindow();}
 @Override protected void onMeasure(int ws,int hs){int w=MeasureSpec.getSize(ws),h=MeasureSpec.getSize(hs);label.setMaxWidth(Math.max(1,Math.min(BrandUi.dp(app,260),w-BrandUi.dp(app,20))));measureChild(label,MeasureSpec.makeMeasureSpec(w,MeasureSpec.AT_MOST),MeasureSpec.makeMeasureSpec(app.flexMode()!=0?h/2:h,MeasureSpec.AT_MOST));setMeasuredDimension(w,h);}
 @Override protected void onLayout(boolean change,int l,int t,int r,int b){int margin=BrandUi.dp(app,10),right=r-l-margin,top=margin;
  if(android.os.Build.VERSION.SDK_INT>=28&&getRootWindowInsets()!=null){android.view.DisplayCutout cut=getRootWindowInsets().getDisplayCutout();if(cut!=null){right-=cut.getSafeInsetRight();top+=cut.getSafeInsetTop();}}
  label.layout(Math.max(margin,right-label.getMeasuredWidth()),top,right,top+label.getMeasuredHeight());
 }
 private boolean metric(String key){return prefs.getBoolean(key,key.equals("fps"));}
 private void sample(int ticket,SampleState state){try{
  long[] nativeData=RuntimeReadinessActivity.nativePerformanceSnapshot();if(nativeData.length<5||nativeData[4]==0){state.lastTime=state.lastCpu=state.lastPresents=-1;post(()->{if(ticket==generation)setVisibility(GONE);});return;}
  long now=SystemClock.elapsedRealtime(),cpu=android.os.Process.getElapsedCpuTime(),elapsed=state.lastTime<0?0:now-state.lastTime;boolean paused=nativeData[3]!=0;
  double fps=PerformancePolicy.rate(state.lastPresents,nativeData[0],elapsed),cpuPercent=PerformancePolicy.cpu(state.lastCpu,cpu,elapsed);StringBuilder text=new StringBuilder();
  if(metric("fps"))row(text,paused?"FPS · Paused":state.previouslyPaused||!Double.isFinite(fps)?"FPS · Sampling…":String.format(Locale.US,"FPS · %.1f / %d",fps,nativeData[1]));
  if(metric("cpu"))row(text,String.format(Locale.US,"CPU app · %s",Double.isFinite(cpuPercent)?String.format(Locale.US,"%.0f%%",cpuPercent):"Sampling…"));
  if(metric("gpu")){double gpu=gpuLoad();row(text,"GPU device · "+(Double.isFinite(gpu)?String.format(Locale.US,"%.0f%%",gpu):"Unavailable"));}
  if(metric("thermal")){PowerManager power=(PowerManager)app.getSystemService(Context.POWER_SERVICE);int status=Build.VERSION.SDK_INT>=29&&power!=null?power.getCurrentThermalStatus():-1;row(text,"Thermal · "+PerformancePolicy.thermal(status));}
  if(metric("battery")){Intent battery=app.registerReceiver(null,new IntentFilter(Intent.ACTION_BATTERY_CHANGED));int temperature=battery==null?Integer.MIN_VALUE:battery.getIntExtra(BatteryManager.EXTRA_TEMPERATURE,Integer.MIN_VALUE);row(text,"Battery · "+(temperature!=Integer.MIN_VALUE?String.format(Locale.US,"%.1f°C",temperature/10.0):"Unavailable"));}
  if(metric("memory")){long rss=rssKb();row(text,"RAM app · "+(rss>=0?String.format(Locale.US,"%.0f MiB",rss/1024.0):"Unavailable"));}
  if(metric("render")){row(text,"Render · "+(nativeData[2]==0?"Follow output":nativeData[2]+"p"));}
  state.lastTime=now;state.lastCpu=cpu;state.lastPresents=nativeData[0];state.previouslyPaused=paused;String result=text.toString();
  post(()->{if(ticket!=generation||!running)return;label.setText(result);label.setContentDescription(result);setVisibility(result.isEmpty()?GONE:VISIBLE);requestLayout();});
 }catch(RuntimeException e){post(()->{if(ticket==generation){label.setText("Monitor · Unavailable");setVisibility(VISIBLE);}});}}
 private static void row(StringBuilder b,String line){if(b.length()>0)b.append('\n');b.append(line);}
 private static String readSmall(String name)throws IOException {try(Reader r=new FileReader(name)){char[] b=new char[8192];int count=r.read(b);return count>0?new String(b,0,count):"";}}
 private static double gpuLoad(){for(String path:new String[]{"/sys/class/kgsl/kgsl-3d0/gpu_busy_percentage"})try{double n=PerformancePolicy.percent(readSmall(path));if(Double.isFinite(n))return n;}catch(IOException|SecurityException e){}return Double.NaN;}
 private static long rssKb(){try{for(String line:readSmall("/proc/self/status").split("\n"))if(line.startsWith("VmRSS:"))return Long.parseLong(line.trim().split("\\s+")[1]);}catch(IOException|RuntimeException e){}return -1;}
 static void addOptions(RuntimeReadinessActivity app,LinearLayout panel){SharedPreferences prefs=app.getSharedPreferences("performance-monitor",0);
  Button master=new Button(app);Runnable refresh=()->master.setText("Performance monitor: "+(prefs.getBoolean("enabled",false)?"On":"Off"));master.setOnClickListener(v->{prefs.edit().putBoolean("enabled",!prefs.getBoolean("enabled",false)).apply();app.refreshMonitor();refresh.run();});refresh.run();panel.addView(master);
  Button size=new Button(app);String[] names={"Small","Medium","Large"};Runnable refreshSize=()->size.setText("Monitor size: "+names[PerformancePolicy.size(prefs.getInt("size",1))]);size.setOnClickListener(v->{prefs.edit().putInt("size",(PerformancePolicy.size(prefs.getInt("size",1))+1)%3).apply();app.refreshMonitor();refreshSize.run();});refreshSize.run();panel.addView(size);
  TextView opacityLabel=new TextView(app);opacityLabel.setTextColor(BrandUi.PRIMARY);panel.addView(opacityLabel);SeekBar opacity=new SeekBar(app);opacity.setMax(90);int initial=Math.max(10,Math.min(100,prefs.getInt("opacity",100)));opacity.setProgress(initial-10);opacityLabel.setText("Monitor opacity: "+initial+"%");opacity.setProgressTintList(android.content.res.ColorStateList.valueOf(BrandUi.ACCENT));panel.addView(opacity);opacity.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onProgressChanged(SeekBar b,int n,boolean user){opacityLabel.setText("Monitor opacity: "+(n+10)+"%");if(user){prefs.edit().putInt("opacity",n+10).apply();if(app.performanceMonitorForOptions()!=null)app.performanceMonitorForOptions().label.setAlpha((n+10)/100f);}}public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){}});
  String[] keys={"fps","cpu","gpu","thermal","battery","memory","render"},titles={"FPS / target","App CPU usage (100% = one core)","Device GPU usage (when available)","Android thermal status","Battery temperature","App RAM usage","Render resolution"};
  for(int i=0;i<keys.length;i++){String key=keys[i];android.widget.Switch toggle=new android.widget.Switch(app);toggle.setText(titles[i]);toggle.setTextColor(BrandUi.PRIMARY);toggle.setPadding(0,BrandUi.dp(app,10),0,BrandUi.dp(app,10));toggle.setChecked(prefs.getBoolean(key,key.equals("fps")));toggle.setOnCheckedChangeListener((b,on)->{prefs.edit().putBoolean(key,on).apply();app.refreshMonitor();});panel.addView(toggle);}
  BrandUi.help(panel,"Performance monitor","Updates once per second while gameplay is open. Each metric and the panel size are saved. FPS counts guest frontbuffer presentations, not measured physical display refreshes. CPU is this app's process: 100% means one fully busy CPU core, so multi-core use can exceed 100%. GPU load is device-wide and is shown only when a readable vendor counter exists; many phones block it. Android thermal status describes heat pressure, not a chip temperature. Battery temperature is not CPU/GPU temperature. RAM is resident process memory. Disabled or backgrounded monitors stop sampling. The monitor never records or sends these readings.");
 }
}
