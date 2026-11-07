"""Owned synthetic fixtures: TeX glyphs, Unicode PDF text, scan, rotation/oversize."""
import pathlib,subprocess,json,cairo,math
from PIL import Image,ImageFilter
from pypdf import PdfReader,PdfWriter
out=pathlib.Path(__file__).resolve().parents[2]/'output/pdf/pdf-formula-fixtures'
out.mkdir(parents=True,exist_ok=True)
cases=[('quadratic',r'y=x^2-3x+2','x^2-3*x+2'),('cubic',r'y=x^3-3x','x^3-3*x'),('reciprocal',r'y=\frac{1}{x}','1/x'),('hole',r'y=\frac{x^2-1}{x-1}','(x^2-1)/(x-1)'),('pole',r'y=\frac{x^2+1}{x-1}','(x^2+1)/(x-1)'),('frac-scope',r'y=\frac{1}{x+1}','1/(x+1)'),('sum-scope',r'y=\frac{1}{x}+1','1/x+1'),('root-scope',r'y=\sqrt{x+1}','sqrt(x+1)'),('root-sum',r'y=\sqrt{x}+1','sqrt(x)+1'),('minus-square',r'y=-x^2','-x^2'),('square-minus',r'y=(-x)^2','(-x)^2'),('negative-power',r'y=x^{-2}','x^(-2)'),('trig',r'y=\sin(x)+\cos(\pi x)','sin(x)+cos(pi*x)'),('exponential',r'y=2^x','2^x'),('log',r'y=\ln x','ln(x)'),('absolute',r'y=|x|','abs(x)'),('parameter',r'y=x^2+mx','x^2+m*x'),('nested',r'y=\frac{1}{1+\frac{1}{x}}','1/(1+1/x)'),('implicit',r'x^2+y^2=1',None)]
writer=PdfWriter();scan=PdfWriter()
for name,tex,expr in cases:
 d=out/name;d.mkdir(exist_ok=True)
 (d/'formula.tex').write_text(r'\documentclass[border=10pt]{standalone}\usepackage{amsmath}\begin{document}$\displaystyle '+tex+r'$\end{document}')
 subprocess.run(['pdflatex','-no-shell-escape','-halt-on-error','-interaction=batchmode','formula.tex'],cwd=d,stdout=subprocess.DEVNULL,check=True)
 subprocess.run(['pdftoppm','-png','-r','288','-singlefile',str(d/'formula.pdf'),str(d/'crop')],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=True)
 # Use the actual PDF as a form on a regular page; selection geometry fixed.
 r=PdfReader(d/'formula.pdf');page=writer.add_blank_page(595,842);page.merge_translated_page(r.pages[0],60,842-90-float(r.pages[0].mediabox.height))
 # Raster-only version has no text layer.
 im=Image.open(d/'crop.png').convert('RGB');im.save(d/'scan-small.pdf',resolution=288)
 sp=PdfReader(d/'scan-small.pdf');page=scan.add_blank_page(595,842);page.merge_translated_page(sp.pages[0],60,842-90-float(sp.pages[0].mediabox.height))
writer.write(out/'formulas-text.pdf');scan.write(out/'formulas-scan.pdf')
multi=PdfWriter();page=multi.add_blank_page(595,842)
for name,top in [('quadratic',90),('cubic',190)]:
 source=PdfReader(out/name/'scan-small.pdf').pages[0]
 page.merge_translated_page(source,60,842-top-float(source.mediabox.height))
multi.write(out/'multiple-scan.pdf')

# Distinct source resolution, clipping and blur are measurable limits, not confidence scores.
base=Image.open(out/'frac-scope/crop.png')
for dpi in (96,144):base.resize((round(base.width*dpi/288),round(base.height*dpi/288))).save(out/f'frac-{dpi}dpi.png')
base.filter(ImageFilter.GaussianBlur(3)).save(out/'blurred.png')
base.crop((0,0,base.width,base.height//2)).save(out/'cut-denominator.png')
r=PdfReader(out/'formulas-text.pdf');rot=PdfWriter();rot.add_page(r.pages[0]).rotate(90);rot.write(out/'rotated.pdf')
ov=PdfWriter();p=ov.add_blank_page(5950,8420);p.merge_transformed_page(r.pages[0],[10,0,0,10,0,0]);ov.write(out/'oversized.pdf')
# Explicit Unicode text-layer fixture, including preserved superscript codepoints.
import gi;gi.require_version('Pango','1.0');gi.require_version('PangoCairo','1.0');from gi.repository import Pango,PangoCairo
surface=cairo.PDFSurface(str(out/'unicode-text.pdf'),595,842);ctx=cairo.Context(surface)
for i,t in enumerate(['y=x²−3x+2','y=x³−3x','y=1/(x+1)','y=sqrt(x+1)','x²+y²=1']):
 l=PangoCairo.create_layout(ctx);l.set_font_description(Pango.FontDescription('DejaVu Serif 20'));l.set_text(t,-1);ctx.move_to(60,90+i*70);PangoCairo.show_layout(ctx,l)
surface.finish()
domains=json.loads(pathlib.Path(__file__).with_name('formula-fixture-expectations.json').read_text())
(out/'manifest.json').write_text(json.dumps([dict(name=n,latex=t,expected=e,expected_ast_input=e,page=i+1,selection=[55,85,400,150],**domains.get(n,{})) for i,(n,t,e) in enumerate(cases)],indent=2))
print(out)
