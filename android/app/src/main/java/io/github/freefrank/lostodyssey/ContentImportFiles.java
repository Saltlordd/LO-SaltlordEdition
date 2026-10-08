package io.github.freefrank.lostodyssey;
import java.io.*;
import java.util.zip.*;

/** Copy-only staging for SAF inputs. ZIP entry names never become output paths. */
final class ContentImportFiles {
 interface Progress {void bytes(long count);}
 private static final long PACKAGE_LIMIT=512L*1024*1024,ISO_LIMIT=9L*1024*1024*1024;
 static boolean packageMagic(byte[] b){return b.length>=4&&((b[0]=='L'&&b[1]=='I'&&b[2]=='V'&&b[3]=='E')||(b[0]=='P'&&b[1]=='I'&&b[2]=='R'&&b[3]=='S')||(b[0]=='C'&&b[1]=='O'&&b[2]=='N'&&b[3]==' '));}
 static int copy(InputStream input,String name,File directory,int start,Progress progress)throws IOException {
  BufferedInputStream in=new BufferedInputStream(input);in.mark(4);byte[] magic=new byte[4];int n=0,k;while(n<4&&(k=in.read(magic,n,4-n))>0)n+=k;in.reset();
  if(n==4&&magic[0]=='P'&&magic[1]=='K'){
   int count=0;long[] total={0};
   try(ZipInputStream zip=new ZipInputStream(in)){
    ZipEntry entry;while((entry=zip.getNextEntry())!=null){
     if(entry.isDirectory())continue;
     if(++count>64)throw new IOException("Too many packages in DLC ZIP");
     File out=new File(directory,"package-"+(start+count-1)+".stfs");
     copyStream(zip,out,PACKAGE_LIMIT-total[0],bytes->{total[0]+=bytes;progress.bytes(bytes);},true);
     zip.closeEntry();
    }
   }
   if(count==0)throw new IOException("DLC ZIP contains no packages");return count;
  }
  if(packageMagic(magic)){copyStream(in,new File(directory,"package-"+start+".stfs"),PACKAGE_LIMIT,progress,true);return 1;}
  if(name.toLowerCase(java.util.Locale.ROOT).endsWith(".iso")){copyStream(in,new File(directory,"disc-"+start+".iso"),ISO_LIMIT,progress,false);return 1;}
  throw new IOException("Choose a Lost Odyssey ISO, original DLC package, or ZIP containing DLC packages");
 }
 private static void copyStream(InputStream in,File file,long limit,Progress progress,boolean stfs)throws IOException {
  if(limit<=0)throw new IOException("DLC ZIP exceeds size limit");
  long total=0;byte[] buffer=new byte[65536];
  try(FileOutputStream out=new FileOutputStream(file)){
   int n;while((n=in.read(buffer))!=-1){if(n==0)continue;total+=n;if(total>limit)throw new IOException("Selected content exceeds size limit");out.write(buffer,0,n);progress.bytes(n);}
   out.getFD().sync();
  }catch(IOException e){file.delete();throw e;}
  if(total==0){file.delete();throw new IOException("Selected file is empty");}
  if(stfs){try(FileInputStream check=new FileInputStream(file)){byte[] b=new byte[4];if(check.read(b)!=4||!packageMagic(b))throw new IOException("DLC ZIP must contain original Xbox Marketplace packages, not extracted game files");}}
 }
 static void remove(File root){File[] children=root.listFiles();if(children!=null)for(File f:children)remove(f);root.delete();}
}
