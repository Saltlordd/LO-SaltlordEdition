package io.github.freefrank.lostodyssey;
import android.app.AlertDialog;
import android.content.Context;
import android.content.SharedPreferences;
import android.hardware.input.InputManager;
import android.os.Handler;
import android.os.Looper;
import android.widget.*;
import org.json.*;
import java.util.*;
/** Standard SDL inputs stay native; profiles replace only their logical mapping. */
final class ControllerProfiles {
 private static final Handler handler=new Handler(Looper.getMainLooper());
 private static final Map<RuntimeReadinessActivity,InputManager.InputDeviceListener> listeners=new HashMap<>();
 private static final Map<Integer,String> applied=new HashMap<>();
 private static SharedPreferences prefs(Context c){return c.getSharedPreferences("controller-profiles",0);}
 private static int[] decode(String json){try{JSONArray a=new JSONArray(json);int[] out=new int[a.length()];for(int i=0;i<out.length;i++)out[i]=a.getInt(i);return ControllerBindingPolicy.valid(out)?out:null;}catch(Exception e){return null;}}
 private static String encode(int[] a){JSONArray out=new JSONArray();for(int b:a)out.put(b);return out.toString();}
 static void attach(RuntimeReadinessActivity a){
  InputManager m=(InputManager)a.getSystemService(Context.INPUT_SERVICE);if(m==null||listeners.containsKey(a))return;
  InputManager.InputDeviceListener l=new InputManager.InputDeviceListener(){public void onInputDeviceAdded(int id){applyConnected(a);}public void onInputDeviceChanged(int id){applyConnected(a);}public void onInputDeviceRemoved(int id){applyConnected(a);}};
  listeners.put(a,l);m.registerInputDeviceListener(l,handler);applyConnected(a);
 }
 static void detach(RuntimeReadinessActivity a){InputManager.InputDeviceListener l=listeners.remove(a);if(l!=null)((InputManager)a.getSystemService(Context.INPUT_SERVICE)).unregisterInputDeviceListener(l);applied.clear();}
 static void applyConnected(RuntimeReadinessActivity a){
  if(a.isDestroyed())return;String[] rows=RuntimeReadinessActivity.nativeControllers();Set<Integer> connected=new HashSet<>();
  for(String row:rows){String[] p=row.split("\n",3);if(p.length!=3)continue;int id=Integer.parseInt(p[0]);connected.add(id);String json=prefs(a).getString("device:"+p[1],encode(ControllerBindingPolicy.DEFAULTS));
   if(!json.equals(applied.get(id))){int[] bindings=decode(json);if(bindings==null)bindings=ControllerBindingPolicy.DEFAULTS.clone();if(RuntimeReadinessActivity.nativeControllerMapping(id,bindings))applied.put(id,json);}}
  applied.keySet().retainAll(connected);
 }
 static void show(RuntimeReadinessActivity a){
  applyConnected(a);String[] rows=RuntimeReadinessActivity.nativeControllers();if(rows.length==0){AlertDialog d=new AlertDialog.Builder(a).setTitle("Controllers & profiles").setMessage("Connect a gamepad, then reopen this page. Controllers recognised by SDL appear here; ordinary keyboards do not.").setPositiveButton("Back",null).create();d.show();BrandUi.finishDialog(d);return;}
  String[] names=new String[rows.length];for(int i=0;i<rows.length;i++){String[] p=rows[i].split("\n",3);names[i]=p[2]+" — connected "+(i+1);}
  AlertDialog d=new AlertDialog.Builder(a).setTitle("Choose a connected controller").setItems(names,(dialog,which)->edit(a,rows[which])).setNegativeButton("Back",null).create();d.show();BrandUi.finishDialog(d);
 }
 private static String glyph(int row){if(row<4)return ControllerBindingPolicy.TARGETS[row];if(row==4)return "LB";if(row==5)return "RB";if(row==6)return "Back";if(row==7)return "Start";if(row==8)return "L3";if(row==9)return "R3";if(row<14)return "D-pad";if(row==14)return "LT";if(row==15)return "RT";return "Stick";}
 private static final class ControlIcon extends android.graphics.drawable.Drawable{
  final OverlayGlyphs art;final String label;final android.graphics.Paint paint=new android.graphics.Paint(3);final int size;final int direction;
  ControlIcon(Context c,OverlayGlyphs art,String label,int row){this.art=art;this.label=label;size=BrandUi.dp(c,44);direction=row>=16?new int[]{2,3,0,1}[(row-16)%4]:row>=10&&row<14?row-10:-1;}
  public void draw(android.graphics.Canvas c){android.graphics.Rect b=getBounds();float scale=b.height()/(float)size;art.draw(c,label,new android.graphics.RectF(b.left,b.top,b.left+size*scale,b.bottom),paint);if(direction>=0){paint.setColor(android.graphics.Color.WHITE);paint.setStyle(android.graphics.Paint.Style.STROKE);paint.setStrokeWidth(2*scale);paint.setStrokeCap(android.graphics.Paint.Cap.ROUND);float x=b.left+size*scale*1.28f,y=b.exactCenterY(),r=size*scale*.14f;c.save();c.rotate(new int[]{-90,90,180,0}[direction],x,y);c.drawLine(x-r,y,x+r,y,paint);c.drawLine(x+r,y,x+r-r*.65f,y-r*.65f,paint);c.drawLine(x+r,y,x+r-r*.65f,y+r*.65f,paint);c.restore();paint.setStyle(android.graphics.Paint.Style.FILL);}}public int getIntrinsicWidth(){return direction>=0?Math.round(size*1.6f):size;}public int getIntrinsicHeight(){return size;}public void setAlpha(int a){paint.setAlpha(a);}public void setColorFilter(android.graphics.ColorFilter f){paint.setColorFilter(f);}public int getOpacity(){return android.graphics.PixelFormat.TRANSLUCENT;}
 }
 private static void edit(RuntimeReadinessActivity a,String row){
  RuntimeReadinessActivity.nativeControllerConfig(true);
  String[] device=row.split("\n",3);int id=Integer.parseInt(device[0]);String key="device:"+device[1];int[] saved=decode(prefs(a).getString(key,""));final int[] bindings=saved==null?ControllerBindingPolicy.DEFAULTS.clone():saved;OverlayGlyphs art=new OverlayGlyphs(a);
  LinearLayout panel=new LinearLayout(a);panel.setOrientation(1);int pad=BrandUi.dp(a,16);panel.setPadding(pad,pad,pad,pad);
  TextView help=new TextView(a);help.setText("Tap a game control, release the controller, then press or move the input you want to use. Changes apply immediately. Save a named profile to reuse this layout. Named profiles are stored on this device and removed if the app is uninstalled.\n\nControllers are remembered by model and serial when available. Identical controllers without serials share a default mapping; load a named profile to choose a different layout for this session.");panel.addView(help);
  Button[] buttons=new Button[24];Runnable refresh=()->{for(int i=0;i<24;i++)buttons[i].setText(ControllerBindingPolicy.TARGETS[i]+"\n"+ControllerBindingPolicy.source(bindings[i]));};
  Runnable apply=()->{if(!RuntimeReadinessActivity.nativeControllerMapping(id,bindings)){Toast.makeText(a,"Controller disconnected. Reconnect and choose it again.",Toast.LENGTH_LONG).show();return;}String json=encode(bindings);prefs(a).edit().putString(key,json).commit();applied.put(id,json);};
  for(int i=0;i<24;i++){final int at=i;Button b=new Button(a);buttons[i]=b;b.setCompoundDrawablesWithIntrinsicBounds(new ControlIcon(a,art,glyph(i),i),null,null,null);b.setCompoundDrawablePadding(BrandUi.dp(a,10));b.setOnClickListener(v->capture(a,id,at,bindings,()->{apply.run();refresh.run();}));panel.addView(b);}refresh.run();
  Button save=new Button(a);save.setText("Save named profile");save.setOnClickListener(v->{EditText name=new EditText(a);name.setSingleLine(true);name.setHint("Profile name");AlertDialog d=new AlertDialog.Builder(a).setTitle("Save controller profile").setView(name).setNegativeButton("Cancel",null).setPositiveButton("Save",null).create();d.show();BrandUi.finishDialog(d);d.getButton(-1).setOnClickListener(x->{String n=name.getText().toString().trim();if(n.isEmpty()||n.length()>80){name.setError("Use a name of 1–80 characters");return;}Runnable write=()->{prefs(a).edit().putString("profile:"+n,encode(bindings)).commit();Toast.makeText(a,"Controller profile saved",Toast.LENGTH_SHORT).show();d.dismiss();};if(prefs(a).contains("profile:"+n)){AlertDialog confirm=new AlertDialog.Builder(a).setTitle("Replace profile?").setMessage("A profile named "+n+" already exists.").setNegativeButton("Cancel",null).setPositiveButton("Replace",(q,w)->write.run()).create();confirm.show();BrandUi.finishDialog(confirm);}else write.run();});});panel.addView(save);
  Button load=new Button(a);load.setText("Load named profile");load.setOnClickListener(v->{ArrayList<String> names=new ArrayList<>();for(String k:prefs(a).getAll().keySet())if(k.startsWith("profile:"))names.add(k.substring(8));Collections.sort(names);if(names.isEmpty()){Toast.makeText(a,"No saved controller profiles yet",Toast.LENGTH_SHORT).show();return;}AlertDialog d=new AlertDialog.Builder(a).setTitle("Load controller profile").setItems(names.toArray(new String[0]),(q,at)->{int[] b=decode(prefs(a).getString("profile:"+names.get(at),""));if(b==null){Toast.makeText(a,"This profile is invalid",Toast.LENGTH_SHORT).show();return;}System.arraycopy(b,0,bindings,0,24);apply.run();refresh.run();}).setNegativeButton("Cancel",null).create();d.show();BrandUi.finishDialog(d);});panel.addView(load);
  Button reset=new Button(a);reset.setText("Restore standard mapping");reset.setOnClickListener(v->{AlertDialog d=new AlertDialog.Builder(a).setTitle("Restore standard mapping?").setMessage("Resets this controller's current mapping. Named profiles are kept.").setNegativeButton("Cancel",null).setPositiveButton("Restore",(q,at)->{System.arraycopy(ControllerBindingPolicy.DEFAULTS,0,bindings,0,24);apply.run();refresh.run();}).create();d.show();BrandUi.finishDialog(d);});panel.addView(reset);
  ScrollView scroll=new ScrollView(a);scroll.addView(panel);ControllerCaptureDialog d=new ControllerCaptureDialog(a);d.setTitle(device[2]);d.setView(BrandUi.menu(a,scroll));d.setButton(AlertDialog.BUTTON_POSITIVE,"Back",(android.content.DialogInterface.OnClickListener)null);d.setCancelable(false);d.setOnDismissListener(q->RuntimeReadinessActivity.nativeControllerConfig(false));d.show();BrandUi.finishDialog(d);d.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{if(ControllerBindingPolicy.neutral(RuntimeReadinessActivity.nativeControllerState(id)))d.dismiss();else Toast.makeText(a,"Release the controller buttons and sticks before closing.",Toast.LENGTH_SHORT).show();});
 }
 private static void capture(RuntimeReadinessActivity a,int id,int target,int[] bindings,Runnable changed){
  TextView status=new TextView(a);status.setPadding(BrandUi.dp(a,20),BrandUi.dp(a,16),BrandUi.dp(a,20),BrandUi.dp(a,16));status.setText("Release all controller buttons and sticks…");
  ControllerCaptureDialog d=new ControllerCaptureDialog(a);d.setTitle("Set "+ControllerBindingPolicy.TARGETS[target]);d.setView(status);d.setButton(AlertDialog.BUTTON_NEGATIVE,"Cancel",(android.content.DialogInterface.OnClickListener)null);d.setButton(AlertDialog.BUTTON_NEUTRAL,"Unbind",(q,w)->{bindings[target]=-1;changed.run();});
  Runnable poll=new Runnable(){long neutralAt=-1;boolean armed=false;int captured=-1;public void run(){if(!d.isShowing()||a.isDestroyed())return;int[] state=RuntimeReadinessActivity.nativeControllerState(id);if(state.length!=27){status.setText("Controller disconnected. Cancel and reconnect.");return;}int b=ControllerBindingPolicy.pressed(state);long now=android.os.SystemClock.uptimeMillis();if(captured!=-1){if(ControllerBindingPolicy.neutral(state)){if(neutralAt<0)neutralAt=now;if(now-neutralAt>=300){bindings[target]=captured;changed.run();d.dismiss();return;}}else neutralAt=-1;}else if(!armed){if(ControllerBindingPolicy.neutral(state)){if(neutralAt<0)neutralAt=now;if(now-neutralAt>=300){armed=true;status.setText("Press a button or move the stick/trigger now.");}}else neutralAt=-1;}else if(b!=-1){captured=b;neutralAt=-1;status.setText("Captured "+ControllerBindingPolicy.source(b)+". Release the buttons and sticks to finish.");}handler.postDelayed(this,40);}};
  d.setOnDismissListener(q->handler.removeCallbacks(poll));d.show();BrandUi.finishDialog(d);handler.post(poll);
 }
}
