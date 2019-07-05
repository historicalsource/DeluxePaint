/*--------------------------------------------------------------*/
/*		rotate.c					*/
/*	Arbitrary rotation of current object			*/
/*								*/
/*	9-8-85	DDS	Ported to Amiga (68000)			*/
/*--------------------------------------------------------------*/

#include <system.h>
#include <prism.h>
#include <pnts.h>


#define local 
#define lfHalf 0x8000

extern PointRec pnts;
extern SHORT curpen;
extern LongFrac LFMULT();
extern struct RastPort tempRP;
extern BMOB curbr,origBr;
extern void (*nop)();
extern SHORT usePen;

#define highHalf(lf)  ( (lf)>>16)

typedef union { LongFrac lf; SHORT s[2]; } lfOvl;

#ifdef DBGROTATE
local BOOL rotdbg = 0;
#endif

#define longf(x) ((x)<<16)

/* Size new object for rotation */

local BMOBRotSize(old,new,sin,cos) 
    BMOB *old,*new; 
    LongFrac sin,cos; 
    {
    SHORT  w,h,neww,newh;
    w = old->pict.box.w;
    h = old->pict.bm->Rows;
    newh = highHalf(h*ABS(cos) + w*ABS(sin) + lfHalf);
    neww = highHalf(w*ABS(cos) + h*ABS(sin) + lfHalf);
    return(BMOBNewSize(new,old->pict.bm->Depth,neww,newh));
    }

local Point pin;

/* rotate bitmap: assumes nbm is already sized and allocated */

local BMRot(obm,nbm,sin,cos,width,xpcol) 
    struct BitMap *obm,*nbm;
    LongFrac sin,cos; 
    SHORT width,xpcol;
    {
    SHORT  w,h,neww,newh,xn,yn,xi,yi,pix;
    LongFrac xs,ys,x,y;
    struct RastPort destRP;
    InitRastPort(&destRP);
    destRP.BitMap = nbm;
    SetDrMd(&destRP,JAM1);
    w = width;
    h = obm->Rows;
    newh = highHalf(h*ABS(cos) + w*ABS(sin) + lfHalf);
    neww = highHalf(w*ABS(cos) + h*ABS(sin) + lfHalf);

#ifdef DBGROTATE
    if (rotdbg) printf(" sin = %lx, cos = %lx \n",sin,cos);
    if (rotdbg) printf("w=%d,h=%d, neww=%d, newh=%d\n",w,h,neww,newh);
#endif

    SetBitMap(nbm,xpcol);  /* watch out-- this uses tempRP */
    tempRP.BitMap = obm;
    if (sin>=0) 
	if (cos>=0) { xs = -h*LFMULT(sin,cos);   ys =  h*LFMULT(sin,sin);  }
	else {  xs = w*LFMULT(cos,cos);   ys = longf(h) - w*LFMULT(sin,cos);  }
    else 
	if (cos>=0) { xs = w*LFMULT(sin,sin);  ys = w*LFMULT(sin,cos);  }
	else {   xs = longf(w) + h*LFMULT(sin,cos);  ys = h*LFMULT(cos,cos);  }
    xs += lfHalf;
    ys += lfHalf;
    for (yn=0; yn<newh; yn++) {
	x = xs;	y = ys;
	for (xn = 0; xn<neww; xn++) {
	    xi = highHalf(x);
	    yi = highHalf(y);
	    if ((xi>=0)&&(xi<w)&&(yi>=0)&&(yi<h)) {
		pix = ReadPixel(&tempRP,xi,yi);
		SetAPen(&destRP,ReadPixel(&tempRP,xi,yi));
		WritePixel(&destRP,xn,yn);
		}
	    x += cos;   y -= sin;
	    }
	if (CheckAbort()) return(FAIL);
	xs += sin; ys += cos;
	}
    return(SUCCESS);
    }


#define AssgnLfrac(plf,hi,lo)   ((SHORT *)plf)[0] = hi; ((SHORT *)plf)[1] = lo

