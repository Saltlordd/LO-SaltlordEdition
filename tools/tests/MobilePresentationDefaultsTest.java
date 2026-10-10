package io.github.freefrank.lostodyssey;
import android.content.Context;
import java.nio.file.*;
public final class MobilePresentationDefaultsTest {
 static void check(boolean value){if(!value)throw new AssertionError();}
 public static void main(String[] args)throws Exception{
  Context fresh=new Context(Files.createTempDirectory("fresh-mobile").toFile(),new byte[0]);
  MobilePresentation.prepare(fresh);check(MobilePresentation.rgb(fresh)==1&&MobilePresentation.scaling(fresh)==0);
  Context upgrade=new Context(Files.createTempDirectory("upgrade-mobile").toFile(),new byte[0]);
  MobilePresentation.prefs(upgrade).edit().putInt("scaling",1).putInt("rgb",0).commit();
  MobilePresentation.prepare(upgrade);check(MobilePresentation.rgb(upgrade)==1&&MobilePresentation.scaling(upgrade)==0);
  MobilePresentation.choose(upgrade,"rgb",0);MobilePresentation.prepare(upgrade);check(MobilePresentation.rgb(upgrade)==0);
  System.out.println("PASS: fresh and upgrade Expanded defaults, fixed Basic output and later Standard override retained");
 }
}
