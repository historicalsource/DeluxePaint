/*--------------------------------------------------------------*/
/*		Bend.c						*/
/*--------------------------------------------------------------*/

#include <system.h>
#include <prism.h>
#include <pnts.h>

#define local static
#define lfHalf 0x8000

extern PointRec pnts;
extern SHORT curpen;
extern struct RastPort tempRP;
extern BMOB curbr,origBr;
extern void (*nop)();
extern SHORT usePen;
extern void frobsh(),frobmv(),frobclr();

local Point p0,p1,p2;
local Point q0,q1,q2;
local SHORT topY,botY,leftX,rightX;
local SHORT crx,cry;

struct BitMap *frombm, *tobm;
local BOOL isHoriz;

local void hBendOp(x,y) SHORT x,y; {
    if (y==cry) return;
    BltBitMap(frombm,0,y,tobm, x, y, origBr.pict.box.w, 1 ,REPOP,0xff,NULL);
    cry = y;
    }

local BoxFromPts(bx) Box *bx; {
    return(BoxThreePts(bx,q0.x,q0.y,q1.x,q1.y,q2.x,q2.y));
    }

/* horizontal bend */
local BOOL BMBendH() 
    {
    SHORT dx,winc;
    Box bx;
    frombm = origBr.pict.bm;  tobm = curbr.pict.bm;
    BoxFromPts(&bx);
    winc = bx.w - 1;
    if (q0.x == q2.x) {
	dx = q1.x - q0.x;
	winc = ABS(dx)/2; /* increase width by this amount */
	/* if bending to left, move box in from p1 to curve */
	if (q1.x<q0.x) bx.x  += (-dx-winc);
	}    
    if (NewSizeBitMap(tobm,frombm->Depth, frombm->BytesPerRow*8 + winc, 
	    frombm->Rows)) return(FAIL);
    SetBitMap(tobm,origBr.xpcolor);
    cry = - 1;
    PCurve(q0.x-bx.x,0,q1.x-bx.x,q1.y-q0.y,q2.x-bx.x,q2.y-q0.y,64,hBendOp);
    return(SUCCESS);
    }

void vBendOp(x,y) SHORT x,y; {
    if (x==crx) return;
    BltBitMap(frombm,x,0,tobm, x, y, 1, origBr.pict.box.h,REPOP,0xff,NULL);
    crx = x;
    }

/* vertical bend */
local BOOL BMBendV()  {
    SHORT hinc;
    Box bx;
    SHORT dy;
    frombm = origBr.pict.bm;  tobm = curbr.pict.bm;
    BoxFromPts(&bx);
    hinc = bx.h -1;
    if (q0.y == q2.y) {
	dy = q1.y - q0.y;
	hinc = ABS(dy)/2;
	if (q1.y<q0.y) bx.y  += (-dy-hinc);
	}    
    if (NewSizeBitMap(tobm,frombm->Depth, frombm->BytesPerRow*8,
    	 frombm->Rows+hinc)) return(FAIL);
    SetBitMap(tobm,origBr.xpcolor);
    crx = - 1;
    PCurve(0, q0.y - bx.y, q1.x - q0.x, q1.y - bx.y, q2.x - q0.x,
	q2.y - bx.y,64,vBendOp);
    return(SUCCESS);
    }

local XorCurve(x0,y0,x1,y1,x2,y2) SHORT x0,y0,x1,y1,x2,y2; {
    SHORT svUsePen;
    TempXOR();
    svUsePen = usePen;
    usePen = 0;
    mCurve(x0,y0,x1,y1,x2,y2);
    usePen = svUsePen;
    RestoreMode();
    }


local invHbend()  {
    SHORT dx;
    q0 = p0;    q1 = p1;    q2 = p2;
    if (my<topY) q0.x = mx;
    else if (my>botY) q2.x = mx;
    else { q1.x = mx; q1.y = my; }
    dx = origBr.pict.box.w-1;
    XorLine(q0.x - dx,q0.y, q0.x,q0.y);
    XorLine(q2.x - dx,q2.y, q2.x,q2.y);
    XorCurve(q0.x,q0.y,q1.x,q1.y,q2.x,q2.y); 
    XorCurve(q0.x - dx,q0.y,q1.x - dx,q1.y, q2.x - dx,q2.y); 
    }

local invVbend()  {
    SHORT dy;
    q0 = p0;    q1 = p1;    q2 = p2;
    if (mx<leftX) q0.y = my;
    else if (mx>rightX) q2.y = my;
    else { q1.x = mx; q1.y = my; }
    dy = origBr.pict.box.h-1;
    XorLine(q0.x, q0.y - dy, q0.x, q0.y);
    XorLine(q2.x, q2.y - dy, q2.x, q2.y);
    XorCurve(q0.x, q0.y, q1.x, q1.y, q2.x, q2.y); 
    XorCurve(q0.x, q0.y - dy, q1.x, q1.y - dy, q2.x, q2.y - dy); 
    }

/* button down proc */
    
local void  beginHbend() { 
    p0.x = mx; p0.y = my-curbr.yoffs; 
    p1.x = mx; p1.y = my;
    p2.x = mx; p2.y = p0.y+origBr.pict.box.h -1;
    topY = p0.y; 
    botY = p2.y; 
    }
    
local void  beginVbend() { 
    p0.y = my; p0.x = mx-curbr.xoffs; 
    p1.y = my; p1.x = mx;
    p2.y = my; p2.x = p0.x+origBr.pict.box.w -1;
    leftX = p0.x; 
    rightX = p2.x; 
    }
    
local BOOL (*bendbm)();

local endbend() {  /* button up proc */
    ZZZCursor();
    if ((*bendbm)())
	{ /* memory allocation failure or abort */
	RestoreBrush();
	AbortIMode();
	}
    else { 
	curbr.pict.box.h = curbr.pict.bm->Rows;
	curbr.pict.box.w = curbr.pict.bm->BytesPerRow*8;
	curbr.xoffs = curbr.pict.box.w/2; /* not correct but ... */
	curbr.yoffs = curbr.pict.box.h/2;
	if (isHoriz) curbr.yoffs = origBr.yoffs;
	else curbr.xoffs = origBr.xoffs; 
	if (BMOBMask(&curbr)) {   RestoreBrush();   AbortIMode();   }
	}
    UnZZZCursor();
    }

local BOOL PrepBend() {
    if (curpen != USERBRUSH)  return(FAIL); 
    if (SaveBrush(BENT)) return(FAIL);
    if (CopyBMOBPict(&origBr,&curbr)==FAIL) {RestoreBrush(); return(FAIL); }
    origBr.xoffs = curbr.xoffs; origBr.yoffs = curbr.yoffs; 
    curbr.xoffs = curbr.pict.box.w-1;  curbr.yoffs = curbr.pict.box.h-1;
    return(SUCCESS);
    }

void IMHBendBrush(){
    if (PrepBend()==FAIL) { RevertIMode(); return; }
    curbr.yoffs>>=1;
    isHoriz = YES;
    bendbm = &BMBendH;
    IModeProcs(frobsh,frobmv,frobclr,beginHbend,invHbend,nop,invHbend,endbend);
    }


void IMVBendBrush() {
    if (PrepBend()==FAIL) { RevertIMode(); return; }
    curbr.xoffs >>=1;
    isHoriz = NO;
    bendbm = &BMBendV;
    IModeProcs(frobsh,frobmv,frobclr,beginVbend,invVbend,nop,invVbend,endbend);
    }