local sincos(dx,dy,sn,cs) SHORT dx,dy; LongFrac *sn,*cs; {
    SHORT hyp = Distance(dx,dy);
    AssgnLfrac(sn,dy,0);
    AssgnLfrac(cs,dx,0);
    *sn /=hyp;
    *cs /=hyp;
#ifdef DBGROTATE
    if (rotdbg) printf("Sincos: dx= %d, dy = %d, sn = %lx, cs = %lx \n",
        dx,dy,*sn,*cs);
#endif
    }

XorLine(x0,y0,x1,y1) SHORT x0,y0,x1,y1; {
    SHORT svUsePen;
    TempXOR();
    svUsePen = usePen;
    usePen = 0;
    mLine(x0,y0,x1,y1);
    usePen = svUsePen;
    RestoreMode();
    }
    
#define lfmulint(lf,i) (highHalf((lf)*(i)+lfHalf))

local XorRotB1(x,y,w,h) SHORT x,y,w,h; {
    LongFrac sn,cs;
    SHORT x1,y1,x2,y2,x3,y3;
#ifdef DBGROTATE
    if (rotdbg) printf("XorRotBox: x,y,w,h = %d, %d, %d, %d \n",x,y,w,h);
#endif
    sincos(x-pin.x,y-pin.y,&sn,&cs);
    x1 = pin.x + lfmulint(w,cs);
    y1 = pin.y + lfmulint(w,sn);
    x2 = x1 + lfmulint(h,sn);
    y2 = y1 + lfmulint(-h,cs);
    x3 = pin.x + lfmulint(h,sn);
    y3 = pin.y + lfmulint(-h,cs);
    XorLine(pin.x,pin.y,x1,y1);
    XorLine(x1,y1,x2,y2);
    XorLine(x2,y2,x3,y3);
    XorLine(x3,y3,pin.x,pin.y);
    }

local XorRotBox(x,y) SHORT x,y; {
    XorRotB1(x,y,curbr.pict.box.w,curbr.pict.box.h);
    }

/* button down proc */
local beginrot() { pin.x = mx-curbr.xoffs; pin.y = my;   }
    
local rotob() { XorRotBox(lx,ly);  XorRotBox(mx,my); }
    
local invrotb() { XorRotBox(mx,my); }

local void endrot() {  /* button up proc */
    LongFrac sn,cs;
    sincos( mx-pin.x, my-pin.y, &sn, &cs);
    if (BMOBRotSize(&origBr, &curbr, sn, cs)) { 
	RestoreBrush();
	AbortIMode();
	return;
	}
    ZZZCursor();
    BMRot( origBr.pict.bm, curbr.pict.bm, sn, cs, origBr.pict.box.w, origBr.xpcolor); 
    BMOBMask(&curbr); 
    curbr.pict.box.h = curbr.pict.bm->Rows;
    curbr.pict.box.w = curbr.pict.bm->BytesPerRow*8;
    curbr.xoffs = curbr.pict.box.w/2; /* not correct but ... */
    curbr.yoffs = curbr.pict.box.h/2;
    UnZZZCursor();
    }

local invbrbox(x,y) SHORT x,y; {
    Box bx;
    TempXOR();
    mFrame(MakeBox(&bx,x-curbr.xoffs,y-curbr.yoffs,
    	curbr.pict.box.w,curbr.pict.box.h));
    RestoreMode();
    }
    
local frobsh() { obmv();  invbrbox(mx,my); }

local frobmv() { invbrbox(lx,ly);  frobsh(); }

local frobclr() {  invbrbox(mx,my);  obclr();  }

void IMRotBrush(restart) BOOL restart; {
    if (!restart) {
	if (curpen != USERBRUSH) { RevertIMode(); return; }
	if (SaveBrush(ROTATED)) { RevertIMode(); return; }
	if (CopyBMOBPict(&origBr,&curbr)==FAIL) {RestoreBrush(); 
		RevertIMode(); return; }
	origBr.xoffs = curbr.xoffs; origBr.yoffs = curbr.yoffs; 
	curbr.xoffs = curbr.pict.box.w;  curbr.yoffs = curbr.pict.box.h;
	}
    IModeProcs(frobsh,frobmv,frobclr,beginrot,invrotb,nop,invrotb,endrot);
    }

