/*----------------------------------------------------------------------*/
/*									*/
/*     	  pgraph.c  -- prism graphics primitives			*/
/*									*/
/*	routines starting with "P" take Physical Device Coords		*/
/*	routines starting with "V" take Virtual Device Coords		*/
/*									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>

#define local static

extern SHORT xShft,yShft;

/* ------  The Current Destination RastPort and Clip Box ---- */
#define NGRSTACK 4

struct RastPort *rpStack[NGRSTACK];
Box clbStack[NGRSTACK];
struct RastPort *curRP = NULL;


Box bigBox = {-1000,-1000, 2000, 2000};

Box clipBox = {0};  /* THIS IS IN SCREEN COORDS */
BoxBM canvas = { {0,0,400,640},NULL};
SHORT grStPtr = 0;
SHORT clipMaxX=0,clipMaxY=0;
    
SetClipBox(r) Box *r; {
    clipBox = *r;
    clipMaxX = clipBox.x + clipBox.w - 1;
    clipMaxY = clipBox.y + clipBox.h - 1;
    }

PushGrDest(rp,clBox) struct RastPort *rp; Box *clBox; {
    rpStack[grStPtr] = curRP;
    clbStack[grStPtr] = clipBox;
    curRP = rp;
    canvas.bm = rp->BitMap;
    SetClipBox(clBox);
    grStPtr++;
    }
    
PopGrDest() {
    grStPtr--;
    curRP = rpStack[grStPtr];
    canvas.bm = curRP->BitMap;
    SetClipBox(&clbStack[grStPtr]);
    }

/* ------------	 MultiColor Fill Pattern -------------	*/
UWORD *curFilPat = NULL;
SetFPat(pattern) UWORD *pattern; { curFilPat = pattern; }
        
/* ------------	 Current Pattern ------------------*/
#define PATSIZE 3
UWORD solidpat[1]={0xffff};
UWORD checkpat[8]={0xaaaa,0x5555,0xaaaa,0x5555,0xaaaa,0x5555,0xaaaa,0x5555};
SetPattern(pattern) USHORT *pattern; {
    SetAfPt(curRP, pattern, (pattern==solidpat)? 0: PATSIZE);  
    }

SolidPattern() { SetPattern(solidpat); }

/* ------------	 Current Pattern, FGCol, BGCol ------	*/
SetFGCol(c) int c; { SetAPen(curRP,c); }
SetBGCol(c) int c; { SetBPen(curRP,c); }

/* general subroutine to replace one pixel value with another in table */
/* replpix(xtab,oldpx,newpx) UBYTE *xtab; UBYTE oldpx,newpx; {    } */

InitPGraph() { 
    curRP = NULL;
    grStPtr = 0;
    }
    	
/* --------- the Paint Mode: how are bits combined in painting ----*/
PaintMode paintMode=Color, svPntMd=Color;
BYTE svmode,svfgpen;
UBYTE curMinterm = COOKIEOP;
SHORT curxpc = 0; /* the current transparent color */
USHORT *svpat;

UBYTE drModes[NPaintModes] = {
	JAM2,		/* Mask */
	JAM2,		/* Color */
	JAM2,		/* Replace */
	JAM2,		/* Smear */
	JAM2,		/* Shade */
	JAM2,		/* Blend */
	JAM2,		/* Cycle Paint*/
	COMPLEMENT	/* Xor */
	};	

void PWritePix(), PShadePix(), PSmearPix(), PBlendPix();

void (*CurPWritePix)();
void (*wrPixTable[])() = { PWritePix,PWritePix,PWritePix,
	PSmearPix , PShadePix,  PBlendPix, PWritePix, PWritePix};

void SetPaintMode(mode) PaintMode mode; {
    paintMode = mode;
    SetDrMd(curRP,drModes[mode]);
    CurPWritePix = wrPixTable[mode];
    curMinterm = (mode==Xor)? XORMASK: COOKIEOP;
    }

/**--------- Temporary Erase mode  for right button -----*/

BOOL erasing = NO;

JamSvPntMd(md) SHORT md;{ svPntMd = md; }

TempErase() { 
    SetDrMd(curRP,JAM2);
    svfgpen = curRP->FgPen;
    SetAPen(curRP,curxpc);
    erasing = YES;
    }
    
ClearErase() { 
    if (erasing) {
	SetDrMd(curRP,drModes[paintMode]);
	SetAPen(curRP,svfgpen);
	erasing = NO;  
	}
    }

/* The transparent color */
SetXPCol(col) SHORT col; {   curxpc = col;    }
PaintMode tsvPntMd;
SHORT tsvfgpen;
#define WHITE 31

TempMode(md,fg,ptrn)   PaintMode md; SHORT fg; USHORT *ptrn; {
    tsvfgpen = curRP->FgPen;
    svpat = curRP->AreaPtrn;
    tsvPntMd = paintMode;
    SetAPen(curRP,fg);
    SetPaintMode(md);
    SetPattern(ptrn);
    }
	
TempXOR() { TempMode(Xor,WHITE,solidpat);  }

RestoreMode() {
    SetAPen(curRP,tsvfgpen);
    SetPaintMode(tsvPntMd);
    SetPattern(svpat);
    }
    
/*------------------ Graphical Operations ----------------------*/
/* operations beginning with "V" take Virtual coordinates     	*/
/* operations beginning with "P" take Physical coordinates    	*/
/*--------------------------------------------------------------*/

