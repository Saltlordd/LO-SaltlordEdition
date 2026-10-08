package io.github.freefrank.lostodyssey;
import java.io.*;import java.nio.file.*;import java.nio.charset.StandardCharsets;
/** Read all three choices before any JNI setter rewrites settings.ini. */
final class SavedGraphics {
 static int[] read(File root)throws IOException {String text=new String(Files.readAllBytes(new File(root,"config/settings.ini").toPath()),StandardCharsets.UTF_8);return parse(text);}
 static int[] parse(String text){int[] values={720,30,0};String[] keys={"internal_resolution","frame_rate","antialiasing"};for(String line:text.split("\n")){String[] pair=line.trim().split("=",2);if(pair.length!=2)continue;for(int i=0;i<keys.length;i++)if(pair[0].equals(keys[i]))try{int n=Integer.parseInt(pair[1].trim());if(i==0&&(n==720||n==1080)||i==1&&(n==30||n==60)||i==2&&(n>=0&&n<=2))values[i]=n;}catch(NumberFormatException ignored){}}return values;}
}
