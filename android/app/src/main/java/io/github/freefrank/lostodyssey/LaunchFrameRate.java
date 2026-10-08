package io.github.freefrank.lostodyssey;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;

/** Fill missing defaults without replacing a saved target before SDL_main reads settings; leave every other key intact. */
final class LaunchFrameRate {
    static String withSavedTarget(String settings) {
        if(java.util.regex.Pattern.compile("(?m)^frame_rate=").matcher(settings).find())return settings;
        return settings+(settings.isEmpty()||settings.endsWith("\n")?"":"\n")+"frame_rate=30\n";
    }
    static void prepare(File filesDir) throws IOException {
        File directory=new File(filesDir,"config"),settings=new File(directory,"settings.ini");
        if(!directory.isDirectory()&&!directory.mkdirs())throw new IOException("Cannot prepare launch settings");
        String previous=settings.exists()?new String(Files.readAllBytes(settings.toPath()),StandardCharsets.UTF_8):"";
        String updated=withSavedTarget(previous);
        // Missing key is a fresh/default configuration, not an explicit saved choice.
        if(!java.util.regex.Pattern.compile("(?m)^internal_resolution=").matcher(previous).find())
            updated+=(updated.endsWith("\n")?"":"\n")+"internal_resolution=720\n";
        if(previous.equals(updated))return;
        File temporary=File.createTempFile("launch-frame-rate-",".tmp",directory);
        try {
            try(java.io.FileOutputStream output=new java.io.FileOutputStream(temporary)) {
                output.write(updated.getBytes(StandardCharsets.UTF_8));output.getFD().sync();
            }
            Files.move(temporary.toPath(),settings.toPath(),StandardCopyOption.ATOMIC_MOVE,StandardCopyOption.REPLACE_EXISTING);
        } finally {temporary.delete();}
    }
}