#ifdef THEIRWAY
#define FixX(x)   ( (x) - clipBox.x )
#define FixY(y)   ( (y) - clipBox.y )
#else
#define FixX(x)   (x) 
#define FixY(y)   (y) 
#endif

/*-----	Physical Coordinate write pixel 			*/
void PWritePix(x,y) int x,y; {
#ifndef THEIRWAY
    if (BoxContains(&clipBox,x,y))
#endif
	WritePixel(curRP,FixX(x),FixY(y));
    }

/*----- Physical Coordinate read pixel */   
int PReadPix(x,y) int x,y; { return(ReadPixel(curRP,FixX(x),FixY(y)));  }

TempColWrPix(x,y,col) SHORT x,y,col; {
    SHORT sv = curRP->FgPen;
    SetAPen(curRP,col);
    PWritePix(x,y);
    SetAPen(curRP,sv);
    }
	    
extern Range *shadeRange, cycles;
	        
void PShadePix(x,y) {
    SHORT newc = PReadPix(x,y);
    if (erasing) {
	if (shadeRange)   if ((newc<=cycles.low)||(newc>cycles.high)) return;
	newc--;
	}
    else {
	if (shadeRange)  if ((newc<cycles.low)||(newc>=cycles.high)) return;
	newc++;
	}
    TempColWrPix(x,y,newc);
    }

extern BOOL firstDown;
SHORT smlast;
void PSmearPix(x,y) {
    SHORT sv = PReadPix(x,y); 
    if (!firstDown) TempColWrPix(x,y,smlast);
    else firstDown = NO;
    smlast = sv;
    }

local BOOL lastIn;
void PBlendPix(x,y) { 
    SHORT sv = PReadPix(x,y);
    BOOL thisIn = (shadeRange==0)||(sv>=cycles.low)&&(sv<=cycles.high);
    if (!firstDown) {
	if (thisIn&&lastIn)
	TempColWrPix(x,y,(sv+smlast)/2); 
	}
    else firstDown = NO;
    smlast = sv;
    lastIn = thisIn;
    }

/* ------------ Horizontal line ---------------- */

void PHorizLine(x1,y,w) SHORT x1,y,w; {
#ifdef THEIRWAY
    x1 -= clipBox.x;    y -= clipBox.y;
#endif
    Move(curRP,x1,y); Draw(curRP,x1+w-1,y);
    } 

void PatHLine(x1,y,w) int x1,y,w; {
    int x2;
#ifdef THEIRWAY
    x1 -= clipBox.x;    y -= clipBox.y;
#endif
    if ((y<clipBox.y)||(y>clipMaxY)) return;
    x2 = x1 + w -1;
    x1=MAX(x1,clipBox.x); x2 = MIN(x2,clipMaxX);
    if (x1>x2) return;
    HLineBlt(curRP,x1,y,x2,curMinterm);
    } 

/* ------------ Vertical line ---------------- */
void PVertLine(x1,y,h) SHORT x1,y,h; {
#ifdef THEIRWAY
    x1 -= clipBox.x;    y -= clipBox.y;
#endif
    if ((x1>= clipBox.x)&&(x1<=clipMaxX)) {
      Move(curRP,x1,y); Draw(curRP,x1,y+h-1);
	}
    } 

#ifdef THEIRWAY
PFillBox(b) Box *b;{
    RectFill(curRP, b->x- clipBox.x, b->y - clipBox.y, b->x + b->w - 1, b->y + b->h - 1);  
    }
#else
PFillBox(b) Box *b; {
    Box c;
    if (BoxAnd(&c,b,&clipBox)) 
	RectFill(curRP, c.x, c.y,c.x+c.w-1,c.y+c.h-1);
    }
#endif

PPatternBox(b,pat,fg,bg,op) Box *b; SHORT *pat,fg,bg,op; {
    USHORT *svpat,svfg,svbg,svop;
    svpat = curRP->AreaPtrn;
    svfg = curRP->FgPen;
    svbg = curRP->BgPen;
    svop = curRP->AOlPen;
    SetPattern(pat);
    SetFGCol(fg);
    SetBGCol(bg);
    SetOPen(curRP,op);
    PFillBox(b);
    SetPattern(svpat);
    SetFGCol(svfg);
    SetBGCol(svbg);
    SetOPen(curRP,svop);
    BNDRYOFF(curRP);
    }
  
PThinFrame(bx) Box *bx;{
    PatHLine(bx->x,bx->y,bx->w);
    PatHLine(bx->x,bx->y+bx->h-1,bx->w);
    PVertLine(bx->x,bx->y+1, bx->h-2);
    PVertLine(bx->x + bx->w -1,bx->y+1, bx->h-2);
    }

PInvertBox(b) Box *b; {
    TempXOR();
    PFillBox(b);
    RestoreMode();
    }

PInvertXHair(x,y) int x,y; {
    TempXOR();
    PHorizLine(clipBox.x,y,clipBox.w);
    PVertLine(x,clipBox.y,clipBox.h);
    RestoreMode();
    }

PFloodArea(x,y) {
    FillArea(x,y); 
    
/*  Flood(curRP, 1, FixX(x), FixY(y));  */
    }

/* set to flood area now */
PFloodBndry(x,y) { Flood(curRP, 1, FixX(x), FixY(y)); }

