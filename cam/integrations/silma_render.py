"""Optional offline asset preparation. Run in a separately provisioned SILMA environment.
No model download or inference has been validated by the CAM build.
SPDX-License-Identifier: GPL-3.0-or-later
"""
import argparse
from pathlib import Path

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--reference', required=True)
    p.add_argument('--reference-text', required=True)
    p.add_argument('--text-file', required=True)
    p.add_argument('--output', required=True)
    a = p.parse_args()
    if not Path(a.reference).is_file():
        p.error('Reference audio does not exist')
    if Path(a.output).exists():
        p.error('Output already exists')
    from silma_tts.api import SilmaTTS
    SilmaTTS().infer(ref_file=a.reference, ref_text=a.reference_text,
                    gen_text=Path(a.text_file).read_text(encoding='utf-8'),
                    file_wave=a.output, seed=None, speed=1)
    if not Path(a.output).is_file():
        raise RuntimeError('SILMA did not create output')

if __name__ == '__main__':
    main()
