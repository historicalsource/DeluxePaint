/*----------------------------------------------------------------------*/
/*  									*/
/*        polyf.c -- Filled Polygon drawing				*/
/*  									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>
#include <pnts.h>

#define local static
extern struct RastPort *curRP;
extern BOOL  goToPerm, symON, abortFlag;
extern void (*doit)(), (*xorit)(), (*nop)(), (*killMode)();
extern void FlipIt(), PaintSym(), obmv(), obclr(), vec(), EraseTempChg();
extern PointRec pnts;
extern Point bp;
extern SHORT butwas,usePen;
extern Box clipBox;
extern SHORT clipMaxX, clipMaxY;
extern SHORT xorg,yorg;
extern void (*completeMode)();

#define eraseFB EraseTempChg

local void dotmv() {mDrawDot(mx,my); }

local void chvec() {   mLine(pex,pey,mx,my); }

local void thinvec() {
    SHORT svusePen = usePen; usePen = 0; vec();
    usePen =svusePen; 
    }

local void thinChVec() {
    SHORT svusePen = usePen; usePen = 0; chvec();
    usePen =svusePen; 
    }


/* Filled Polygon ==========================================================*/

/*---------------Magnified Area Flood ------------*/
mFillPoly(points,np) Point *points; int np; {
    SHORT xmin,xmax,ymin,ymax,px,py;
    int i;
    Box b;
    xmin = xmax = points->x;
    ymin = ymax = points->y;
    UpdtON();
    ClipInit(clipBox.x,clipBox.y, clipMaxX, clipMaxY );
    ClipMove(curRP,points[np-1].x+xorg, points[np-1].y+yorg, bp.y);
    for (i=0; i<np; i++) {
	px = points->x;
	py = points->y;
	ClipDraw(curRP, px+xorg, py+yorg);
	xmin = MIN(xmin,px); xmax = MAX(xmax,px);
	ymin = MIN(ymin,py); ymax = MAX(ymax,py);
	points++;
	}
    ClipEnd(curRP);
    UpdtSibs(MakeBox(&b,xmin,ymin,xmax-xmin+1, ymax-ymin+1));
    UpdtOFF();
    }


#define MAXPOLPNTS  50

extern SHORT nPolPnts;
extern Point *polPnts;
extern struct AreaInfo areaInfo;
extern WORD *areaBuff;
    
void RecPnt(x,y) int x,y; {
    if (nPolPnts>=MAXPOLPNTS) return;
    polPnts[nPolPnts].x = x;
    polPnts[nPolPnts].y = y;
    nPolPnts++;
    }

FreePoly() { DFree(polPnts);  polPnts = NULL; DFree(areaBuff); 
	areaBuff = NULL; }
    
void killFPoly() {
    int i; 
    SHORT svusePen = usePen;
    usePen = 0;
    TempXOR();
    for (i=0; i<nPolPnts; i++) {
	mLine(bp.x,bp.y, polPnts[i].x, polPnts[i].y);
	bp.x = polPnts[i].x;
	bp.y = polPnts[i].y;
	}
    FreePoly();
    RestoreMode();
    usePen =svusePen; 
    }

local void drPoly() { mFillPoly(polPnts,nPolPnts);}

local void fpolyBup() {  
    if ((nPolPnts>1)&&(Distance(mx-bp.x,my-bp.y) < 3)) {
	goToPerm = YES;
	mx = bp.x; my = bp.y;
	RecPnt(mx,my);
	if (symON){
	    abortFlag = NO;
	    SymDo(nPolPnts, polPnts, drPoly);
	    CycPaint();
	    }
	else drPoly();
	FreePoly();
	}
    else {
	FlipIt();	/* display the current line */
	RecPnt(mx,my);  /* record the point */
	}
    } 

void complPoly() {
    mx = bp.x; my = bp.y;
    fpolyBup();
    }

XorMode(xorproc,down,up) void (*xorproc)(), (*down)(), (*up)(); {
    xorit = xorproc;
    IModeProcs(FlipIt,nop,FlipIt,down,FlipIt,nop,FlipIt, up);
    }
    
void IMFPoly2() {
    xorit = thinChVec;
    killMode = killFPoly;
    completeMode = complPoly;
    XorMode(thinChVec,nop,fpolyBup);
    }

local void markFStart() {
    bp.x = mx; bp.y = my; 
    RecPnt(mx,my);
    xorit = thinvec;
    }

void IMFPoly(restart) BOOL restart; {
    killMode = killFPoly;
    if (!restart) {
	polPnts = (Point *)ChipAlloc(MAXPOLPNTS*sizeof(Point));
	areaBuff = (WORD *)ChipAlloc(MAXPOLPNTS*5);
	nPolPnts = 0;
	InitArea(&areaInfo, areaBuff, MAXPOLPNTS);
	curRP->AreaInfo = &areaInfo;
	}
    XorMode(dotmv,markFStart,fpolyBup);
    }

