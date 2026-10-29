from pathlib import Path
import argparse, subprocess, os, json, tarfile, hashlib, re, shutil, sys
p=argparse.ArgumentParser(description='Build NASAQ format plugins in a Visual Studio developer shell')
p.add_argument('--dependency-prefix',required=True);p.add_argument('--ucrt-prefix',required=True);p.add_argument('--source-archives',required=True);p.add_argument('--build-dir',required=True);p.add_argument('--output',required=True)
a=p.parse_args();dep=Path(a.dependency_prefix).resolve();ucrt=Path(a.ucrt_prefix).resolve();archives=Path(a.source_archives).resolve();b=Path(a.build_dir).resolve();out=Path(a.output).resolve();meta=Path(__file__).resolve().parents[1]/'packaging/media-codecs'
b.mkdir(parents=True,exist_ok=True);out.mkdir(parents=True,exist_ok=True);src=b/'sources';src.mkdir(exist_ok=True)
def run(args,**kw):subprocess.run([str(x) for x in args],check=True,**kw)
for entry in json.loads((meta/'sources.json').read_text()):
 f=archives/entry['file'];assert hashlib.sha256(f.read_bytes()).hexdigest()==entry['sha256'],f
 with tarfile.open(f) as t:t.extractall(src,filter='data')
for name,patch in json.loads((meta/'source-patches.json').read_text()).items():
 f=src/name;assert hashlib.sha256(f.read_text(encoding='utf-8-sig').replace('\r\n','\n').encode()).hexdigest()==patch['original_sha256'],f
 f.write_text(patch['text'],encoding='utf-8')
# libaom exposes a stable C ABI; all GStreamer plugins use the application's MSVC prefix.
aom=b/'aom-prefix'
for part in ['include','bin','lib/pkgconfig']:(aom/part).mkdir(parents=True,exist_ok=True)
shutil.copytree(ucrt/'include/aom',aom/'include/aom',dirs_exist_ok=True)
for name in ['libaom.dll','libwinpthread-1.dll']:shutil.copy2(ucrt/'bin'/name,aom/'bin'/name)
exports=subprocess.check_output(['dumpbin','/exports',str(aom/'bin/libaom.dll')],text=True)
names=re.findall(r'^\s+\d+\s+[0-9A-F]+\s+[0-9A-F]+\s+(aom_\w+)',exports,re.M);assert len(names)>10
(aom/'lib/aom.def').write_text('LIBRARY libaom.dll\nEXPORTS\n'+'\n'.join(n+(' DATA' if n.endswith('_algo') else '') for n in names))
run(['lib','/nologo','/def:'+str(aom/'lib/aom.def'),'/machine:x64','/out:'+str(aom/'lib/aom.lib')])
(aom/'lib/pkgconfig/aom.pc').write_text((ucrt/'lib/pkgconfig/aom.pc').read_text().replace('prefix=/ucrt64','prefix='+aom.as_posix()))
assert 'Version: 3.15.2' in (aom/'lib/pkgconfig/aom.pc').read_text(),'Use the tested libaom 3.15.2 headers and DLL'
env=os.environ.copy();env['PATH']=str(dep/'bin')+';'+env['PATH'];env['PKG_CONFIG_PATH']=str(aom/'lib/pkgconfig')+';'+str(dep/'lib/pkgconfig')
def meson(kind,opts,targets):
 build=b/(kind+'-build');source=src/('gst-'+('libav' if kind=='libav' else 'plugins-'+kind)+'-1.28.8')
 cmd=[sys.executable,'-m','mesonbuild.mesonmain','setup']
 if (build/'meson-private/coredata.dat').exists():cmd+=['--reconfigure']
 run(cmd+[build,source,'--buildtype=release','--wrap-mode=nofallback','-Dauto_features=disabled','-Dpkg_config_path='+str(aom/'lib/pkgconfig')+','+str(dep/'lib/pkgconfig')]+opts,env=env)
 run(['ninja','-j','2','-C',build]+targets,env=env)
meson('good',['-Davi=enabled','-Dflv=enabled','-Dauparse=enabled'],[])
meson('bad',['-Dmpegdemux=enabled','-Dvideoparsers=enabled','-Daom=enabled','-Dc_args=/Zc:preprocessor'],['gst/mpegdemux/gstmpegpsdemux.dll','gst/videoparsers/gstvideoparsersbad.dll','ext/aom/gstaom.dll'])
meson('libav',[],[])
run([sys.executable,Path(__file__).with_name('build-theora-decoder.py'),'--sources',src,'--dependency-prefix',dep,'--build-dir',b/'theora-native'],env=env)
plugin_paths=['good-build/gst/avi/gstavi.dll','good-build/gst/flv/gstflv.dll','good-build/gst/auparse/gstauparse.dll','bad-build/gst/mpegdemux/gstmpegpsdemux.dll','bad-build/gst/videoparsers/gstvideoparsersbad.dll','bad-build/ext/aom/gstaom.dll','libav-build/ext/libav/gstlibav.dll','theora-native/gsttheora.dll']
for rel in plugin_paths:shutil.copy2(b/rel,out/Path(rel).name)
for name in ['libaom.dll','libwinpthread-1.dll']:shutil.copy2(aom/'bin'/name,out/name)
manifest={'gstreamer_version':'1.28.8','libaom_version':'3.15.2','files':{f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(out.glob('*.dll'))}}
(out/'format-runtime.json').write_text(json.dumps(manifest,indent=2))
print('Built and hashed',len(manifest['files']),'format runtime libraries')

licenses=out/'licenses';licenses.mkdir(exist_ok=True)
for name in ['aom','libwinpthread','winpthreads']:
 original=ucrt/'share/licenses'/name
 if original.exists():shutil.copytree(original,licenses/name,dirs_exist_ok=True)
shutil.copy2(src/'theora-1.2.0/COPYING',licenses/'theora-COPYING')
shutil.copy2(src/'gst-plugins-base-1.28.8/COPYING',licenses/'GStreamer-COPYING')
shutil.copy2(meta/'sources.json',out/'sources.json')
shutil.copy2(meta/'source-patches.json',out/'source-patches.json')
