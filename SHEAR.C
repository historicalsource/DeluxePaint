/*--------------------------------------------------------------*/
/*			shear.c					*/
/*								*/
/*--------------------------------------------------------------*/
#include <system.h>
#include <prism.h>
#include <pnts.h>

#define local static

extern void RestoreBrush();
extern PointRec pnts;
extern BMOB curbr, origBr;
extern void (*nop)(), (*killMode)();
extern void obmv(), obclr();
extern SHORT curpen;
	
BOOL BMShearX(oldbm,newbm,oldwidth, xpcolor,dx)
    struct BitMap *oldbm, *newbm; 
    SHORT oldwidth, xpcolor, dx; 
    {
    SHORT sum,x,y,h,w,xinc;
    if (dx==0) return(CopyBitMap(oldbm,newbm)); 
    if (dx<0) { dx = -dx; xinc = -1; x = dx; }
    	else { xinc = 1; x = 0; }
    if (NewSizeBitMap(newbm, oldbm->Depth, oldwidth + dx, oldbm->Rows)) 
	return(FAIL);
    SetBitMap(newbm, xpcolor);
    h = oldbm->Rows;
    w = oldwidth;
    sum = h/2;
    for (y=0; y<h; y++) {
	BltBitMap(oldbm,0,y,newbm,x,y,w,1,REPOP,0xff,NULL);
	sum += dx;
	while (sum>h) { x += xinc; sum -= h; }
	}
    return(SUCCESS);
    }


/* ----------- Shear Brush ------------------ */


local Point bpl;
local PaintMode svPntMode;
extern PaintMode paintMode;

void killShear() {
    SetPaintMode(svPntMode);
    RestoreBrush();
    }
    
/* Begin stretching brush (button down) */

local beginshr() {
    /* remember the upper left corner */
    bpl.x = mx-curbr.xoffs;
    bpl.y = my-curbr.yoffs;
    }

local GiveUp() {   ClearFB();   RestoreBrush();   AbortIMode();   }
    
local shrob() {
    short dx,dy;
    dx = mx - bpl.x - origBr.pict.box.w;
    dy = my - bpl.y - origBr.pict.box.h;
    if (BMShearX( origBr.pict.bm, curbr.pict.bm, origBr.pict.box.w, 
    	 origBr.xpcolor, dx))	GiveUp();
    else {
	curbr.pict.box.w = origBr.pict.box.w + ABS(dx);
	if (BMOBMask(&curbr))  GiveUp();
	else {
	    curbr.xoffs = origBr.pict.box.w + MAX(dx,0); 
	    curbr.yoffs = my - bpl.y;
	    obmv();
	    }
	}
    }

void endShrob() { 
    SetPaintMode(svPntMode);
    curbr.yoffs = origBr.yoffs;
    curbr.xoffs = curbr.pict.box.w/2;
    }

    
/* first change the handle to the lower right corner for stretching */
void IMShrBrush(){  if ((curpen!=USERBRUSH)||
    	(SaveBrush(SHEARED))) {RevertIMode(); return;}
    if (CopyBMOBPict(&origBr,&curbr)) { RevertIMode(); RestoreBrush(); return;}
    curbr.xoffs = curbr.pict.box.w;  curbr.yoffs = curbr.pict.box.h;
    IModeProcs(obmv, obmv, obclr, beginshr, obmv, shrob, obclr, endShrob);
    killMode =  &killShear;
    svPntMode = paintMode;
    SetPaintMode(Mask);
    }

        
#ifdef oidoidoid
/********************************************************************/
 This code  not yet converted 
    
BMShearY(oldbm,newbm,dy) struct BitMap *oldbm, *newbm; SHORT dy; {
    SHORT sum,x,y,h,w,yinc;
    if (dy==0){ CopyBitMap(oldbm,newbm); return; }
    if (dy<0) { dy = -dy; yinc = -1; y = dy; }
    	else { yinc = 1; y = 0; }
    NewShBM(newbm, oldbm->box.w, oldbm->box.h + dy);
    BMLike(oldbm,newbm);
    FillBitMap(newbm,compcolw(oldbm->xpcolor));
    h = oldbm->box.h;
    w = oldbm->box.w;
    sum = w/2;
    for (x=0; x<w; x++) {
	rop(REPMODE,NIL,oldbm,newbm,1,h,x,0,x,y);
	sum += dy;
	while (sum>w) { y += yinc; sum -= w; }
	}
    return(SUCCESS);
    }

BMShear(obm,nbm,dx,dy) struct BitMap *obm,*nbm; SHORT dx,dy; {
    struct BitMap tmp;
    tmp.segment= 0;
    if (dx==0)  BMShearY(obm,nbm,dy);
    else if (dy==0)  BMShearX(obm,nbm,dx);
    else {
	BMShearY(obm,&tmp,dy);
	BMShearX(&tmp,nbm,dx);
	FreeBitMap(&tmp);
	}
    return(SUCCESS);
    }

#endif

