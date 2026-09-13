import json,os,re,subprocess,sys
PLUG=sys.argv[1]; MODEL=sys.argv[2]; PANEL=sys.argv[3]
SCALE=float(sys.argv[4]) if len(sys.argv)>4 else 2.0
OUT=sys.argv[5]
ROOT=os.path.expanduser(f'~/Library/Application Support/Rack2/plugins-mac-arm64/{PLUG}')
cen=json.load(open(os.path.expanduser(f'~/ProgrammingProjects/DreamerDevelopment/research/help/census/{PLUG}.json')))['models'][MODEL]
svg=open(os.path.join(ROOT,'res',PANEL),encoding='utf-8',errors='ignore').read()
head=re.search(r'<svg\b[^>]*>',svg,re.S).group(0)
def attr(n):
    m=re.search(n+r'="([^"]+)"',head)
    return m.group(1) if m else None
vb=[float(x) for x in re.split(r'[ ,]+',attr('viewBox').strip())]
w=attr('width')
m=re.match(r'([\d.]+)\s*(\w*)',w); n=float(m.group(1)); u=m.group(2)
mm = n if u=='mm' else n/(96/25.4)
modpx = mm*75/25.4
outw=int(modpx*SCALE)
body=svg[svg.index(head)+len(head):]
body=body[:body.rindex('</svg>')]
k=vb[2]/modpx
ov=[]
cols={'param':'#ff0000','in':'#0000ff','out':'#00aa00'}
for kind,key in (('param','paramPos'),('in','inputPos'),('out','outputPos')):
    P=cen[key]
    items=P.items() if isinstance(P,dict) else enumerate(P)
    for i,p in items:
        if not p: continue
        x=vb[0]+p[0]*k; y=vb[1]+p[1]*k
        r=5*k
        ov.append(f'<circle cx="{x}" cy="{y}" r="{r}" fill="none" stroke="{cols[kind]}" stroke-width="{0.8*k}"/>')
        ov.append(f'<text x="{x}" y="{y+3*k}" font-size="{9*k}" font-family="Helvetica" fill="{cols[kind]}" text-anchor="middle" stroke="white" stroke-width="{0.7*k}" paint-order="stroke">{i}</text>')
out=head+body+''.join(ov)+'</svg>'
tmp=OUT+'.svg'
open(tmp,'w').write(out)
subprocess.run(['rsvg-convert','-w',str(outw),tmp,'-o',OUT],check=True)
print(OUT,outw)
