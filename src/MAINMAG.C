/*----------------------------------------------------------------------*/
/*   Main magnify window						*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>
#include <pnts.h>

extern PointRec pnts; /* contains mx,my,sx,sy, etc */

extern void (*thisMode)(), (*nop)(), (*killMode)();
extern struct Window *mainW;
extern Box bigBox,screenBox;
extern struct RastPort tempRP, *mainRP, tempRP, *curRP,screenRP;
extern struct BitMap hidbm;
extern Pane mainP;
extern SHORT xorg,yorg,curMBarH,curxpc;
extern void mainCproc(), mainMproc();
extern Box mainBox;
extern SHORT xShft;
extern SHORT checkpat[];
extern struct Screen *screen;
extern BOOL wasCtrl;
extern BOOL Painting;    
extern PaintMode paintMode;
    
BOOL isUndoOn = NO;
UndoOn() { isUndoOn = YES; }
UndoOff() { isUndoOn = NO; }

MagContext magCntxt = {0};
Pane magP = {0};
BOOL magOn = NO;
BOOL updatePending; /* have changed the screen but not copied to hidbm yet */
Box chgB = {0};
Box tmpChgBox = {0};

SHORT didClearCol;

/* clips a change box against the non-magnified window */
/* this is done in backing bitmap coords */
ClipChgBox(clip,chgbx) {
    Box b; 
    b.x = magCntxt.srcPos.x; b.y = magCntxt.srcPos.y;
    b.w = magCntxt.srcBox->w; b.h = magCntxt.srcBox->h;
    return(BoxAnd(clip,chgbx,&b));
    }

/* records changes so UndoSave knows how big a rectangle to save */

void RecChange(b) Box *b; {
    EncloseBox(Painting ? & chgB: &tmpChgBox,b);
    updatePending = YES;
    }

void RecTempChg(b) Box *b; { EncloseBox(&tmpChgBox,b);  }

CopyBack(b,op) Box *b; SHORT op; {
    tempRP.BitMap = &hidbm;
    ClipBlit(mainRP, b->x +xorg, b->y + yorg,
	&tempRP, b->x, b->y, b->w, b->h, op);
    } 

CopyFwd(b,op) Box *b; SHORT op; {
    tempRP.BitMap = &hidbm;
    ClipBlit( &tempRP, b->x, b->y,
		mainRP,b->x +xorg, b->y + yorg, b->w, b->h, op); 
    } 

/* make previous change permanent */
SHORT didType = DIDNothing;
    
#ifdef hobaaaho
UndoSave() { 
    Box cb;
    if (didClear) {
	tempRP.BitMap = &hidbm; SetRast(&tempRP,didClearCol);
	didClear = NO;
	}
    else if (didMerge)  MrgSpareUpdt();
    if (ClipChgBox(&cb,&chgB)) CopyBack(&cb,REPOP); chgB.w = 0; 
    }
#endif

/* this should be called "MakeChangePermanent" */
/* it copies changed area from the screen to the hidbm */
UndoSave() { 
    Box cb;
    ClearFB();
    switch(didType) {
	case DIDClear:
	    tempRP.BitMap = &hidbm; SetRast(&tempRP,didClearCol);
	    break;
	case DIDMerge:   MrgSpareUpdt(); break;
	}
    if (ClipChgBox(&cb,&chgB)) CopyBack(&cb,REPOP); chgB.w = 0; 
    didType = DIDNothing;
    updatePending = NO;
    }

/* swap hidden bitmap and screen in chgB */
SwapHidScr() {
    Box cb;
    if (ClipChgBox(&cb,&chgB)) {
	CopyFwd(&cb,XOROP); 
	CopyBack(&cb,XOROP); 
	CopyFwd(&cb,XOROP);
	MagUpdate(&chgB);
	}
    }

Undo() { 
    switch(didType) {
	case DIDClear: case DIDMerge:  UpdtDisplay(); didType = DIDNothing; break;
	case DIDHPoly: SwapHidScr();  UndoHPoly(); didType = DIDNothing; break;
	case DIDNothing: SwapHidScr();   break;
	}
    }

/* this is called when operations are aborted in midstream */
UnDisplay() { UpdtDisplay(); UndoSave(); }

EraseBox(b) Box *b; {
    Box clb;
    if (ClipChgBox(&clb,b)) { CopyFwd(&clb,REPOP); MagUpdate(&clb);}
    }
    
/* this erases feedback from the screen */
EraseTempChg() {
    if (wasCtrl) { EncloseBox(&chgB, &tmpChgBox);
	CycPaint();	/* A cheap effect... */    
	}
    else EraseBox(&tmpChgBox); 
    tmpChgBox.w = 0;
    }

/**-------------------------- **/

#define BIG 1000;


void magPaint() {
    Box sbx; 
    if (magOn) {
	sbx.x = magCntxt.magPos.x;
	sbx.y = magCntxt.magPos.y;
	sbx.w = sbx.h = BIG;
	MagBits(&hidbm, &sbx, mainRP->BitMap,magP.box.x, magP.box.y,
	    magCntxt.magN, magCntxt.magN, &magP.box);
	}
    }

