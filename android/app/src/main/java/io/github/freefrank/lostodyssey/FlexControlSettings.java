package io.github.freefrank.lostodyssey;
import android.content.Context;
import android.content.SharedPreferences;
import java.util.Map;
final class FlexControlSettings {
 static boolean layoutKey(String key){return key.startsWith("x_")||key.startsWith("y_")||key.startsWith("scale_")||key.equals("size")||key.equals("left-mode")||key.equals("right-mode")||key.equals("stick-travel");}
 static boolean copyCover(Context c,int mode){
  if(FlexLayoutPolicy.valid(mode)==0)return false;
  SharedPreferences prefs=c.getSharedPreferences("touch-options",0);DefaultTouchLayouts.seed(c,prefs);String target=FlexLayoutPolicy.profile(mode)+"_";SharedPreferences.Editor edit=prefs.edit();
  for(Map.Entry<String,?> entry:prefs.getAll().entrySet())if(entry.getKey().startsWith("cover_")&&layoutKey(entry.getKey().substring(6))){String key=target+entry.getKey().substring(6);Object v=entry.getValue();if(v instanceof Float)edit.putFloat(key,(Float)v);else if(v instanceof Integer)edit.putInt(key,(Integer)v);}
  if(!prefs.getBoolean(target+"opacity-custom-v44",false))edit.putInt(target+"opacity",100);
  edit.putBoolean(target+"opacity-custom-v44",true).putBoolean(target+"initialized",true).putBoolean(target+"zones-v21",true).putBoolean(target+"compact-actions-v22",true);return edit.commit();
 }
}
