/*----------------------------------------------------------------------*/
/*  									*/
/*        polyh.c -- Hollow Polygon 					*/
/*  									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>
#include <pnts.h>

#define local static

extern BOOL  goToPerm;
extern void (*doit)(), (*xorit)(), (*nop)();
extern void FlipIt(), PaintSym(), obmv(), obclr(), vec(), EraseTempChg();
extern PointRec pnts;
extern SHORT usePen, didType;

#define eraseFB EraseTempChg

/* Hollow Polygon ================================================================*/
local Point bp, prevp;

UndoHPoly() {   pex = prevp.x; pey = prevp.y;   }
    
local void chvec() {   mLine(pex,pey,mx,my); }
local void thinChVec() {
    SHORT svusePen = usePen; usePen = 0; chvec();
    usePen =svusePen; 
    }

local void polyBup() {  
    if (Distance(mx-bp.x,my-bp.y) < 3) {
	goToPerm = YES;
	mx = bp.x; my = bp.y;
	}
    didType = DIDHPoly;
    PaintSym();
    prevp.x = pex;
    prevp.y = pey;
    } 

void IMPoly2() {
    xorit = thinChVec;
    doit = chvec;
    UpdtOFF();
    IModeProcs(FlipIt,nop,FlipIt,nop,chvec,nop,eraseFB, polyBup);
    }
    
local void markStart() {bp.x = mx; bp.y = my; }

void IMPoly() {
    doit = &vec;
    IModeProcs(obmv,obmv,obclr,markStart,vec,nop,eraseFB,PaintSym);
    }

