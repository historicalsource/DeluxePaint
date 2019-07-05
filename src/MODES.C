/*----------------------------------------------------------------------*/
/*  									*/
/*        modes.c -- Painting Modes 1					*/
/*  									*/
/*----------------------------------------------------------------------*/
#include <system.h>
#include <prism.h>
#include <pnts.h>

#define local static

extern NewMode();
extern struct RastPort *curRP;
extern Box clipBox, screenBox;
extern struct BitMap hidbm;
extern SHORT Distance();
extern SHORT(*nop)();
extern void (*killMode)();
extern void (*WentDn)(), (*WentUp)();
extern UWORD curIMFlags;
extern SHORT curIMode,paintMode;
extern PointRec pnts; /* contains mx,my,sx,sy, etc */
extern void ClearXY();
extern SHORT curpen,usePen;
extern SHORT button,butwas;
extern BOOL Painting;
extern SHORT curbgcol;
extern BMOB curpenob,curbr;
extern BOOL gridON,everyTime,wasShift;
extern SHORT xorg,yorg; /* These shouldn't really be referenced here */
extern void mDispOB(), mMoveOB();
extern BOOL goToPerm,loadedBrush;
extern Box mainBox;
extern SHORT LoadBrColors[];
extern SHORT xShft,yShft;
extern IModeDesc imodes[];
extern SHORT lastPermMode;

extern void SymPts();  /* forward declaration */

SHORT npts = 6;
BOOL symON = NO;
BOOL abortFlag = NO;
Point bp;

BMOB *curob;  

/* make a box with the start button point and current point as corners*/
Box  *BoxFromSM(bx) Box *bx; { BoxTwoPts(bx,sx,sy,mx,my); }

SHORT RadSM() { return((SHORT) PMapX(Distance(VMapX(mx-sx),VMapY(my-sy)))); }


/* Atomic graphic functions that happen with mouse motion and button events */
    
extern void EraseTempChg();

#define eraseFB	   EraseTempChg

void obsplat() { mDispOB(mx,my); }
void obmv() { mMoveOB(mx,my); }
void obclr() { mClearOB(mx,my); }

local void incline( ) {mLine(lx,ly,mx,my); }
void vec() { mLine(sx,sy,mx,my); }

local void crv() { mCurve(psx,psy,mx,my,pex,pey); }

local void cir() {mCircle(sx,sy,RadSM());}
local void fcir() {mFillCircle(sx,sy,RadSM());}

local void doval() { mOval(sx,sy,mx,my); }
local void foval() { mFillOval(sx,sy,mx,my);}


local void invXH() { minvXHair(mx,my); }

showXHAndOb() { obmv(); invXH(); }
clrXHAndOb() {minvXHair(mx,my); obclr();  }
movXHAndOb() {minvXHair(lx,ly); obmv(); minvXHair(mx,my); }

UsePen() { curob = &curpenob; }
UseBrush() { curob = &curbr; }

local BOOL abortable = YES;
DisableAbort() { abortable = NO; }
EnableAbort() { abortable = YES; }
 
BOOL CheckAbort() { 
    if (AbortKeyHit()&&abortable){
	    /*ClearPushB();*/  UnDisplay(); abortFlag = YES;}
    return(abortFlag); 
    }

/*------------ Misc functionals ----*/
void (*doit)(),(*xorit)();    
void PaintIt() { UpdtON(); (*doit)(); UpdtOFF(); CycPaint(); }
void PaintSym() { UpdtON(); SymPts(); UpdtOFF(); }
void FlipIt() { TempXOR(); (*xorit)(); RestoreMode(); }

/*-------------- Symmetry ------------------*/

SymState(s) BOOL s; { if (symON!=s) TogSymSt(); }
TogSymSt() { symON = !symON; CPInvAct(symAct); }
TurnSymOn() { if (!symON) TogSymSt(); }

void SymPts() {
    abortFlag = NO;
    if (symON) SymDo(npts,&pnts, doit);  else { (*doit)();} 
    CycPaint();
    }

/*========Some "pre-fabricated" mode types =============*/

SimpleOps(atproc) void (*atproc)(); { WentDn = atproc; }

/* ------ 
    is up: cursor;
    is down: paint(atproc);
-------*/
PaintWith(atproc) void (*atproc)(); {
    doit = atproc;
    IModeProcs(obmv,obmv,obclr,SymPts,nop,SymPts,nop,nop);
    }
    
/* ------ 
    is up: brush;
    is down: current geom; 
    goes up: paint(dof);
-------*/
VTypeOps(shof,clrf,dof) void (*shof)(),(*clrf)(),(*dof)(); {
    doit = dof; 
    UpdtOFF();
    IModeProcs(obmv,obmv,obclr,nop,shof,nop,clrf,PaintSym);
    }

/* ------ 
    is up: crosshair;
    is down: xorfeedback(xorf); 
    goes up: paint(dof);
-------*/
XHTypeOps(xorf,dof) void (*xorf)(),(*dof)(); {
    void (*afunc)() = (curIMFlags&NOSYM)? &PaintIt: &PaintSym;
    doit = dof;  xorit=xorf; 
    IModeProcs(&invXH,nop,&invXH,nop,&FlipIt,nop,&FlipIt,afunc);
    }

