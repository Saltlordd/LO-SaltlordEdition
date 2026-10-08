package io.github.freefrank.lostodyssey;
import org.json.*;
import java.util.*;
import java.io.*;

/** Versioned, typed touch-only backup. Both profiles are materialised independently. */
final class OverlayBackup {
 static final String[] LABELS={"Options","Hide","L","R","D-pad","A","B","X","Y","LB","LT","RB","RT","Back","Start","L3","R3"};
 static final String[] PROFILES={"cover","inner","flex-bottom","flex-side"};
 static Map<String,Object> snapshot(Map<String,?> stored,Map<String,Float> defaults){
  Map<String,Object> result=new TreeMap<>();
  for(String k:stored.keySet())if(kind(k)!=null)result.put(k,normalise(k,stored.get(k)));
  for(String p:PROFILES){
   if(p.startsWith("flex-")&&!Boolean.TRUE.equals(stored.get(p+"_initialized")))continue;
   for(String label:LABELS){for(String axis:new String[]{"x_","y_"}){String k=p+"_"+axis+label;if(!result.containsKey(k))result.put(k,normalise(k,stored.containsKey(axis+label)?stored.get(axis+label):defaults.get(axis+label)));}
    String k=p+"_scale_"+label;if(!result.containsKey(k))result.put(k,100);}
   for(String k:new String[]{"size","opacity","left-mode","right-mode","stick-travel"}){String full=p+"_"+k;if(!result.containsKey(full))result.put(full,normalise(full,stored.containsKey(k)?stored.get(k):k.equals("size")?80:k.equals("opacity")?70:k.equals("stick-travel")?100:2));}
   for(String k:new String[]{"initialized","compact-actions-v22","zones-v21","opacity-custom-v44"})result.put(p+"_"+k,true);
  }
  return result;
 }
 static String kind(String key){
  if(Arrays.asList("haptics","stick-haptics","context-controls").contains(key))return "bool";
  if(Arrays.asList("profile-choice","auto-hide","size","opacity").contains(key))return "int";
  String k=key;for(String p:PROFILES)if(k.startsWith(p+"_")){k=k.substring(p.length()+1);if(Arrays.asList("initialized","compact-actions-v22","zones-v21","opacity-custom-v44").contains(k))return "bool";if(Arrays.asList("size","opacity","left-mode","right-mode","stick-travel").contains(k))return "int";break;}
  for(String label:LABELS){if(k.equals("x_"+label)||k.equals("y_"+label))return "float";if(k.equals("scale_"+label))return "int";}return null;
 }
 static Object normalise(String key,Object value){
  String type=kind(key);if(type==null)throw new IllegalArgumentException("Unsupported layout setting: "+key);
  if(type.equals("bool")){if(!(value instanceof Boolean))throw new IllegalArgumentException("Invalid boolean: "+key);return value;}
  if(!(value instanceof Number))throw new IllegalArgumentException("Invalid number: "+key);
  double n=((Number)value).doubleValue();if(!Double.isFinite(n))throw new IllegalArgumentException("Invalid number: "+key);
  if(type.equals("float")){if(n<0||n>1)throw new IllegalArgumentException("Position outside screen: "+key);return (float)n;}
  int low=0,high=2;if(key.equals("auto-hide"))high=30;
  else if(key.endsWith("size")){low=60;high=110;}
  else if(key.endsWith("opacity")){low=20;high=100;}
  else if(key.endsWith("stick-travel")){low=50;high=150;}
  else if(key.contains("scale_")){low=40;high=200;}
  if(n!=Math.rint(n)||n<low||n>high)throw new IllegalArgumentException("Setting outside supported range: "+key);return (int)n;
 }
 static byte[] encode(Map<String,Object> snapshot)throws JSONException {
  JSONObject root=new JSONObject();root.put("format","lo-saltlord-touch-layout");root.put("version",2);java.util.List<String> included=new ArrayList<>();for(String p:PROFILES)if(Boolean.TRUE.equals(snapshot.get(p+"_initialized")))included.add(p);root.put("profiles",new JSONArray(included));JSONObject settings=new JSONObject();for(String key:snapshot.keySet())settings.put(key,normalise(key,snapshot.get(key)));root.put("settings",settings);return root.toString(2).getBytes(java.nio.charset.StandardCharsets.UTF_8);
 }
 static Map<String,Object> decode(InputStream input)throws IOException,JSONException {
  ByteArrayOutputStream bytes=new ByteArrayOutputStream();byte[] buffer=new byte[8192];int n;while((n=input.read(buffer))!=-1){if(bytes.size()+n>1024*1024)throw new IOException("Layout backup exceeds 1 MB");bytes.write(buffer,0,n);}
  JSONObject root=new JSONObject(new String(bytes.toByteArray(),java.nio.charset.StandardCharsets.UTF_8));
  if(!"lo-saltlord-touch-layout".equals(root.getString("format"))||(!(root.get("version") instanceof Number)||(((Number)root.get("version")).doubleValue()!=1&&((Number)root.get("version")).doubleValue()!=2)))throw new IOException("Unsupported layout backup format/version");
  JSONArray profiles=root.getJSONArray("profiles");if(profiles.length()<2||profiles.length()>4||!profiles.getString(0).equals("cover")||!profiles.getString(1).equals("inner"))throw new IOException("Backup must contain the main touch layouts");
  Set<String> included=new HashSet<>();for(int i=0;i<profiles.length();i++){String p=profiles.getString(i);if(!Arrays.asList(PROFILES).contains(p)||!included.add(p))throw new IOException("Unsupported or duplicate layout profile");}
  JSONObject settings=root.getJSONObject("settings");if(settings.length()>512)throw new IOException("Too many layout settings");Map<String,Object> out=new TreeMap<>();Iterator<String> keys=settings.keys();while(keys.hasNext()){String key=keys.next();try{out.put(key,normalise(key,settings.get(key)));}catch(IllegalArgumentException e){throw new IOException(e.getMessage(),e);}}
  for(String p:included){if(!Boolean.TRUE.equals(out.get(p+"_initialized"))||!Boolean.TRUE.equals(out.get(p+"_zones-v21"))||!Boolean.TRUE.equals(out.get(p+"_compact-actions-v22")))throw new IOException("Incomplete profile metadata: "+p);
   for(String label:LABELS)for(String part:new String[]{"x_","y_","scale_"})if(!out.containsKey(p+"_"+part+label))throw new IOException("Missing control: "+p+" "+label);
   for(String k:new String[]{"size","opacity","left-mode","right-mode","stick-travel"})if(!out.containsKey(p+"_"+k))throw new IOException("Incomplete profile: "+p);}
  return out;
 }
}
