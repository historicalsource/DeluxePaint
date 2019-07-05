/*----------------------------------------------------------------------*/
/*  									*/
/*        curbrush.c -- The current brush & pen				*/
/*  									*/
/*----------------------------------------------------------------------*/
#include <system.h>
#include <prism.h>

extern short xShft, yShft;
extern Box screenBox;
extern SHORT prevColors[];
extern SHORT LoadBrColors[32];
extern struct BitMap brBM,brSVBM;
extern BMOB curbr;
extern SHORT curpen;
extern struct BitMap penBM,penSVBM;
extern BMOB curpenob;
extern BOOL modeHelp;

#define local static 

BOOL LegalMode(i) int i; {
    if (modeHelp)
	return((BOOL)! ( (curpen!=USERBRUSH) && ((i==Mask)||(i==Replace)) ) );
    else  return((BOOL)YES);
    }

BOOL BrushDefined() { return((BOOL)(curbr.pict.bm->Planes[0] != NULL)); }

UseBrPalette() { if (curpen==USERBRUSH) {
	GetColors(prevColors);
	LoadCMap(LoadBrColors);
	}
    }
    
/* Pens have only a MASK and no source bit-planes.  FixUpPen
   fixes pen data structure to reflect this */
local FixUpPen() {
    curpenob.xpcolor = 0;
    curpenob.minTerm = COOKIEOP;
    DFree(curpenob.mask);
    curpenob.mask = penBM.Planes[0];
    penBM.Planes[0] = (APTR)0;
    penBM.Depth = 0;
    curpenob.planeUse = 0;
    curpenob.planeDef = 0; 
    curpenob.xoffs = curpenob.pict.box.w/2;
    curpenob.yoffs = curpenob.pict.box.h/2;
    }

local SzPen() { return(NewSizeBitMap(curpenob.pict.bm,1,
	    curpenob.pict.box.w,curpenob.pict.box.h));}

local SizePenBM() {
    if (SzPen()) {
	FreeBMOB(&curbr);
	if (SzPen()) return(FAIL);
	}
    ClearBitMap(&penBM);
    return(SUCCESS);
    }

/* ------------- Circular Pen radius r: 
    r is in VDC (virtual device coords )
*/ 
RoundPen(r) SHORT r; {
    struct RastPort crp;
    SHORT rx,ry;
    rx = PMapX(r);
    ry = MAX(PMapY(r),1);
    MakeBox(&curpenob.pict.box,0,0,2*rx+1,2*ry+1);
    InitRastPort(&crp);
    SizePenBM();
    crp.BitMap = &penBM;
    SetAPen(&crp,15);
    PushGrDest(&crp,&screenBox); /* any large box with x=y=0 will do */
    PFillCirc(rx,ry,rx);
    PopGrDest();
    FixUpPen();
    }

OneBitPen() {
    MakeBox(&curpenob.pict.box,0,0,1,1);
    NewSizeBitMap(&penBM,1,1,1);
    *((SHORT *)penBM.Planes[0]) = 0x8000;
    FixUpPen();
    }

    
/*---------------- Square Pen side s ------------- */ 
SquarePen(s) SHORT s; {
    struct RastPort crp;
    SHORT pw = PMapX(s);
    SHORT ph = PMapY(s);
    MakeBox(&curpenob.pict.box,0,0,pw,ph);
    InitRastPort(&crp);
    SizePenBM();
    crp.BitMap = &penBM;
    SetAPen(&crp,15);
    WaitBlit();
    RectFill(&crp,0,0,pw-1,ph-1);
    FixUpPen();
    }

#define NPENS	9

#ifdef fromprism_h
/* Pen TYPE ENcoding */
#define USERBRUSH -1
#define ROUND_B		1
#define SQUARE_B	2
#define DOT_B		3
#define AIR_B		4    
#define RoundB(n)	((ROUND_B<<12)|n)
#define SquareB(n)	((SQUARE_B<<12)|n)
#define DotB(n)		((DOT_B<<12)|n)
#define AirB		AIR_B<<12

#endif



/* dot pens ---------------- */
extern UWORD dots10[],dots20[],dots30[],dots40[],dots50[],dots60[];
UWORD *dotbms[] = { dots10, dots20, dots30, dots40, dots50, dots60};

DotBFromRad(r) int r; {  return( DotB(r/2));  }

DotsPen(s) int s; {
    int pensz;
    s = MIN(6, MAX(1,s));
    pensz = 4*s+1;
    MakeBox(&curpenob.pict.box,0,0,pensz,pensz);
    SizePenBM();
    movmem(dotbms[s-1], penBM.Planes[0], BytesNeeded(pensz)*pensz);
    FixUpPen();
    }

/** Select a builtin Pen --------------- */
        
void SelPen(n) int n; {
    int type,size;
    if (n==USERBRUSH) { 
	if (!BrushDefined()) return; 
	else UseBrush(); 
	}
    else {
	if (n==0) OneBitPen(); 
	else {
	    type = (n>>12)&0xf;
	    size = n&0xfff;
	    switch(type) {
		case ROUND_B: size = MIN(size,50); RoundPen(VMapY(size)); break;
		case SQUARE_B: size = MIN(size,100); SquarePen(VMapY(size)); break;
		case DOT_B: DotsPen(size); break;
		default: break;
		}
	    n = (n&0xf000)|size;
	    }
	if (curpen==USERBRUSH){
	    curpen=n;	/* have to do this first so LegalMode() works right*/
	    if (modeHelp) SetModeMenu(Color);
	    }
	UsePen();
	}
    curpen = n;
    FixUsePen();	/* this updates the usePen variable*/
    ResetCursor();
    }

