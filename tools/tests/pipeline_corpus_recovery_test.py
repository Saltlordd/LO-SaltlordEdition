#!/usr/bin/env python3
import importlib.util,pathlib,tempfile,subprocess
r=pathlib.Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('fixtures',r/'tools/tests/pipeline_warmup_seeds_test.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
with tempfile.TemporaryDirectory() as temp:
 p=pathlib.Path(temp);sources=[]
 for n,t in m.STUBS.items():
  q=p/n;q.parent.mkdir(parents=True,exist_ok=True);q.write_text(t);sources.append(str(q))
 sources.extend([str(r/'android/app/src/main/java/io/github/freefrank/lostodyssey/PipelineWarmupSeeds.java'),str(r/'tools/tests/PipelineCorpusRecoveryTest.java')])
 subprocess.run(['java','com.sun.tools.javac.Main','-d',temp,*sources],check=True)
 subprocess.run(['java','-cp',temp,'io.github.freefrank.lostodyssey.PipelineCorpusRecoveryTest',str(r/'android/app/src/main/assets/warmup/pipelines_corpus.bin')],check=True)
