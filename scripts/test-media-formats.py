import os,subprocess,json,pathlib,time
import argparse
parser=argparse.ArgumentParser(description="Generate and decode 38 media fixtures using the staged NASAQ runtime")
parser.add_argument('--ffmpeg',required=True);parser.add_argument('--runtime',required=True);parser.add_argument('--output',required=True);parser.add_argument('--fixtures',required=True)
args=parser.parse_args();ff=pathlib.Path(args.ffmpeg).resolve();rt=pathlib.Path(args.runtime).resolve();out=pathlib.Path(args.output).resolve();fixtures=pathlib.Path(args.fixtures).resolve();out.mkdir(parents=True,exist_ok=True);fixtures.mkdir(parents=True,exist_ok=True)

env=os.environ.copy();env.update(PATH=str(rt)+';'+env['PATH'],GST_PLUGIN_PATH=str(rt/'gstreamer-plugins'),GST_PLUGIN_SYSTEM_PATH=str(rt/'gstreamer-plugins'),GST_REGISTRY=str(fixtures/'registry.bin'),GST_PLUGIN_SCANNER=str(rt/'gst-plugin-scanner.exe'))
videos=[('h264.mp4','libx264','aac'),('hevc.mp4','libx265','aac'),('av1.mkv','libaom-av1','libopus'),('h264.mkv','libx264','aac'),('vp8.webm','libvpx','libvorbis'),('vp9.webm','libvpx-vp9','libopus'),('mpeg4.avi','mpeg4','libmp3lame'),('h264.mov','libx264','aac'),('h264.m4v','libx264','aac'),('wmv.wmv','wmv2','wmav2'),('asf.asf','wmv2','wmav2'),('flv.flv','flv','libmp3lame'),('mpeg.mpg','mpeg2video','mp2'),('mpeg.mpeg','mpeg2video','mp2'),('h264.ts','libx264','aac'),('h264.m2ts','libx264','ac3'),('h264.mts','libx264','ac3'),('theora.ogv','libtheora','libvorbis'),('h263.3gp','h263','aac')]
audios=[('mp3','libmp3lame'),('wav','pcm_s16le'),('flac','flac'),('ogg','libvorbis'),('opus','libopus'),('m4a','aac'),('aac','aac'),('wma','wmav2'),('aiff','pcm_s16be'),('ac3','ac3'),('eac3','eac3'),('mp2','mp2'),('mka','flac'),('wv','wavpack'),('tta','tta'),('caf','alac'),('au','pcm_s16be'),('aif','pcm_s16be')]
cases=[(n,v,a,True) for n,v,a in videos]+[('audio.'+ext,None,c,False) for ext,c in audios]+[('alac.m4a',None,'alac',False)]
results=[]
for name,v,a,isvideo in cases:
 f=fixtures/name;cmd=[str(ff),'-hide_banner','-loglevel','error','-y']
 if isvideo:cmd+=['-f','lavfi','-i','testsrc2=size='+('176x144' if v=='h263' else '640x360')+':rate=25:duration=7']
 cmd+=['-f','lavfi','-i','sine=frequency=440:sample_rate=48000:duration=7']
 if v:cmd+=['-c:v',v,'-threads','2']
 if v in ['libx264','libx265']:cmd+=['-preset','ultrafast']
 if v=='libaom-av1':cmd+=['-cpu-used','8','-crf','40']
 cmd+=['-c:a',a]
 if f.suffix=='.m4v':cmd+=['-f','mp4']
 if f.suffix in ['.mts','.m2ts']:cmd+=['-f','mpegts']
 cmd+=[str(f)]
 if not f.exists():
  p=subprocess.run(cmd,capture_output=True,timeout=120)
  if p.returncode:results.append(dict(file=name,generation_error=p.stderr.decode(errors='replace')));continue
 pipe='filesrc location="'+f.as_posix()+'" ! decodebin name=d d. ! queue ! audioconvert ! fakesink sync=false'
 if isvideo:pipe+=' d. ! queue ! video/x-raw ! fakesink sync=false'
 p=subprocess.run([str(rt/'nasaq_codec_probe.exe'),pipe,'15'],env=env,cwd=rt,capture_output=True,timeout=20)
 row=dict(file=name,video=isvideo,video_codec=v,audio_codec=a,exit=p.returncode,output=(p.stdout+p.stderr).decode(errors='replace')[-3000:]);results.append(row);print(name,p.returncode,flush=True)
 (out/'decode-results.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
(out/'decode-results.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
