#!/usr/bin/env python3
"""Builds the HD structure pictures the game draws over its classic ones.

Reads the artist's sheets in assets/hd and writes data/hd: manifest.txt (which
pictures go with which classic animation strip) and one .hdp pack per
building (pictures QOI compressed, each with a team colour mask). The game
loads <data dir>/hd at start (see game/sdl/hdsprites.cpp); F9 switches between
HD and classic.

Each sheet's pictures are placed on the classic frames by fitting their shape
to the classic one (tools/hd/classic: the classic frames and their bounds,
dumped once from the game data), so a building covers the same ground and
lines up with its shadow. The walls sheet is exact: 3 picture pixels per game
pixel.

Needs Python 3 with numpy, Pillow and qoi (pip install numpy pillow qoi).

    python tools/hd/build_hd.py [--out data/hd] [--preview DIR] [--only NAME]
"""

import argparse, os, re, struct, sys
import numpy as np
from PIL import Image, ImageFilter
import qoi

ROOT=os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SHEETS=os.path.join(ROOT,'assets','hd')
CLASSIC=os.path.join(ROOT,'tools','hd','classic')
manifest=[]; previews=[]; PACKS={}

def sheet(name):
    im=Image.open(os.path.join(SHEETS,name)).convert('RGBA')
    a=np.array(im)
    al=a[:,:,3].astype(np.int32)
    al[al<10]=0; al[al>=248]=255
    a[:,:,3]=al
    return a
def cell(a,cw,ch,col,row):
    return a[row*ch:(row+1)*ch, col*cw:(col+1)*cw].copy()
def classic(anir,strip,frame):
    fn=os.path.join(CLASSIC,'%s_%d_%d.png'%(anir,strip,frame))
    return np.array(Image.open(fn).convert('RGBA'))
def bounds(anir,strip,frame):
    cur=None
    for l in open(os.path.join(CLASSIC,'%s.txt'%anir)):
        m=re.match(r"strip (\d+)",l)
        if m: cur=int(m.group(1)); continue
        m=re.match(r"\s+frame (\d+) bounds (-?\d+) (-?\d+) (-?\d+) (-?\d+)",l)
        if m and cur==strip and int(m.group(1))==frame:
            return [int(x) for x in m.groups()[1:]]
def strip_timing(anir,strip):
    """(frame count, length of one loop in game updates) of a classic strip"""
    cur=None; nfrm=0; delay=0; total=0
    for l in open(os.path.join(CLASSIC,'%s.txt'%anir)):
        m=re.match(r"strip (\d+) '[^']*' frames (\d+) delay (\d+)",l)
        if m:
            cur=int(m.group(1))
            if cur==strip: nfrm=int(m.group(2)); delay=int(m.group(3))
            continue
        m=re.match(r"\s+frame (\d+) bounds .* hold (\d+)",l)
        if m and cur==strip: total+=delay+int(m.group(2))+1
    return nfrm,total
def fit(cmask, hdalpha, smin=1.8, smax=7.0, step=0.02):
    """cmask: game-res bool mask (object). hdalpha: HD cell alpha (0..255).
    returns (S, ox, oy): hd = S*(g) + (ox,oy) where g = game pixel coords in cmask frame"""
    best=None
    H=Image.fromarray(hdalpha.astype(np.uint8))
    cy,cx=cmask.shape
    gy,gx=np.nonzero(cmask)
    for S in np.arange(smin,smax,step):
        w=max(1,int(round(hdalpha.shape[1]/S))); h=max(1,int(round(hdalpha.shape[0]/S)))
        d=np.array(H.resize((w,h),Image.BILINEAR))>127
        hy,hx=np.nonzero(d)
        if len(hy)==0: continue
        # align centroids then search +-4
        dx0=int(round(hx.mean()-gx.mean())); dy0=int(round(hy.mean()-gy.mean()))
        for dy in range(dy0-5,dy0+6):
            for dx in range(dx0-5,dx0+6):
                # overlap: classic pixel (x,y) vs d[y+dy,x+dx]
                xs=gx+dx; ys=gy+dy
                ok=(xs>=0)&(xs<w)&(ys>=0)&(ys<h)
                inter=d[ys[ok],xs[ok]].sum()
                union=len(gx)+d.sum()-inter
                iou=inter/union
                if best is None or iou>best[0]:
                    best=(iou,S,dx,dy,w,h)
    iou,S,dx,dy,w,h=best
    # game (x,y) -> small d coords (x+dx,y+dy) -> hd = (x+dx)*S*(W/w)
    sx=hdalpha.shape[1]/w; sy=hdalpha.shape[0]/h
    return dict(iou=float(iou),S=float(S),ox=float(dx*sx),oy=float(dy*sy),sx=float(sx),sy=float(sy))


