#!/usr/bin/env python3
import argparse,pathlib,subprocess,tempfile
from pipeline_warmup_seeds_test import STUBS
def main():
 p=argparse.ArgumentParser();p.add_argument('--java',default='java');a=p.parse_args()
 root=pathlib.Path(__file__).resolve().parents[2];package=root/'android/app/src/main/java/io/github/freefrank/lostodyssey'
 with tempfile.TemporaryDirectory(prefix='mobile-presentation-') as temp:
  folder=pathlib.Path(temp);sources=[]
  for name,text in STUBS.items():
   if name.endswith('/MobilePresentation.java'):continue
   target=folder/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(text);sources.append(str(target))
  sources += [str(package/(name+'.java')) for name in ['MobilePresentation','SavedGraphics']]
  sources += [str(root/'tools/tests'/name) for name in ['MobilePresentationTest.java','MobilePresentationDefaultsTest.java']]
  subprocess.run([a.java,'com.sun.tools.javac.Main','-d',temp,*sources],check=True)
  for name in ['MobilePresentationTest','MobilePresentationDefaultsTest']:
   subprocess.run([a.java,'-cp',temp,'io.github.freefrank.lostodyssey.'+name],check=True)
if __name__=='__main__':main()
