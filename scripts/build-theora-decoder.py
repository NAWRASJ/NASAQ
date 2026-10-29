from pathlib import Path
import subprocess
import argparse
parser=argparse.ArgumentParser()
parser.add_argument('--sources',required=True);parser.add_argument('--dependency-prefix',required=True);parser.add_argument('--build-dir',required=True)
args=parser.parse_args();d=Path(args.sources).resolve();p=Path(args.dependency_prefix).resolve();b=Path(args.build_dir).resolve();b.mkdir(parents=True,exist_ok=True);src=d/'theora-1.2.0'

files='apiwrapper bitpack decapiwrapper decinfo decode dequant fragment huffdec idct info internal quant state'.split()
subprocess.run(['cl','/nologo','/MD','/O2','/c','/I'+str(src/'include'),'/I'+str(p/'include')]+[str(src/'lib'/(n+'.c')) for n in files],cwd=b,check=True)
subprocess.run(['lib','/nologo','/out:theoradec.lib']+[n+'.obj' for n in files],cwd=b,check=True)
(b/'config.h').write_text('#define VERSION "1.28.8"\n#define PACKAGE "gst-plugins-base"\n#define GST_PACKAGE_NAME "NASAQ format plugins"\n#define GST_PACKAGE_ORIGIN "https://github.com/NAWRASJ/NASAQ"\n')
g=d/'gst-plugins-base-1.28.8/ext/theora';s=(g/'gsttheora.c').read_text().replace('  ret |= GST_ELEMENT_REGISTER (theoraenc, plugin);','');(b/'gsttheora-decoder.c').write_text(s)
incs=[b,g,src/'include',p/'include',p/'include/gstreamer-1.0',p/'include/glib-2.0',p/'lib/glib-2.0/include']
subprocess.run(['cl','/nologo','/MD','/O2','/LD','/DHAVE_CONFIG_H','/DGST_USE_UNSTABLE_API']+['/I'+str(i) for i in incs]+[str(b/'gsttheora-decoder.c'),str(g/'gsttheoradec.c'),str(g/'gsttheoraparse.c'),'/link','/OUT:gsttheora.dll','/LIBPATH:'+str(p/'lib'),'theoradec.lib','gstvideo-1.0.lib','gsttag-1.0.lib','gstbase-1.0.lib','gstreamer-1.0.lib','gobject-2.0.lib','glib-2.0.lib','ogg.lib'],cwd=b,check=True)