def team_mask(rgba):
    a=rgba[:,:,3]>0
    rgb=rgba[:,:,:3].astype(np.float32)/255
    mx=rgb.max(2); mn=rgb.min(2); d=mx-mn
    s=np.where(mx>0,d/np.maximum(mx,1e-6),0)
    r,g,b=rgb[:,:,0],rgb[:,:,1],rgb[:,:,2]
    h=np.zeros_like(mx)
    m=d>1e-6
    hr=((g-b)/np.maximum(d,1e-6))%6; hg=(b-r)/np.maximum(d,1e-6)+2; hb=(r-g)/np.maximum(d,1e-6)+4
    h=np.where(mx==r,hr,np.where(mx==g,hg,hb))*60
    h=np.where(m,h,0)
    # blue team paint: hue 195..245, saturated enough
    w_h=np.clip(1-np.maximum(0,np.abs(h-218)-22)/12,0,1)
    w_s=np.clip((s-0.16)/0.14,0,1)
    w_v=np.clip((mx-0.08)/0.1,0,1)
    t=(w_h*w_s*w_v*a)
    return (t*255).astype(np.uint8)

def vehicle_team_mask(rgba):
    """The vehicle sheets' paint is a greyer steel blue than the buildings'
    (saturation mostly 0.17..0.33, hue 185..240), so team_mask caught it
    only in part: the paint came out mottled, blended half way between the
    side's colour and the sheet's blue (purple on red, green on yellow).
    Nothing else on the vehicles is near that hue (the hulls are tan, the
    metal neutral grey, the Hydra's nose cone magenta), so a wider hue
    window and a much lower saturation floor take the paint whole."""
    a=rgba[:,:,3]>0
    rgb=rgba[:,:,:3].astype(np.float32)/255
    mx=rgb.max(2); mn=rgb.min(2); d=mx-mn
    s=np.where(mx>0,d/np.maximum(mx,1e-6),0)
    r,g,b=rgb[:,:,0],rgb[:,:,1],rgb[:,:,2]
    hr=((g-b)/np.maximum(d,1e-6))%6; hg=(b-r)/np.maximum(d,1e-6)+2; hb=(r-g)/np.maximum(d,1e-6)+4
    h=np.where(mx==r,hr,np.where(mx==g,hg,hb))*60
    h=np.where(d>1e-6,h,0)
    w_h=np.clip(1-np.maximum(0,np.abs(h-215)-37)/12,0,1)
    w_s=np.clip((s-0.05)/0.07,0,1)
    w_v=np.clip((mx-0.06)/0.08,0,1)
    return (w_h*w_s*w_v*a*255).astype(np.uint8)

def crop(rgba):
    al=rgba[:,:,3]
    ys,xs=np.nonzero(al)
    if len(ys)==0: return rgba[:1,:1],0,0
    y0,y1,x0,x1=ys.min(),ys.max()+1,xs.min(),xs.max()+1
    return rgba[y0:y1,x0:x1],x0,y0

def save_pic(name, rgba, gx, gy, S, team=True):
    """rgba: HD cell; (gx,gy): game coords (rel. to draw point) of the cell's top-left; S: HD px per game px"""
    c,x0,y0=crop(rgba)
    pack,pic=name.split('/')
    t=(team(c) if callable(team) else team_mask(c)) if team else None
    if t is not None and t.max()==0: t=None
    PACKS.setdefault(pack,[]).append((pic, gx+x0/S, gy+y0/S, S, c, t))

def last_row_wreck_icon(a,cw,ch,row,cols):
    cells=[(c,cell(a,cw,ch,c,row)) for c in range(cols)]
    cells=[(c,x) for c,x in cells if (x[:,:,3]>0).sum()>50]
    cells.sort(key=lambda t:-(t[1][:,:,3]>0).sum())
    return [x for c,x in cells]  # biggest first: wreck(s)..., icon last

def placement(cm_strip, anir, rgba0, origin=None, S_fixed=None):
    """fit HD cell rgba0 onto classic frame mask; returns (gx,gy,S) for the cell top-left in draw-point coords.
    S_fixed: only find the place, at this scale"""
    cm,(l,t)=cm_strip
    if S_fixed is None:
        r=fit(cm, rgba0[:,:,3])
    else:
        r=fit(cm, rgba0[:,:,3], smin=S_fixed, smax=S_fixed+0.001)
    S=(r['sx']+r['sy'])/2
    gx=l - r['ox']/S; gy=t - r['oy']/S
    print('  fit %s iou %.3f S %.3f'%(anir,r['iou'],S))
    return gx,gy,S

def classic_mask(anir,strip,frame):
    im=classic(anir,strip,frame); b=bounds(anir,strip,frame)
    return im[:,:,3]==255,(b[0],b[1])

def preview(name, anir, strip, frame, rgba, gx, gy, S):
    im=Image.fromarray(classic(anir,strip,frame)); b=bounds(anir,strip,frame)
    # canvas in game coords *S
    cl=Image.new('RGBA',(int(im.width*S),int(im.height*S)))
    big=im.resize((int(im.width*S),int(im.height*S)),Image.NEAREST)
    W=max(big.width, int((gx-b[0])*S)+rgba.shape[1])+int(abs(gx-b[0])*S)+10
    canvas=Image.new('RGBA',(big.width*3+40, max(big.height,rgba.shape[0])+int(abs(gy-b[1])*S)+40),(40,90,40,255))
    canvas.paste(big,(0,20),big)
    h=Image.fromarray(rgba)
    ox=int((gx-b[0])*S); oy=int((gy-b[1])*S)
    canvas.paste(h,(big.width+20+ox,20+oy),h)
    # overlay outline
    ov=canvas.copy(); ov.paste(big,(big.width+20,20),Image.fromarray(((np.array(big)[:,:,3]>0)*90).astype(np.uint8)))
    canvas=Image.blend(canvas,ov,0.5)
    previews.append((name,canvas))