/* This shows the object AND the XHairs during button up */
CircTypeOps(shof,clrf,dof)  void (*shof)(),(*clrf)(),(*dof)(); {
    doit = dof; 
    UpdtOFF();
    IModeProcs(showXHAndOb,movXHAndOb,clrXHAndOb,nop,shof,nop,clrf,PaintSym);
    }

FCircTypeOps(shof,clrf,dof) void (*shof)(),(*clrf)(),(*dof)(); {
    doit = dof;
    IModeProcs(invXH,nop,invXH,nop,shof,nop,clrf,PaintSym);
    }


/* ---  Mode Specifications ----*/

void IMDraw() { PaintWith(incline);}

void IMShade() {PaintWith(obsplat);}


/* ------ Vector  Mode ------------------- */

void IMVect() { VTypeOps(vec,eraseFB,vec); }

/* Conic Curve ===========================================================*/
clrcrv() { RecTempChg(&screenBox); EraseTempChg();  }

void IMCurve1(){ UpdtOFF();
    IModeProcs(obmv,obmv,obclr,nop,vec,nop,eraseFB,nop);
    }

void IMCurve2(){  doit = &crv;
    IModeProcs(crv,nop,clrcrv,nop,crv,nop,clrcrv,PaintSym);
    }
  
/* Circle ==============================================================*/
void IMCirc() {  CircTypeOps(cir,eraseFB,cir); }

/* Filled Circle ========================================================*/
void IMFCirc() { FCircTypeOps(fcir,eraseFB,fcir); }


/* Rectangle ============================================================*/

Box *ConstBoxFromSM(b) Box *b; {
    SHORT s;
    BoxFromSM(b);
    if (wasShift) { 
	s = MAX(VMapX(b->w), VMapY(b->h));
	b->w = PMapX(s);
	b->h = PMapY(s);
	} 
    return(b);
    }
    
local void frbox() {
    Box bx; ConstBoxFromSM(&bx); mOBFrame(&bx); 
    }
void IMRect() { CircTypeOps(frbox,eraseFB,frbox); }


/* Filled Rectangle ======================================================*/

local void filbox() { Box bx; ConstBoxFromSM(&bx); mRect(&bx); }
void IMFRect() { FCircTypeOps(filbox,eraseFB,filbox); }

void IMOval() { CircTypeOps(doval,eraseFB,doval); }

/* Filled Ellipse ==========================================================*/
void IMFOval() { FCircTypeOps(foval,eraseFB,foval); }


/* Flood Fill =============================================================*/
void afillproc() { mFillArea(NO,mx,my);}
IMFill() { doit = &afillproc; WentUp = &SymPts; }

/* ----- null mode (used to abort operations) ------*/    

IMNull(){ SimpleOps(nop); }

/* ReadPixel =============================================================*/
doRead() {
    SHORT px = mReadPix(mx,my);
    if (butwas==2) CPChgBg(px); else CPChgCol(px);
    }

IMReadPix(){ SimpleOps(&doRead);     JamCursor(PICKCURSOR); }

    
/* Select Brush ==========================================================*/

SHORT brx,bry;
BOOL exclBox = NO;

BrBox(bx) Box *bx; {
    BoxFromSM(bx);
    if (exclBox) { bx->w = MAX(1,bx->w-1); bx->h = MAX(1,bx->h-1); }
    }

BOOL midHandle = YES;

    
void DoSelBr() {
    SHORT midx,midy;
    SHORT xoff,yoff;
    Box bx;
    BrBox(&bx);
    if  (midHandle) {
	midx = bx.x + bx.w/2; 
	midy = bx.y + bx.h/2;
	Gridify(&midx,&midy);
	xoff = midx-bx.x;       yoff = midy-bx.y;
	}
    else { xoff = MAX(0,mx - sx);  yoff = MAX(0,my - sy); }
    bx.x += xorg;    
    bx.y += yorg;
    if (ExtractBMOB(&curbr,&bx,0,0)) {    /* --- failed --- */
	SelPen(0); 
	SetNextIMode(IM_draw); 
	return;
	} 
    CPChgPen(USERBRUSH);
    UpdtON();
    if (butwas == 2) { 
	mx = bx.x - xorg; my = bx.y - yorg;
	obsplat(); 
	mx = ex; my = ey;
	}
    curbr.xoffs = xoff;
    curbr.yoffs = yoff;
    ClearBrXform();
    GetColors(LoadBrColors);
    UserBr();
    }

local void sbfrbox()  { Box bx; BrBox(&bx); mFrame(&bx);  }

void IMSelBrush(){ XHTypeOps(&sbfrbox,&DoSelBr); }


/* == Specify Grid Location and spacing ======================================*/

extern SHORT gridx,gridy;
extern BOOL dispCoords;

local SHORT tgridx,tgridy;
local SHORT grXorg,grYorg;
local BOOL svDispCoords;

