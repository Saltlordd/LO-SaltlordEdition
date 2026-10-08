package io.github.freefrank.lostodyssey;
import android.content.Context;
import android.media.AudioAttributes;
import android.media.SoundPool;
/** Wizard-only cue, routed through media volume alongside the game's audio. */
final class WizardChime {
 private SoundPool pool;private int sound,pending;private boolean ready,failed;
 private static final float VOLUME=.45f;
 void prepare(Context c){if(pool!=null)return;
  ready=false;failed=false;pending=0;
  pool=new SoundPool.Builder().setMaxStreams(2).setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build()).build();
  pool.setOnLoadCompleteListener((p,id,status)->{
   if(p!=pool||id!=sound)return;
   ready=status==0;failed=!ready;
   if(failed){pending=0;android.util.Log.w("LO.Wizard","Crystal cue failed to load: "+status);return;}
   int advances=pending;pending=0;
   for(int i=0;i<advances;i++)playReady();
  });
  try(android.content.res.AssetFileDescriptor fd=c.getAssets().openFd("audio/wizard_crystal.wav")){
   sound=pool.load(fd,1);failed=sound==0;
  }catch(java.io.IOException e){failed=true;android.util.Log.w("LO.Wizard","Chime unavailable",e);}
 }
 void play(){if(pool==null||failed)return;if(ready)playReady();else pending++;}
 private void playReady(){if(pool.play(sound,VOLUME,VOLUME,1,0,1f)==0)android.util.Log.w("LO.Wizard","Crystal cue could not start");}
 void close(){ready=false;pending=0;sound=0;failed=false;if(pool!=null){pool.release();pool=null;}}
}