# The damaged and destroyed pictures aren't always in the same place in their
# cells as the healthy one (a few pixels; the HRC and VTS columns drift
# right). The game draws all three at the same spot, so each is moved to
# stand where the healthy building stands: damaged by its whole outline
# (the same shape), destroyed by its lower part, the foundation (the top
# may have fallen in: the Surveillance Center's dish, the Rocket Tower's
# launcher, which match less well but still by their pedestals; checked
# against the classic wrecks too). A wreck matching worse than that stays
# put.

def _iou_shift(A,B,dx,dy,cut):
    H,W=A.shape
    sh=np.zeros_like(B)
    sh[max(0,dy):min(H,H+dy),max(0,dx):min(W,W+dx)]=B[max(0,-dy):min(H,H-dy),max(0,-dx):min(W,W-dx)]
    a=A[cut:]; s=sh[cut:]
    return (a&s).sum()/max(1,(a|s).sum())

def align(healthy,other,base=False,r=40):
    """(dx,dy) in sheet pixels moving `other` onto `healthy`, and how well"""
    A=healthy[:,:,3]>0; B=other[:,:,3]>0
    ys=np.nonzero(A)[0]; cut=int(ys.max()-(ys.max()-ys.min())*0.4) if base else 0
    k=4
    As=A[::k,::k]; Bs=B[::k,::k]; best=(0,0,-1.0)
    for dy in range(-r//k,r//k+1):
        for dx in range(-r//k,r//k+1):
            v=_iou_shift(As,Bs,dx,dy,cut//k)
            if v>best[2]: best=(dx*k,dy*k,v)
    x0,y0,_=best; best=(0,0,-1.0)
    for dy in range(y0-k,y0+k+1):
        for dx in range(x0-k,x0+k+1):
            v=_iou_shift(A,B,dx,dy,cut)
            if v>best[2]: best=(dx,dy,v)
    return best

def moved(gx,gy,S,healthy,other,base=False,name=''):
    """placement of `other` standing where `healthy` stands"""
    dx,dy,iou=align(healthy,other,base)
    if base and iou<0.55:
        print('  %s: too different to align (%.2f), left in place'%(name,iou)); return gx,gy
    if dx or dy: print('  %s moved %d,%d sheet px (match %.2f)'%(name,dx,dy,iou))
    return gx+dx/S, gy+dy/S

def seq(a,cw,ch,cols,start,count):
    return [cell(a,cw,ch,(start+i)%cols,(start+i)//cols) for i in range(count)]

def strip(anir, name, **kw):
    s='strip\t%s\t%s'%(anir,name)
    for k,v in kw.items(): s+='\t%s=%s'%(k,v)
    manifest.append(s)

def standard(bld, sheetname, cw, ch, cols, anir, healthy_n, damaged_start, nrows_last, strips, sheet_obj=None, fitframe=(0,0), wreck_cell=None, cycle=None, hd_name=None):
    """healthy pictures 0..n-1, damaged from damaged_start, last row wreck+icon.
    cycle: the pictures loop on their own, once in that many game updates,
    instead of following the classic frames (which are too few and too
    quick for this much art).
    hd_name: the name the game binds the pictures by, when it isn't the
    classic animation's (a building drawn with another one's classic art)"""
    print(bld)
    anir_classic=anir
    a=sheet(sheetname) if sheet_obj is None else sheet_obj
    H=seq(a,cw,ch,cols,0,healthy_n); D=seq(a,cw,ch,cols,damaged_start,healthy_n)
    last=last_row_wreck_icon(a,cw,ch,nrows_last,cols)
    W=last[0] if wreck_cell is None else cell(a,cw,ch,*wreck_cell)
    gx,gy,S=placement(classic_mask(anir,*fitframe),anir,H[0])
    dgx,dgy=moved(gx,gy,S,H[0],D[0],name=bld+' damaged')
    wgx,wgy=moved(gx,gy,S,H[0],W,base=True,name=bld+' wreck')
    for i,c in enumerate(H): save_pic('%s/h%03d'%(bld,i),c,gx,gy,S)
    for i,c in enumerate(D): save_pic('%s/d%03d'%(bld,i),c,dgx,dgy,S)
    save_pic('%s/w000'%bld,W,wgx,wgy,S)
    preview(bld,anir,fitframe[0],fitframe[1],H[0],gx,gy,S)
    preview(bld+'_d',anir,1,0,D[0],gx,gy,S)
    preview(bld+'_w',anir,2,0,W,gx,gy,S)
    for i,c in enumerate(H+D):
        if (c[:,:,3]>0).sum()==0: raise SystemExit('%s: picture %d is empty (wrong cell?)'%(bld,i))
    s0,s1,s2=strips
    if hd_name:
        anir=hd_name
    if cycle:
        strip(anir,s0,pics='%s/h'%bld,n=healthy_n,var=0,cycle=cycle)
        strip(anir,s1,pics='%s/d'%bld,n=healthy_n,var=1,cycle=cycle)
    else:
        strip(anir,s0,pics='%s/h'%bld,n=healthy_n,var=0)
        # A damaged strip the classic art doesn't animate (one frame) still
        # plays all its pictures, looping at the healthy animation's speed
        nfrm_d,_=strip_timing(anir_classic,1); nfrm_h,cycle_h=strip_timing(anir_classic,0)
        if nfrm_d==1 and nfrm_h>1:
            strip(anir,s1,pics='%s/d'%bld,n=healthy_n,var=1,cycle=cycle_h)
        else:
            strip(anir,s1,pics='%s/d'%bld,n=healthy_n,var=1)
    strip(anir,s2,pics='%s/w'%bld,n=1,var=2)

# HQ: 8x9 cells 288x216; 32 healthy, 32 damaged, last row wreck+icon
# Power generator: 6 cols, 192x168
# Surveillance center
# R&D center (panel version, as the classic one)

def warehouse():
    print('warehouse'); a=sheet('warehouse.png'); cw=ch=362
    H=[cell(a,cw,ch,c,0) for c in range(4)]; D=[cell(a,cw,ch,c,1) for c in range(4)]; W=[cell(a,cw,ch,c,2) for c in range(3)]
    gx,gy,S=placement(classic_mask('Warehouse.anir',0,0),'Warehouse.anir',H[0])
    dgx,dgy=moved(gx,gy,S,H[0],D[0],name='warehouse damaged')
    wgx,wgy=moved(gx,gy,S,H[0],W[0],base=True,name='warehouse wreck')
    for i,c in enumerate(H): save_pic('warehouse/h%03d'%i,c,gx,gy,S)
    for i,c in enumerate(D): save_pic('warehouse/d%03d'%i,c,dgx,dgy,S)
    for i,c in enumerate(W): save_pic('warehouse/w%03d'%i,c,wgx,wgy,S)
    preview('warehouse','Warehouse.anir',0,3,H[3],gx,gy,S); preview('warehouse_w','Warehouse.anir',2,0,W[0],gx,gy,S)
    strip('Warehouse.anir','a 0',pics='warehouse/h',n=4,var=0,blend=0)
    strip('Warehouse.anir','a 1',pics='warehouse/d',n=4,var=1,blend=0)
    strip('Warehouse.anir','a 2',pics='warehouse/w',n=3,var=2,blend=0)

def hrcvts():
    a=sheet('hrcvts.png'); cw=ch=362
    for row,(bld,anir,names) in enumerate([('hrc','HRC.anir',('base 0','base 1','base 2')),('hrc2','HRC.anir',('base2 0','base2 1','base2 2')),
            ('vts','VTS.anir',('a 0','a 1','a 2')),('vts2','VTS.anir',('b 0','b 1','b 2'))]):
        print(bld)
        cs=[cell(a,cw,ch,c,row) for c in range(3)]
        si=[0,0,0]
        # classic strip indexes
        idx={'base 0':0,'base 1':1,'base 2':2,'base2 0':3,'base2 1':4,'base2 2':5,'a 0':0,'a 1':1,'a 2':2,'b 0':3,'b 1':4,'b 2':5}
        gx,gy,S=placement(classic_mask(anir,idx[names[0]],0),anir,cs[0])
        pos=[(gx,gy),moved(gx,gy,S,cs[0],cs[1],name=bld+' damaged'),moved(gx,gy,S,cs[0],cs[2],base=True,name=bld+' wreck')]
        for k,(c,nm) in enumerate(zip(cs,names)):
            save_pic('%s/%s000'%(bld,'hdw'[k]),c,pos[k][0],pos[k][1],S)
            strip(anir,nm,pics='%s/%s'%(bld,'hdw'[k]),n=1,var=k)
        preview(bld,anir,idx[names[0]],0,cs[0],gx,gy,S)

def proc():
    print('proc'); a=sheet('proc.png'); cw=ch=296
    H=seq(a,cw,ch,6,0,36); D=seq(a,cw,ch,6,36,36)
    W=last_row_wreck_icon(a,cw,ch,12,6)[0]
    OPEN=20
    gx,gy,S=placement(classic_mask('proc.anir',0,0),'proc.anir',H[OPEN])
    pos={'h':(gx,gy),'d':moved(gx,gy,S,H[OPEN],D[OPEN],name='proc damaged')}
    wgx,wgy=moved(gx,gy,S,H[OPEN],W,base=True,name='proc wreck')
    for nm,L in (('h',H),('d',D)):
        base=L[OPEN]
        px,py=pos[nm]
        save_pic('proc/%s000'%nm,base,px,py,S)
        bf=base.astype(np.float32)
        for i,c in enumerate(L):
            cf=c.astype(np.float32)
            diff=np.abs(cf[:,:,:3]-bf[:,:,:3]).max(2)+np.abs(cf[:,:,3]-bf[:,:,3])
            w=np.clip((diff-12)/28,0,1)
            ov=c.copy(); ov[:,:,3]=(c[:,:,3]*w).astype(np.uint8)
            if ov[:,:,3].max()<16: ov[:,:,3]=0; ov[0,0,3]=1
            save_pic('proc/%sdoor%03d'%(nm,i),ov,px,py,S)
    save_pic('proc/w000',W,wgx,wgy,S)
    preview('proc','proc.anir',0,0,H[OPEN],gx,gy,S); preview('proc_w','proc.anir',2,0,W,gx,gy,S)
    strip('proc.anir','base 0',pics='proc/h',n=1,var=0)
    strip('proc.anir','base 1',pics='proc/d',n=1,var=1)
    strip('proc.anir','base 2',pics='proc/w',n=1,var=2)
    strip('proc.anir','door 0',pics='proc/{V}door',n=36,map='8,10,12,13,15,17',bmap='35,33,31,30,28,26')

def tower(t, anir):
    print(t)
    A=sheet('%s_ok.png'%t); B=sheet('%s_dmg.png'%t); Wr=sheet('%s_wreck.png'%t)
    # the Gatling tower's sheets are swapped: its _ok sheet is the cracked one
    if t=='mtower': A,B=B,A
    cw=ch=384
    # classic composite: base 0 + turret 00 frame 0 at the base's special point (16,7)
    bm,(bl,bt)=classic_mask(anir,0,0); tm,(tl,tt)=classic_mask(anir,3,0)
    sx,sy=16,7
    L=min(bl,tl+sx); T=min(bt,tt+sy); R=max(bl+bm.shape[1],tl+sx+tm.shape[1]); Bm=max(bt+bm.shape[0],tt+sy+tm.shape[0])
    m=np.zeros((Bm-T,R-L),bool)
    m[bt-T:bt-T+bm.shape[0],bl-L:bl-L+bm.shape[1]]|=bm
    m[tt+sy-T:tt+sy-T+tm.shape[0],tl+sx-L:tl+sx-L+tm.shape[1]]|=tm
    gx,gy,S=placement((m,(L,T)),anir,cell(A,cw,ch,0,0))
    for d in range(16):
        # damaged: matched per direction (the turret's outline differs)
        dgx,dgy=moved(gx,gy,S,cell(A,cw,ch,0,d),cell(B,cw,ch,0,d),name='%s %02d damaged'%(t,d))
        for k in range(3):
            save_pic('%s/h%02d_%03d'%(t,d,k),cell(A,cw,ch,k,d),gx-sx,gy-sy,S)
            save_pic('%s/d%02d_%03d'%(t,d,k),cell(B,cw,ch,k,d),dgx-sx,dgy-sy,S)
        strip(anir,'turret %02d'%d,pics='%s/{V}%02d_'%(t,d),n=3,map='0,1,2,1,2',blend=0)
    wgx,wgy=moved(gx,gy,S,cell(A,cw,ch,0,0),Wr,base=True,name=t+' wreck')
    save_pic('%s/w000'%t,Wr,wgx,wgy,S)
    strip(anir,'base 0',none=1,var=0); strip(anir,'base 1',none=1,var=1)
    strip(anir,'base 2',pics='%s/w'%t,n=1,var=2)
    # preview: composite classic frame vs HD
    previews.append((t,Image.fromarray(np.concatenate([np.array(Image.fromarray((m*255).astype(np.uint8)).resize((int(m.shape[1]*S),int(m.shape[0]*S)))) [:,:,None].repeat(4,2)],1))))

# Vehicles: a body in 8 directions (one row, clockwise from north, as the
# classic 'base 0'..'base 7') and a turret in 16 directions (columns, as
# 'fire 00'..'fire 15') with a row per classic firing frame (row 0 at
# rest). The body sheet is fitted once ('base 0') and keeps that placement
# in every direction (the artist turned the model about the same point);
# the turret sheet takes the body's placement (see below). The build menu icon is the body and
# turret together, facing west like the classic icon.

def turret_orbit(T, c):
    """How far (HD px) each of the 16 turret pictures' middle is off the
    turret's axis: the middles (the turret without its thin barrel, an
    opening of the mask) are fitted to P + R(22.5 d) r, a point turning
    about the axis P; returns R(22.5 d) r for each direction"""
    import math
    M=[]; y=[]
    for d in range(16):
        a=(cell(T,c,c,d,0)[:,:,3]>128).astype(np.uint8)*255
        o=np.array(Image.fromarray(a).filter(ImageFilter.MinFilter(9)).filter(ImageFilter.MaxFilter(9)))>128
        if o.sum()==0: o=a>0
        ys,xs=np.nonzero(o)
        t=math.radians(22.5*d); co,si=math.cos(t),math.sin(t)
        M.append([1,0,co,-si]); y.append(xs.mean())
        M.append([0,1,si,co]); y.append(ys.mean())
    px,py,rx,ry=np.linalg.lstsq(np.array(M,float),np.array(y,float),rcond=None)[0]
    print('  turret axis %.1f,%.1f, middle %.1f HD px off it'%(px,py,math.hypot(rx,ry)))
    out=[]
    for d in range(16):
        t=math.radians(22.5*d); co,si=math.cos(t),math.sin(t)
        out.append((co*rx-si*ry, si*rx+co*ry))
    return out

# The game recolours the team paint by its brightness on the classic blue
# ramp (95, 79, 51, 39, 26 luma). Each sheet's paint came out brighter or
# darker than the others' (median luma 75 to 96, much of it above the
# brightest shade, where the side's colour is only scaled up) and with its
# own spread of shading, so a side's colour looked different from vehicle
# to vehicle: the Hydra's launcher came out dark and dull next to its
# bright pods. Each sheet's paint brightness is matched, quantile by
# quantile, to one common spread: all the vehicle sheets' paint pooled,
# scaled so its upper quartile (the lit faces, the colour the eye reads)
# sits just above the brightest shade.
PAINT_SHEETS=['broadsword_body.png','broadsword_turret.png','eagle_body.png','eagle_turret.png',
    'hydra_body.png','hydra_turret.png','liberator_body.png','liberator_turret.png','cyclops.png']
PAINT_UPPER=100.0
_paint_ref=[]
def paint_luma(a):
    m=vehicle_team_mask(a).astype(np.float32)/255
    l=a[:,:,:3].astype(np.float32)@np.array([0.299,0.587,0.114],np.float32)
    return m,l
def paint_reference():
    if not _paint_ref:
        qs=np.linspace(0,100,101)
        tabs=[]
        for f in PAINT_SHEETS:
            m,l=paint_luma(sheet(f)); tabs.append(np.percentile(l[m>0.5],qs))
        ref=np.mean(tabs,0)
        ref*=PAINT_UPPER/np.interp(75,qs,ref)
        _paint_ref.append(ref)
    return _paint_ref[0]
def balance_paint(a, what):
    m,l=paint_luma(a)
    sel=m>0.5
    if sel.sum()==0: return a
    qs=np.linspace(0,100,101)
    src=np.percentile(l[sel],qs)
    src=np.maximum.accumulate(src+np.arange(101)*1e-4)
    lnew=np.interp(l,src,paint_reference())
    k=np.where(l>0.5,lnew/np.maximum(l,0.5),1)
    print('  %s paint median luma %.0f -> %.0f'%(what,np.median(l[sel]),np.median(lnew[sel])))
    f=1+(k-1)*m
    out=a.copy()
    out[:,:,:3]=np.clip(a[:,:,:3].astype(np.float32)*f[:,:,None]+0.5,0,255).astype(np.uint8)
    return out

def vehicle(name, anir, body_sheet, turret_sheet, c, turret_rows, icon_dirs=(6,12)):
    print(name)
    B=sheet(body_sheet); T=sheet(turret_sheet)
    B=balance_paint(B,'body'); T=balance_paint(T,'turret')
    bodies=[cell(B,c,c,d,0) for d in range(8)]
    gx,gy,S=placement(classic_mask(anir,0,0),anir,bodies[0])
    for d in range(8):
        if (bodies[d][:,:,3]>0).sum()==0: raise SystemExit('%s: body %d is empty'%(name,d))
        save_pic('%s/b%d_000'%(name,d),bodies[d],gx,gy,S,team=vehicle_team_mask)
        strip(anir,'base %d'%d,pics='%s/b%d_'%(name,d),n=1)
        preview('%s_b%d'%(name,d),anir,d,0,bodies[d],gx,gy,S)
    # The turret sheet is rendered with the same camera as the body sheet
    # (same cell size, the model at the same spot in its cell): the bodies
    # turn about the cell centre, where their turret socket is, and the
    # turret pictures get the body's placement (the classic turret is drawn
    # at the body's special point, (0,0) for these vehicles). But a turret
    # model need not turn about its own middle: the Hydra's launcher sits
    # 7-8 HD px off its axis, so it swung round the hull as it turned and sat
    # on the nose or the tail. Each turret picture is moved so the turret's
    # middle stays on the axis: it turns in place on its socket.
    tgx,tgy,TS=gx,gy,S
    tsh=turret_orbit(T,c)
    for d in range(16):
        dx,dy=tsh[d]
        for k in range(turret_rows):
            save_pic('%s/t%02d_%03d'%(name,d,k),cell(T,c,c,d,k),tgx-dx/S,tgy-dy/S,TS,team=vehicle_team_mask)
        strip(anir,'fire %02d'%d,pics='%s/t%02d_'%(name,d),n=turret_rows,blend=0)
        preview('%s_t%02d'%(name,d),anir,8+d,0,cell(T,c,c,d,0),tgx-dx/S,tgy-dy/S,TS)
    # icon: body and turret composed in HD pixels (game coords * S)
    db,dt=icon_dirs
    bc=bodies[db]; tc=cell(T,c,c,dt,0)
    if abs(TS-S)>0.05*S:
        print('  turret scale %.2f differs from the body\'s %.2f'%(TS,S))
    tgx-=tsh[dt][0]/S; tgy-=tsh[dt][1]/S
    tc=np.array(Image.fromarray(tc).resize((max(1,int(round(tc.shape[1]*S/TS))),max(1,int(round(tc.shape[0]*S/TS)))),Image.LANCZOS))
    ox=int(round((tgx-gx)*S)); oy=int(round((tgy-gy)*S))
    L=min(0,ox); Tp=min(0,oy); R=max(bc.shape[1],ox+tc.shape[1]); Bt=max(bc.shape[0],oy+tc.shape[0])
    canvas=Image.new('RGBA',(R-L,Bt-Tp))
    canvas.alpha_composite(Image.fromarray(bc),(-L,-Tp))
    canvas.alpha_composite(Image.fromarray(tc),(ox-L,oy-Tp))
    save_pic('%s/icon000'%name,np.array(canvas),gx+L/S,gy+Tp/S,S,team=vehicle_team_mask)
    strip(anir,'icon',pics='%s/icon'%name,n=1)

def weathered_team_mask(rgba):
    """The Cyclops' paint is weathered: its blue plates are flecked with
    brown grime and rivets, so the paint mask came out speckled. The mask
    is closed (gaps a few px wide inside painted areas filled) and softened."""
    m=Image.fromarray(vehicle_team_mask(rgba))
    m=m.filter(ImageFilter.MaxFilter(7)).filter(ImageFilter.MinFilter(7)).filter(ImageFilter.GaussianBlur(1.2))
    return np.minimum(np.array(m),(rgba[:,:,3]>0)*255).astype(np.uint8)

def cyclops():
    """Cyclops (artillery.anir): no turret, the whole gun turns. The sheet:
    8 directions across, 3 rows down (at rest, firing, after), the classic
    'fire d' strips' 3 frames; 'move d' is the one at-rest frame."""
    name,anir,c='cyclops','artillery.anir',344
    print(name)
    A=balance_paint(sheet('cyclops.png'),'gun')
    gx,gy,S=placement(classic_mask(anir,8,0),anir,cell(A,c,c,0,0))
    for d in range(8):
        for k in range(3):
            save_pic('%s/f%d_%03d'%(name,d,k),cell(A,c,c,d,k),gx,gy,S,team=weathered_team_mask)
        save_pic('%s/m%d_000'%(name,d),cell(A,c,c,d,0),gx,gy,S,team=weathered_team_mask)
        strip(anir,'fire %d'%d,pics='%s/f%d_'%(name,d),n=3,blend=0)
        strip(anir,'move %d'%d,pics='%s/m%d_'%(name,d),n=1)
        preview('%s_m%d'%(name,d),anir,8+d,0,cell(A,c,c,d,0),gx,gy,S)
    # icon: facing east, as the classic one
    save_pic('%s/icon000'%name,cell(A,c,c,2,0),gx,gy,S,team=weathered_team_mask)
    strip(anir,'icon',pics='%s/icon'%name,n=1)

def walls():
    print('walls'); a=sheet('walls.png'); cw=ch=112
    # the sheet's tiles are 96 px with an 8 px margin in each 112 px cell:
    # exactly 3 HD px per game px, so neighbouring pieces join seamlessly
    S=3.0; gx=gy=-8/3.0
    for i in range(16):
        save_pic('wall/h%03d'%i,cell(a,cw,ch,i%8,i//8),gx,gy,S)
        save_pic('wall/d%03d'%i,cell(a,cw,ch,i%8,2+i//8),gx,gy,S)
    strip('walls.anir','a',pics='wall/h',n=16,blend=0)
    strip('walls.anir','b',pics='wall/d',n=16,blend=0)
    preview('wall15','walls.anir',0,15,cell(a,cw,ch,7,1),gx,gy,S)
    preview('wall5','walls.anir',0,5,cell(a,cw,ch,5,0),gx,gy,S)

def finish(out, preview_dir):
    if preview_dir:
        os.makedirs(preview_dir,exist_ok=True)
        for n,c in previews:
            c.save(os.path.join(preview_dir,'%s.png'%n))
    os.makedirs(out,exist_ok=True)
    for pack,L in PACKS.items():
        with open(os.path.join(out,pack+'.hdp'),'wb') as f:
            f.write(b'HDP1'); f.write(struct.pack('<I',len(L)))
            for pic,x,y,S,c,t in L:
                nb=pic.encode(); f.write(struct.pack('<B',len(nb))); f.write(nb)
                f.write(struct.pack('<fff',x,y,S))
                q=qoi.encode(np.ascontiguousarray(c)); f.write(struct.pack('<I',len(q))); f.write(q)
                if t is None: f.write(struct.pack('<I',0))
                else:
                    tt=np.zeros(t.shape+(4,),np.uint8); tt[:,:,0]=tt[:,:,1]=tt[:,:,2]=t; tt[:,:,3]=255
                    q=qoi.encode(tt); f.write(struct.pack('<I',len(q))); f.write(q)
    hdr=['# HD structure pictures (see game/sdl/hdsprites.cpp). Made by tools/hd/build_hd.py from the artist\'s sheets.',
         '# strip<TAB>anir<TAB>strip name<TAB>key=value...: pics=<pack>/<prefix> n=<count> var=<0 healthy|1 damaged|2 wreck>',
         '#   blend=0 (no in-between pictures) map=/bmap= (picture per classic frame going forward/back) none=1 (draw nothing)',
         '#   cycle=<updates>: the pictures loop on their own (not following the classic frames), one loop in that many game updates',
         '#   {V} in pics = h/d by the variant of the strip drawn just before (2 = draw nothing)']
    open(os.path.join(out,'manifest.txt'),'w').write('\n'.join(hdr+manifest)+'\n')
JOBS=[
    ('warehouse', lambda: warehouse()),
    ('hrcvts', lambda: hrcvts()),
    ('proc', lambda: proc()),
    ('mtower', lambda: tower('mtower','mtower.anir')),
    ('rtower', lambda: tower('rtower','rtower.anir')),
    ('walls', lambda: walls()),
    ('hq', lambda: standard('hq','hq.png',288,216,8,'HQ.anir',32,32,8,('a 0','a 1','a 2'))),
    # Times at the default game speed (2.0: 40 ms an update); at speed 1.0
    # twice as long.
    # Power Generator: a turn of the fan in 15 updates (0.6 s at speed 2.0,
    # 60 pictures a second: a new one each frame on a 60 Hz screen)
    ('reactor', lambda: standard('reactor','reactor.png',192,168,6,'Reactor.anir',36,36,12,('spin 0','spin 1','spin 2'),cycle=15)),
    # Surveillance Center: a slow sweep, a turn of the dish in 48 updates (1.9 s)
    ('radar', lambda: standard('radar','radar.png',362,362,6,'Radar.anir',36,36,12,('a 0','a 1','a 2'),cycle=48)),
    # R&D Center: the panel version (the classic one turns its panel; the
    # dish version, research_dish.png, isn't used)
    # (52 healthy pictures fill 6.5 rows: the damaged ones start on row 7.)
    # The panel goes out and back in 66 updates (2.6 s)
    ('research', lambda: standard('research','research_panel.png',362,362,8,'Research.anir',52,56,13,('a 0','a 1','a 2'),wreck_cell=(4,13),cycle=66)),
    # Command Center: the R&D Center's classic art and timing with the dish
    # version of its HD art (the game binds it as CommandCenter.anir)
    ('ccenter', lambda: standard('ccenter','research_dish.png',362,362,8,'Research.anir',52,56,13,('a 0','a 1','a 2'),wreck_cell=(4,13),cycle=66,hd_name='CommandCenter.anir')),
    # R&D Center upgrade menu icons: the upgraded buildings' healthy pictures
    # (HRC + VTS 02 and 04, from the hrcvts job's packs)
    ('upgrades', lambda: (strip('upgrades.anir','hrc1',pics='hrc2/h',n=1), strip('upgrades.anir','vts1',pics='vts2/h',n=1))),
    # Broadsword (light tank): 3 turret rows (at rest, firing, after)
    ('broadsword', lambda: vehicle('broadsword','ltank.anir','broadsword_body.png','broadsword_turret.png',208,3)),
    # Eagle (machine gun vehicle): 9 turret rows, its 9 classic firing frames
    ('eagle', lambda: vehicle('eagle','tmac.anir','eagle_body.png','eagle_turret.png',176,9)),
    # Hydra (rocket vehicle): 3 turret rows (at rest, firing, after)
    ('hydra', lambda: vehicle('hydra','troc.anir','hydra_body.png','hydra_turret.png',216,3)),
    # Liberator (medium tank): the Hydra's chassis; 5 turret rows, its 5
    # classic firing frames (at rest, then the two barrels in turn)
    # Cyclops (artillery): one sheet, the whole gun turns
    ('cyclops', cyclops),
    ('liberator', lambda: vehicle('liberator','mtank.anir','liberator_body.png','liberator_turret.png',272,5)),
]

# Every classic strip that must have HD pictures (a missing one draws the
# classic art; checked after a full build)
REQUIRED=[('HQ.anir',s) for s in ('a 0','a 1','a 2')]+\
    [('Reactor.anir',s) for s in ('spin 0','spin 1','spin 2')]+\
    [(a,s) for a in ('Radar.anir','Research.anir','CommandCenter.anir','Warehouse.anir','VTS.anir') for s in ('a 0','a 1','a 2')]+\
    [('VTS.anir',s) for s in ('b 0','b 1','b 2')]+\
    [('HRC.anir',s) for s in ('base 0','base 1','base 2','base2 0','base2 1','base2 2')]+\
    [('proc.anir',s) for s in ('base 0','base 1','base 2','door 0')]+\
    [(t,s) for t in ('mtower.anir','rtower.anir') for s in ['base 0','base 1','base 2']+['turret %02d'%d for d in range(16)]]+\
    [('walls.anir','a'),('walls.anir','b')]+\
    [('upgrades.anir','hrc1'),('upgrades.anir','vts1')]+\
    [(a,s) for a in ('ltank.anir','tmac.anir','troc.anir','mtank.anir') for s in ['base %d'%d for d in range(8)]+['fire %02d'%d for d in range(16)]+['icon']]+\
    [('artillery.anir',s) for s in ['fire %d'%d for d in range(8)]+['move %d'%d for d in range(8)]+['icon']]

def check_manifest():
    have=set(tuple(l.split('\t')[1:3]) for l in manifest)
    missing=[r for r in REQUIRED if r not in have]
    if missing: raise SystemExit('manifest is missing strips: %s'%missing)
    if len(have)!=len(manifest): raise SystemExit('manifest has a strip twice')

def main():
    ap=argparse.ArgumentParser(description='Build the HD structure packs (data/hd)')
    ap.add_argument('--out',default=os.path.join(ROOT,'data','hd'))
    ap.add_argument('--preview',default=None,help='also write fit previews here')
    ap.add_argument('--only',default=None,help='one job (for trying things; the manifest then has only its strips)')
    args=ap.parse_args()
    for name,job in JOBS:
        if args.only is None or args.only==name:
            job()
    if args.only is None:
        check_manifest()
    finish(args.out,args.preview)

if __name__=='__main__':
    main()