/* show a 4x4 piece of grid */
local drawGrid() {
    int i,x,y,dx,dy;
    SHORT svUsePen = usePen;
    usePen = 0;	/* make it draw thin lines */
    TempXOR();
    dx = 4*tgridx;
    dy = 4*tgridy;
    x = grXorg; y = grYorg;
    for (i=0; i<5; i++) {
	mLine(grXorg, y, grXorg + dx, y ); 
	mLine(x, grYorg, x, grYorg + dy ); 
	x += tgridx;
	y += tgridy;
	}
    RestoreMode(); 
    usePen = svUsePen;
    }

setGrOrg() { 
    grXorg = mx - 4*tgridx;
    grYorg = my - 4*tgridy;
    } 

showgrid() { setGrOrg(); drawGrid(); DispXY(grXorg, grYorg); 
  /*  minvXHair(grXorg, grYorg); */ } 
    
strgrid() { 
    tgridx = (mx - grXorg)/4;
    tgridy = (my - grYorg)/4;
    drawGrid();
    DispXY(tgridx,tgridy);
    } 

dogrid() {
    SetGridXY(tgridx,tgridy); 
    dispCoords = svDispCoords;
    SetGridOrg(grXorg,grYorg);  
    ClearXY();
    }
    
void IMGridSpec(){
    tgridx = gridx;
    tgridy = gridy;
    svDispCoords = dispCoords;
    dispCoords = NO;
    killMode = &ClearXY;
    IModeProcs(showgrid,nop,showgrid,setGrOrg,strgrid,nop,strgrid,dogrid);
    }


/* == Specify Symmetry Center ======================================*/

void doSymCenter() { SymCenter(mx,my); }
void IMSymCent(){IModeProcs(invXH,nop,invXH,doSymCenter,nop,nop,nop,nop);  }

/* == Size Pen ======================================*/
/* should only be called with pen as current object */
local SHORT siz, pnType = 0;
local BOOL  sizingAirB = NO;
local SHORT svPen, svABMode;
    
szSet() {
    if (sizingAirB) { 
	SetAirBRad(VMapY(siz)); 
	CPChgPen(svPen); 
	SetModeMenu(svABMode);
	}
    else  CPChgPen( (pnType == DOT_B)? DotBFromRad(siz) : (pnType<<12)|siz); 
    dispCoords = svDispCoords;
    ClearXY();
    }

pnDraw() { 
    SHORT svmd = paintMode;
    SHORT pen, x,y, svUsePen;
    SetPaintMode(Color);
    DispXY(siz,siz);
    x = bp.x; y = bp.y;
    pen = (pnType<<12)|siz;
    if (pnType == SQUARE_B) {x = bp.x+siz/2; y =  bp.y+siz/2;  }
    else if (pnType == DOT_B)  pen = DotBFromRad(siz); 
    SelPen(pen); 
    svUsePen = usePen;
    usePen = curpen;
    mDispOB(x,y);
    usePen = svUsePen;
    SetPaintMode(svmd);
    }

void pnUpMv() {bp.x = mx - siz;    bp.y = my - siz;  pnDraw();  }
    
void pnDnMv() { siz = MAX(ABS(mx-bp.x),ABS(my-bp.y)); pnDraw(); }

void IMSizePen(){ 
    IModeProcs(pnUpMv,nop,eraseFB, nop, pnDnMv, nop,eraseFB, szSet); 
    }

SizeAirOrPen() {
    NewIMode(IM_sizePen);
    svDispCoords = dispCoords;
    dispCoords = NO;
    JamCursor(SIZECURSOR);
    killMode = &ClearXY;
    UndoSave();
    }
    
SizePen(pn) SHORT pn; {
    CPChgPen(pn);
    sizingAirB = NO;
    if ((pn==0)||(pn==USERBRUSH)) pnType = ROUND_B;
    else pnType = (pn>>12)&15;
    siz = MAX(pn&0xfff,1);  
    SizeAirOrPen();
    }

SHORT abRadius = INITABRADIUS;

SizeAirBrush() {
    svABMode = paintMode;
    svPen = curpen;
    siz = PMapY(abRadius);
    pnType = ROUND_B;
    sizingAirB = YES;
    SizeAirOrPen();
    }

/* == Specify Fill Pattern ======================================*/
#ifdef dothis
void fpSet()  {
    }
    
void showFPBox() {
    Box b;
    mFrame(MakeBox(&b, mx - PATWIDE + 1, my - PATHIGH + 1,  PATWIDE, PATHIGH));
    }
    
void IMGetFPat(){ 
    xorit = showFPBox;
    IModeProcs(FlipIt,nop,FlipIt, nop, FlipIt, nop,FlipIt, fpSet); 
    }
#endif

void IMGetFPat(){}

/*  =============================================================*/

extern BOOL modeHelp;
	
UserBr() {CPChgPen(USERBRUSH); 
    RestoreBrush(); 
    SetModeMenu(Mask);  
    NewIMode(IM_shade); 
    }






