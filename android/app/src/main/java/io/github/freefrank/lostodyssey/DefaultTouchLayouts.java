package io.github.freefrank.lostodyssey;
import android.content.Context;
import android.content.SharedPreferences;
import org.json.JSONObject;
import java.io.InputStream;
import java.io.ByteArrayOutputStream;
import java.util.Iterator;
/** Supplied presets seed empty profiles only; never overwrite an edited or restored layout. */
final class DefaultTouchLayouts {
 static void seed(Context c,SharedPreferences prefs){
  boolean cover=!prefs.getBoolean("cover_initialized",false),inner=!prefs.getBoolean("inner_initialized",false);
  boolean legacy=prefs.contains("x_A");if(legacy||(!cover&&!inner))return;
  boolean fresh=prefs.getAll().isEmpty();
  try(InputStream in=c.getAssets().open("default_touch_layouts.json")){
   ByteArrayOutputStream bytes=new ByteArrayOutputStream();byte[] buffer=new byte[4096];int n;while((n=in.read(buffer))!=-1)bytes.write(buffer,0,n);
   JSONObject settings=new JSONObject(new String(bytes.toByteArray(),java.nio.charset.StandardCharsets.UTF_8)).getJSONObject("settings");
   SharedPreferences.Editor edit=prefs.edit();Iterator<String> keys=settings.keys();
   while(keys.hasNext()){
    String key=keys.next();boolean apply=key.startsWith("cover_")?cover:key.startsWith("inner_")?inner:fresh;
    if(!apply||prefs.contains(key))continue;Object v=settings.get(key);
    if(v instanceof Boolean)edit.putBoolean(key,(Boolean)v);
    else if(v instanceof Number){if(key.contains("_x_")||key.contains("_y_")||key.startsWith("x_")||key.startsWith("y_"))edit.putFloat(key,((Number)v).floatValue());else edit.putInt(key,((Number)v).intValue());}
   }
   edit.commit();
  }catch(Exception e){android.util.Log.e("LO.Touch","Could not read default layouts; using existing fallback",e);}
 }
}