mClearToBg()  {
    UndoSave();
    TempErase();
    PFillBox(&screenBox);
    ClearErase();
    didType = DIDClear;
    didClearCol = curxpc;
    MagUpdate(&screenBox);
    }

#define DivWidth 6
PaintDivider() {
    Box dbox;
    if (magOn) {
	dbox.x = mainBox.w/3;
	dbox.y = mainBox.y;
	dbox.w = DivWidth;
	dbox.h = mainBox.h;
	
	PushGrDest(&screenRP,&screenBox);
	PPatternBox(&dbox, checkpat, 1, 0, 0);
	PopGrDest();
	}
    }


void magRefresh() {  magPaint();  PaintDivider();  }

void mainRefresh() {
    tempRP.BitMap = &hidbm;
    ClipBlit(&tempRP,magCntxt.srcPos.x,magCntxt.srcPos.y,
	mainRP, mainP.box.x, mainP.box.y, mainP.box.w, mainP.box.h, REPOP);
    }

UpdtDisplay() {ClearFB(); mainRefresh(); magPaint(); }


#ifdef THEIRWAY /*---------------------------------*/
MoveWinTo(win,x,y) struct Window *win; int x,y; {
    MoveWindow(win, x - win->LeftEdge, y - win->TopEdge);
    }

SizeWinTo(win,w,h) struct Window *win; int w,h; {
    SizeWindow(win, w - win->Width, h - win->Height);
    }
    
SizeAndMove(win,b) struct Window *win; Box *b; {
    if (win->Height != b->h) {
	if (b->h > win->Height) { /* getting higher: move first */
	    MoveWinTo(win, win->LeftEdge, b->y);
	    SizeWinTo(win, win->Height, b->h);
	    }
	else {
	    SizeWinTo(win, win->Height, b->h);
	    MoveWinTo(win, win->LeftEdge, b->y);
	    }
	}
    if (win->Width != b->w) 
	{
	if (b->w > win->Width) { /* getting wider:move first */
	    MoveWinTo(win, b->x, win->TopEdge);
	    SizeWinTo(win, b->w, win->Height);
	    }
	else {
	    SizeWinTo(win, b->w, win->Height);
	    MoveWinTo(win, b->x, win->TopEdge);
	    }
	}
    }

#endif /* ------------------------------------------ */

ResizeWins() {
    SHORT magx;
    Box mbox;
    mbox = mainBox;
    mbox.w = (magOn)? mainBox.w/3: mainBox.w; 

#ifdef THEIRWAY
    SizeAndMove(mainW,&mbox);
    KPrintF(" After SizeAndMOve mainW = %ld, %ld, %ld, %ld\n",
	    mainW->LeftEdge,mainW->TopEdge,mainW->Width, mainW->Height);
#endif

    PaneSize(&mainP, mainBox.x, mainBox.y, mbox.w, mainBox.h);
    magx = mbox.w + DivWidth;
    PaneSize(&magP, magx, mainBox.y, mainBox.w - magx, mainBox.h);
    }
	
MagCnst(b,p) Box *b; Point *p; {
    Box c;
    c.x = p->x;    c.y = p->y;
    c.w = b->w;    c.h = b->h;
    BoxBeInside(&c,&screenBox);
    p->x = c.x;    p->y = c.y;
    b->w = c.w;    b->h = c.h;
    }
    
/* make sure that the magnified portion of the bitmap is visible in
   the main window */

BOOL FixMag() {
    SHORT nout;
    Box mf;
    BOOL srcChg = NO;
    /* first make sure mag and src  views are inside the hidbm*/
    MagCnst(magCntxt.srcBox,&magCntxt.srcPos);
    if (magOn) {
    	mf.x = magCntxt.magPos.x;
	mf.y = magCntxt.magPos.y;
	mf.w = UnMag(magCntxt.magBox->w);
	mf.h = UnMag(magCntxt.magBox->h);

	BoxBeInside(&mf,&screenBox);
	magCntxt.magPos.x = mf.x;
	magCntxt.magPos.y = mf.y;

	if (magCntxt.magPos.x < magCntxt.srcPos.x)
	    { srcChg = YES; magCntxt.srcPos.x = magCntxt.magPos.x; }
	
	if (magCntxt.magPos.y < magCntxt.srcPos.y)
	    { srcChg = YES; magCntxt.srcPos.y = magCntxt.magPos.y; }
    
	nout = (mf.x + mf.w) - (magCntxt.srcPos.x + magCntxt.srcBox->w); 
	if (nout>0) { srcChg = YES; magCntxt.srcPos.x += nout; }
    
	nout = (mf.y + mf.h) - (magCntxt.srcPos.y + magCntxt.srcBox->h); 
	if (nout>0) { srcChg = YES; magCntxt.srcPos.y += nout; }

	}
    UpdtCach();
    return(srcChg);
    }

