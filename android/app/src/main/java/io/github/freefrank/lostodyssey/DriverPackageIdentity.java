package io.github.freefrank.lostodyssey;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.security.*;
import java.util.*;

/** Identity of the validated loader inputs, independent of ZIP order and import directory. */
final class DriverPackageIdentity {
 static String fingerprint(File directory)throws IOException {
  MessageDigest hash;
  try{hash=MessageDigest.getInstance("SHA-256");}catch(NoSuchAlgorithmException e){throw new IOException(e);}
  File[] files=directory.listFiles(f->f.isFile()&&(f.getName().equals("meta.json")||f.getName().endsWith(".so")));
  if(files==null||files.length<2||files.length>64)throw new IOException("Incomplete driver package");
  Arrays.sort(files,Comparator.comparing(File::getName));
  long total=0;byte[] buffer=new byte[32768];
  for(File file:files){
   long length=file.length();total+=length;
   if(length==0||total>(257L<<20))throw new IOException("Invalid driver package size");
   hash.update((file.getName()+"\0"+length+"\0").getBytes(StandardCharsets.UTF_8));
   long read=0;try(InputStream in=new FileInputStream(file)){int n;while((n=in.read(buffer))!=-1){read+=n;if(read>length)throw new IOException("Driver changed while hashing");hash.update(buffer,0,n);}}
   if(read!=length)throw new IOException("Driver changed while hashing");
  }
  StringBuilder value=new StringBuilder();for(byte b:hash.digest())value.append(String.format(Locale.ROOT,"%02x",b&255));return value.toString();
 }
}
