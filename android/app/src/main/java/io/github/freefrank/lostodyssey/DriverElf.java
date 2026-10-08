package io.github.freefrank.lostodyssey;
import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
/** Validate the imported library before native code attempts to load it. */
final class DriverElf {
    static boolean validEntryName(String name){return name!=null&&!name.isEmpty()&&name.length()<=255&&!name.startsWith(".")&&!name.contains("/")&&!name.contains("\\")&&name.indexOf(0)<0;}
    static void validate(File file,long pageSize) throws IOException {
        try(RandomAccessFile f=new RandomAccessFile(file,"r")){
            long length=f.length();if(length<64)throw new IOException("Driver library is truncated");
            byte[] bytes=new byte[64];f.readFully(bytes);ByteBuffer h=ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN);
            if(h.getInt(0)!=0x464c457f||bytes[4]!=2||bytes[5]!=1||bytes[6]!=1||h.getShort(16)!=3||h.getShort(18)!=183)
                throw new IOException("Driver must be an ARM64 ELF shared library");
            long offset=h.getLong(32);int size=h.getShort(54)&65535,count=h.getShort(56)&65535;
            if(offset<64||size<56||count<1||count>4096||offset>length||(long)size*count>length-offset)throw new IOException("Invalid driver ELF program headers");
            boolean load=false;long page=Math.max(4096,pageSize);
            for(int i=0;i<count;i++){
                f.seek(offset+(long)i*size);byte[] data=new byte[56];f.readFully(data);ByteBuffer p=ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN);
                if(p.getInt(0)!=1)continue;load=true;
                long at=p.getLong(8),address=p.getLong(16),fileSize=p.getLong(32),memory=p.getLong(40),align=p.getLong(48);
                if(at<0||address<0||fileSize<0||memory<fileSize||at>length||fileSize>length-at||align<page||(align&(align-1))!=0||at%page!=address%page)
                    throw new IOException("Driver load segment is invalid or incompatible with this phone's "+page+"-byte pages");
            }
            if(!load)throw new IOException("Driver has no load segments");
        }
    }
}