FixMagAndPaint(p) Pane *p; {
    BOOL srcChg = FixMag();
    if (p == &magP) { magPaint(); if (srcChg) mainRefresh(); }
    else mainRefresh();
    }

Point svSrcPos;
    
CenterPoint(x,y) SHORT x,y; {
    UndoSave();
    /*------- center mag box at x,y----*/
    magCntxt.magPos.x = x - ( UnMag(magP.box.w) >> 1);
    magCntxt.magPos.y = y - ( UnMag(magP.box.h) >> 1);

    /* -- and try to center main box at x,y--- */
    magCntxt.srcPos.x = x - magCntxt.srcBox->w/2;
    magCntxt.srcPos.y = y - magCntxt.srcBox->h/2;
    
    MagState(YES);
    FixMag();
    
    UpdtDisplay();
    }

ShowMag(x,y) SHORT x,y; {
    if (!magOn) {
	UndoSave();
	svSrcPos = magCntxt.srcPos;
	magOn = YES;
	ResizeWins();
	PaneInstall(&magP);
	PaintDivider();
	}
    CenterPoint(x,y);
    }
    
void RemoveMag() {
    if (!magOn) return;
    UndoSave();
    magOn = NO;
    PaneRemove(&magP);
    ResizeWins();
    MagState(NO);
    magCntxt.srcPos = svSrcPos;
    FixMag();
    PaneRefresh(&bigBox);
    UpdtCach();
    }

ToggleMag() {if (magOn) RemoveMag(); else ShowMag(mx,my); } 

BOOL titleShowing = YES;
static BOOL titleWasUp = YES;
BOOL skipRefresh = NO;
RepaintEverything() {
    ResizeWins();
    FixMag();
    PaneRefresh(&bigBox);
    skipRefresh = YES;
    UpdtCach();
    DispPntMode();
/*    DispDPaint(); */
    }

FixTitle() {
    ShowTitle(screen,titleShowing);
    if (titleShowing) {
	mainBox.y = curMBarH; 
	mainBox.h = screenBox.h - curMBarH; 
	magCntxt.srcPos.y += curMBarH;
	}
    else { 
	mainBox.y = 0; 
	mainBox.h = screenBox.h; 
	magCntxt.srcPos.y -= curMBarH;
	}
    }
    
TogTitle() {
    titleWasUp = titleShowing = !titleShowing;
    UndoSave();
    FixTitle();
    RepaintEverything();
    }

extern SHORT cpWidth;
extern BOOL cpShowing;
TogBoth() {
    cpShowing = !cpShowing;
    UndoSave();
    if (cpShowing) { 
	titleShowing = titleWasUp; 
	mainBox.w -= cpWidth; 
	InstCP();
	}
    else { 
	titleWasUp = titleShowing;
	titleShowing = NO;
	RmvCP(); 
	mainBox.w += cpWidth; 
	}
    FixTitle();
    RepaintEverything();
    }
    
/* scroll window: dx and dy are the amount the contents of the window
will slide to the right and down respectively */
MWScroll(p,dx,dy) Pane *p; SHORT dx,dy; {
    UndoSave();
    if (p == &mainP ) {
	magCntxt.srcPos.x -= dx;
	magCntxt.srcPos.y -= dy;
	}
    else if ( p == &magP) {
	magCntxt.magPos.x -=dx;
	magCntxt.magPos.y -=dy;
	}
    FixMagAndPaint(p);
    }

/* Zoom window:  */
void MWZoom(dmag)   SHORT dmag; 
    {
    SHORT x,y;
    SHORT newmag = magCntxt.magN + dmag;
    newmag = MAX(2,newmag);
    if (newmag>24) return;
    UndoSave();
    x = magCntxt.magPos.x + UnMag(magCntxt.magBox->w)/2;
    y = magCntxt.magPos.y + UnMag(magCntxt.magBox->h)/2;
    SetMag(newmag);
    magCntxt.magPos.x = x - UnMag(magCntxt.magBox->w)/2;
    magCntxt.magPos.y = y - UnMag(magCntxt.magBox->h)/2;
    FixMag();
    UpdtDisplay();
    }


/**-----(From Menu) Specify Magnify Window Position */

static SHORT mgw,mgh;

domag() { UpdtOFF();  ShowMag(mx,my); }

void killMag() { CPInvAct(magAct); }
xmagbox() { 
    Box bx;
    TempXOR();	
    mFrame(MakeBox(&bx,mx-mgw/2,my-mgh/2,mgw,mgh));
    RestoreMode(); 
    }

void IMMagSpec() {
    mgw = UnMag(2*mainBox.w/3);
    mgh = UnMag(mainBox.h);
    killMode = killMag;
    IModeProcs(&xmagbox,nop,&xmagbox,nop,&xmagbox,nop,&xmagbox,&domag);
    }
    
ToglMagw(){ 
    CPInvAct(magAct);
    if (magOn) RemoveMag(); else NewIMode(IM_magSpec);
    }

