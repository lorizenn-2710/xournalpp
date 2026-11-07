#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Local-only formula OCR worker. stdout is an INI protocol; never log crops.
The parent bounds wall time and kills this process on cancellation. No network
is needed: every model path is verified before creating the inference engine.
"""
import base64, configparser, io, os, pathlib, resource, sys, time
os.environ['OMP_NUM_THREADS']='2'
os.environ['OPENBLAS_NUM_THREADS']='2'
resource.setrlimit(resource.RLIMIT_CPU, (50, 55))
resource.setrlimit(resource.RLIMIT_AS, (4*1024**3, 4*1024**3))
resource.setrlimit(resource.RLIMIT_FSIZE, (8*1024**2, 8*1024**2))

def main():
    out=configparser.ConfigParser()
    out['result']={'version':'1','code':'error','count':'0'}
    try:
        import rapid_latex_ocr
        from rapid_latex_ocr import LaTeXOCR
        import rapid_latex_ocr.utils_load as utils
        from PIL import Image
        import numpy as np
        import cv2
        cv2.setNumThreads(2)
        models=pathlib.Path(os.environ.get('XOURNALPP_FORMULA_MODELS',str(pathlib.Path(rapid_latex_ocr.__file__).parent/'models')))
        names=['image_resizer.onnx','encoder.onnx','decoder.onnx','tokenizer.json']
        if any(not (models/n).is_file() for n in names):
            out['result']['code']='missing-model';raise RuntimeError('Thiếu model OCR. Chạy scripts/teaching-tools/setup-formula-ocr.sh trong mã nguồn.')
        original=utils.OrtInferSession._init_sess_opt
        def limited(self):
            self.num_threads=2;original(self)
        utils.OrtInferSession._init_sess_opt=limited
        image=Image.open(sys.argv[1]).convert('RGB')
        if image.width*image.height>8_000_000:raise ValueError('Ảnh quá lớn. Chọn vùng nhỏ hơn.')
        gray=np.asarray(image.convert('L'))
        mask=gray<190
        ys,xs=np.where(mask)
        if len(xs)<15:raise ValueError('Vùng trống hoặc quá mờ. Chọn lại vùng rõ hơn.')
        clipped=bool(xs.min()<2 or ys.min()<2 or xs.max()>image.width-3 or ys.max()>image.height-3)
        image=image.crop((max(0,int(xs.min())-12),max(0,int(ys.min())-12),min(image.width,int(xs.max())+13),min(image.height,int(ys.max())+13)))
        start=time.monotonic()
        engine=LaTeXOCR(**dict(zip(['image_resizer_path','encoder_path','decoder_path','tokenizer_json'],[models/n for n in names])))
        count=0
        truncated=False
        original_rows=np.where((np.asarray(image.convert('L'))<190).any(axis=1))[0]
        horizontal_lines=bool(np.any(np.diff(original_rows)>max(24,image.height*.09)))
        # A tall crop may contain several horizontal formulas. Detect their
        # whitespace gutters before treating aspect ratio as a rotation hint.
        orientations=[(0,image)] if image.height<image.width*1.3 or horizontal_lines else [(90,image.rotate(90,expand=True)),(270,image.rotate(270,expand=True))]
        for rotation,oriented in orientations:
            # Split ONLY at wide empty horizontal gutters. Fraction/root
            # components stay together. Multi-column prose requires recrop.
            arr=np.asarray(oriented.convert('L'))<190
            rows=np.where(arr.any(axis=1))[0]
            gutters=np.where(np.diff(rows)>max(24,oriented.height*.09))[0]
            spans=[];line_start=int(rows[0])
            for g in gutters:
                spans.append((line_start,int(rows[g])+1));line_start=int(rows[g+1])
            spans.append((line_start,int(rows[-1])+1))
            if len(spans)>6:raise ValueError('Có quá nhiều dòng. Chọn riêng vùng công thức.')
            for line,(y1,y2) in enumerate(spans,1):
                crop=oriented.crop((0,max(0,y1-8),oriented.width,min(oriented.height,y2+8)))
                alternatives=[]
                # Keep distinct scale readings visible, without guessed scores.
                scales=(0.5,1.0,4.0) if crop.height<70 else (0.5,1.0)
                for factor in scales:
                    sample=crop.resize((max(1,round(crop.width*factor)),max(1,round(crop.height*factor))),Image.Resampling.LANCZOS)
                    raw,_=engine(np.asarray(sample))
                    if len(raw)>2048:raise ValueError('Kết quả quá dài. Chọn vùng nhỏ hơn.')
                    if raw not in alternatives:
                        alternatives.append(raw)
                        if count<12:
                            out[f'candidate{count}']={'raw64':base64.b64encode(raw.encode()).decode(),'line':str(line),'rotation':str(rotation),'scale':str(factor)}
                            count+=1
                        else:truncated=True
        if truncated:out['result']['notes']='Có hơn 12 cách đọc; chọn vùng nhỏ hơn để xem đủ ứng viên.'
        if clipped:out['result']['notes']='Chữ chạm mép vùng: có thể bị cắt mất số mũ, căn hoặc mẫu số. Nên chọn lại vùng rộng hơn.'
        out['result'].update(code='ok', count=str(count), seconds=f'{time.monotonic()-start:.4f}')
    except ImportError:
        out['result'].update(code='missing-backend',message='Thiếu runtime OCR. Chạy scripts/teaching-tools/setup-formula-ocr.sh trong mã nguồn.')
    except Exception as e:
        out['result']['message']=str(e).replace('\n',' ')[:800]
    out.write(sys.stdout)
if __name__=='__main__': main()
