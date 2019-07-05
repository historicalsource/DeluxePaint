/*-------------------------------------------------------------------*/
/*        paintw -- Painting Window	 			     */
/*-------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>
#include <pnts.h>

#define local 
#define UP	0
#define DN	1

extern Pane mainP,magP;
extern BOOL ShiftKey, Ctrl;
extern SHORT dbgpane;
extern SHORT curxpc;
extern BOOL titleShowing, abortFlag;
extern SHORT npts;

BOOL firstDown = YES;  /* for modes that need to know its the
		first call on a button down drag (like blend & smear)
		or in a LineWith etc */
BOOL dispCoords= NO;
BOOL goToPerm = NO;/* for falling out of extended modes like poly*/    
void nada() {}
void (*nop)() = &nada;

/* the following 8 proc vars are plugged to install a mode */
local void (*upShow)(),(*upMove)(),(*upClear)(),(*WentDn)();
local void (*dnShow)(),(*dnMove)(),(*dnClear)(),(*WentUp)();
        
/* state of program */
PointRec pnts; /* contains mx,my,sx,sy,etc */
SHORT nx,ny;
SHORT butwas=0, button=0;
void (*killMode)() = &nada;
void (*completeMode)() = &nada;
local BOOL gridded=NO;
local BOOL fbOn = NO;
local BOOL updn = UP;
local BOOL inside = NO;
SHORT gridOrgX = 0;
SHORT gridOrgY = 0;
SHORT gridx=8;
SHORT gridy=8;
SHORT gridx2=4;
SHORT gridy2=4;
BOOL gridON = NO;
BOOL wasShift = NO;
BOOL wasCtrl = NO;

BOOL everyTime = NO;
SHORT curbgcol=0;

/* IMode state */
SHORT lastPermMode= IM_none, curIMode=IM_none;
UWORD curIMFlags= 0;
IModeDesc *curIMDesc = NULL;

SetGridXY(gx,gy) SHORT gx,gy; {
    gx = MAX(1,gx); gy= MAX(1,gy);
    gridx = gx; gridy = gy;
    gridx2 = gx/2;  gridy2=gy/2;
    }

SetGridOrg(ox,oy) SHORT ox,oy; {
    gridOrgX = ox;
    gridOrgY = oy;
    }

SetGridSt(g) BOOL g; { if (gridON!=g) TogGridSt(); }
TogGridSt() { gridON = !gridON; CPInvAct(gridAct); }	    

Gridify(x,y) SHORT *x,*y;{
    SHORT rx,ry;
    if (gridded&gridON) { 
	rx =  *x - gridOrgX + gridx2;
	if (rx<0) rx -= gridx;
	*x = gridOrgX + (rx/gridx)*gridx;
	ry =  *y - gridOrgY + gridy2;
	if (ry<0) ry -= gridy;
	*y = gridOrgY + (ry/gridy)*gridy;
	}
/*    MWClipPt(x,y,gridx,gridy); */
    }
	
nxtpoint() {lx = mx; ly = my; mx = nx; my = ny;	}

/*--- XOR Feedback to tell the user what's happening under symmetry */
extern BOOL symON;
extern SHORT usePen;
short symcount = 0;

void symsplat() { if (++symcount>1) mDispOB(mx,my); }

SymShowOb(upOrDn) WORD upOrDn; { 
    SHORT saveUsePen;
    if ((curIMFlags&upOrDn)&&(symON)) {
	TempXOR(); 
	saveUsePen = usePen;
	if (curIMFlags&NOBR)  usePen = 0; 
	symcount = 0;
	DisableAbort();
	SymDo(1,&pnts,symsplat);
	EnableAbort();
	usePen = saveUsePen;
	RestoreMode();
	}
    }
    
void ShowFB(){ if (fbOn||(!inside)) return;
    if (updn==UP) { (*upShow)(); SymShowOb(SYMUP);}
    else { (*dnShow)();  SymShowOb(SYMDN);}
    fbOn = YES;
    }
    
void ClearFB(){ if (!fbOn) return;
    if (updn==UP) { SymShowOb(SYMUP); (*upClear)(); }
    else { SymShowOb(SYMDN); (*dnClear)(); }
    fbOn = NO;
    }

local EnterW(pn,x,y) Pane *pn; SHORT x,y; { 
    inside = YES;
    LoadIMode();
    ResetCursor();
    SetCurWin(pn);
    nx = x; ny = y;	/* have to recompute coords for new window */
    MagCoords(&nx,&ny); 
    Gridify(&nx,&ny);
    updn = button;
    nxtpoint();
    nxtpoint();
    sx = mx;  sy = my;
    ShowFB(); 
    }

local LeaveW(pn,x,y) Pane *pn; {  
    ClearFB();
    SetCursor(DEFCURSOR);
    inside = FALSE;  
    }
    
local TrackUp()  {  
    if (!fbOn) { nxtpoint(); ShowFB(); }
    else if ((nx!=mx)||(ny!=my)||everyTime) {
	SymShowOb(SYMUP);
	if (upMove!=nop) { nxtpoint(); (*upMove)(); }
	else {  (*upClear)(); nxtpoint(); (*upShow)(); }
	SymShowOb(SYMUP);
	}
    }

SetEveryTime() {
    everyTime = YES;
    mainP.flags |= ALWAYS;
    magP.flags |= ALWAYS;
    }
    
ClrEveryTime() {
    everyTime = NO;
    mainP.flags &= ~ALWAYS;
    magP.flags &= ~ALWAYS;
    }
    
local GoDown() {
    ClearFB(); 
    updn = DN;
    FixUsePen();
    butwas = button;

    wasShift = ShiftKey;
    wasCtrl = Ctrl;
    nxtpoint();
    sx = mx;  sy = my;
    if ((button==2)&&((NOERASE&curIMFlags) == 0)) TempErase();
    UndoSave();
    firstDown = YES;
    if (curIMFlags&EVTIM) SetEveryTime();
    if (curIMFlags&PNTWDN) UpdtON();
    (*WentDn)();
    ShowFB();
    }
	
local TrackDown() {
    if (!fbOn) { nxtpoint(); ShowFB(); }
    else if (((nx!=mx)||(ny!=my))||everyTime) {
	SymShowOb(SYMDN);
	if (dnMove!=nop) { nxtpoint(); (*dnMove)(); }
	else {  (*dnClear)(); nxtpoint(); (*dnShow)(); }
	SymShowOb(SYMDN);
	}
    }
	
void GoUp() {
    ClrEveryTime();
    ClearFB();
    updn = UP;
    ex = mx; ey = my;
    (*WentUp)();
    UpdtOFF();
    ClearErase();
    EndCycPaint(); 	/* this updates ctr panel if color changed*/
    BMOBFreeTmp(); 	/* in case smear was going */
    psx = sx;   psy = sy;
    pex = ex;   pey = ey;
    NextIMode();
    ShowFB();
    }

/*----------------------------------------------------------------------*/
/* Interaction Modes							*/
/*----------------------------------------------------------------------*/

extern void IMNull(), IMShade(), IMDraw(), IMVect(), IMCurve1(), IMCurve2();
extern void IMRect(), IMFRect(), IMCirc(), IMFCirc(), IMOval(), IMFOval();
extern void IMSelBrush(), IMMagSpec(), IMFill(), IMText1(), IMText2();
extern void IMGridSpec(),  IMReadPix(), IMStrBrush(), IMSymCent();
extern void IMRotBrush(), IMShrBrush(), IMSizePen();
extern void IMPoly(), IMPoly2(),  IMFPoly(), IMFPoly2(), IMAirBrush();
extern void IMGetFPat(), IMHBendBrush(),IMVBendBrush();

#if 0
typedef UBYTE IMode;
typedef struct {
    WORD flags;		/* see below */
    UBYTE cursor;	/* what cursor to use */
    UBYTE activity;	/* ctr panel activity for this imode */
    UBYTE symNpoints;	/* number of points to be transformed*/
    IMode nextIMode;	/* for chaining */
    void (*startProc)(); /* call this to plug mode procs*/
    } IModeDesc;
#endif

/* Master IMODE table */

IModeDesc imodes[NIMODES] = {
/*-- Flags, cursor, activity, nextIMode, startProc -------*/
 { NOLOCK|NOSYM|NOBR, DEFC, nullAct,1,IM_none, &IMNull},	/* IM_null	*/
 { PERM|SYMUP|PNTWDN, DEFC, shadeAct,1, IM_none, &IMShade},	/* IM_shade	*/
 { PERM|NOGR|SYMUP|PNTWDN, DEFC, drawAct,2, IM_none, &IMDraw},	/* IM_draw	*/
 { PERM|SYMALL, DEFC, vectAct, 3, IM_none, &IMVect},	/* IM_vect	*/
 { PERM|SYMALL, DEFC, curvAct, 3, IM_curve2, &IMCurve1},	/* IM_curve1	*/
 { SLAVE, DEFC, curvAct, 6, IM_none, &IMCurve2},	/* IM_curve2	*/
 { NOCON|SYMALL, DEFC, rectAct, 3, IM_none, &IMRect},		/* IM_rect	*/
 { NOCON|NOBR|SYMALL, DEFC, frectAct, 3, IM_none, &IMFRect},	/* IM_frect	*/
 { SYMALL, DEFC, circAct, 3, IM_none, &IMCirc},		/* IM_circ	*/
 { NOBR|SYMALL, DEFC, fcircAct, 3, IM_none, &IMFCirc},	/* IM_fcirc	*/
 { SYMALL, DEFC, ovalAct, 3, IM_none, &IMOval},		/* IM_oval	*/
 { NOBR|SYMALL, DEFC, fovalAct, 3, IM_none, &IMFOval},	/* IM_foval	*/
 { NOSYM|NOLOCK, DEFC, selbAct, 1, IM_none, &IMSelBrush},	/* IM_selBrush	*/
 { NOBR|NOGR|NOSYM|NOLOCK, DEFC, nullAct, 1, IM_none, &IMMagSpec},/* IM_magSpec	*/
 { NOBR|NOGR|SYMALL, FILLCURSOR, fillAct, 1, IM_none, &IMFill},	/* IM_fill	*/
 { NOLOCK|NOBR|NOSYM, DEFC, textAct, 1, IM_text2, &IMText1},		/* IM_text1	*/
 { NOBR|NOSYM|SLAVE, DEFC, textAct, 1, IM_text2, &IMText2},	/* IM_text2	*/
 { NOBR|NOGR|NOSYM|NOLOCK, DEFC, nullAct, 1, IM_none, &IMGridSpec},	/* IM_gridSpec  */
 { NOBR|NOSYM|NOLOCK, DEFC, nullAct, 1, IM_none, &IMSymCent},	/* IM_symCent	*/
 { NOBR|NOGR|NOSYM|NOLOCK, PICKCURSOR, nullAct,1,IM_none, &IMReadPix},/* IM_readPix	*/
 { NOCON|NOSYM|NOLOCK, SIZECURSOR, nullAct, 1, IM_none, &IMStrBrush},	/* IM_strBrush	*/
 { NOGR|NOSYM|NOLOCK, DEFC, nullAct, 1, IM_none, &IMRotBrush},	/* IM_rotBrush	*/
 { NOSYM|NOLOCK, DEFC, nullAct, 1, IM_none, &IMShrBrush},	/* IM_shrBrush	*/
 { NOERASE|NOSYM|NOGR|NOLOCK,SIZECURSOR, nullAct, 1, IM_none, &IMSizePen},	/* IM_sizePen	*/
 { SYMALL,DEFC, polyAct, 6, IM_poly2, &IMPoly},	/* IM_poly	*/
 { SYMALL|SLAVE,DEFC, polyAct, 6, IM_poly2, &IMPoly2},	/* IM_poly2	*/
 { NOBR|SYMALL,DEFC, fpolyAct, 6, IM_fpoly2, &IMFPoly},	/* IM_fpoly	*/
 { COMPLETE|NOBR|SYMALL|SLAVE,DEFC, fpolyAct, 6, IM_fpoly2, &IMFPoly2},	/* IM_fpoly2	*/
 { PERM|SYMUP|NOGR|PNTWDN|EVTIM, DEFC, airbAct, 1, IM_none, &IMAirBrush},	/* IM_airBrush	*/
 { NOSYM|NOBR, DEFC, nullAct, 1, IM_none, &IMGetFPat},		/* IM_getFPat	*/
 { NOGR|NOSYM|NOLOCK, DEFC, nullAct, 1, IM_none, &IMHBendBrush},/* IM_hbendBrush	*/
 { NOGR|NOSYM|NOLOCK, DEFC, nullAct, 1, IM_none, &IMVBendBrush}	/* IM_vbendBrush	*/
 };

BOOL allPerm = YES;

BOOL fastFB = NO;
SHORT usePen = 0;
extern SHORT curpen;
extern BOOL Painting;

SetUsePen(p) SHORT p; { usePen = p; }

/* if we arent updating the drawing, in fast feedback mode, if the 
button is down or this is a no brush  or slave mode use the 1 dot pen*/
FixUsePen() {
    usePen = curpen;
    if (!Painting && fastFB) 
	if ((updn==DN) || (curIMFlags&(NOBR|SLAVE))) usePen = 0;
    }

/* Call this to change mode */

void NewIMode(im) IMode im; {    
    if (im==IM_none) return;
    abortFlag = NO;
    everyTime = NO;
    (*killMode)();
    curIMDesc = &imodes[im];
    curIMFlags = curIMDesc->flags;
    if (!(curIMFlags&SLAVE)) { /* a "master" mode */
	if ( (curIMFlags&PERM)||
	     ( ((im==curIMode)||allPerm) && (!(curIMFlags&NOLOCK)) ) ) 
		lastPermMode = im; /* lock this mode in */
	}
    curIMFlags &= (~RIGHTBUT);
    gridded = ((curIMFlags&NOGR) == 0);
    npts = curIMDesc->symNpoints;
    completeMode = killMode =  upShow = upMove =  upClear = WentDn= nop;
    dnShow = dnMove = dnClear =  WentUp = nop;
    curIMode = im;
    if (curIMDesc->cursor==DEFC) DefaultPaintCursor();
	else SetPaintCursor(curIMDesc->cursor);
    CPChgAct(curIMDesc->activity);
    FixUsePen();
    (*curIMDesc->startProc)(NO);
    }
/* start proc called only these 2 places,above and --*/
LoadIMode() { (*curIMDesc->startProc)(YES); }

RevertIMode() { NewIMode(lastPermMode); }


/* this is called after button up, to go to next mode.  */
NextIMode() {
    IMode next = curIMDesc->nextIMode;
    killMode = nop;
    if ((!goToPerm)&&(next!=IM_none)) 	NewIMode(next);
    else { goToPerm = NO; RevertIMode();}
    }

void NewButIMode(im,but) IMode im; short but; {
    NewIMode(im);
    if (but==2) curIMFlags |= RIGHTBUT; else curIMFlags &= (~RIGHTBUT);
    }

AbortIMode() {
    abortFlag = NO;
    if (MouseBut()) NewIMode(IM_null); 
    else {
	if (curIMFlags&COMPLETE) { 
	    (*completeMode)(); /* hack for fill poly */
	    completeMode = &nada;
	    }
	else  { (*killMode)(); goToPerm = YES;}
	NextIMode(); 
	}
    }

/* This is typically to set up a mode from within the button down or up
	proc of the current mode */
    
SetNextIMode(m) IMode m; { lastPermMode = m; }   

/* This is typically called by the start proc to stuff procs */
/* They all default to "nop" so only the necessary ones need
   be stuffed */

IModeProcs(upSh,upMv,upCl,wdn,dnSh,dnMv,dnCl,wup)
    void (*upSh)(),(*upMv)(),(*upCl)(),(*wdn)(),
	(*dnSh)(),(*dnMv)(),(*dnCl)(),(*wup)();    {   
    upShow = upSh; upMove = upMv; upClear = upCl; WentDn= wdn;
    dnShow = dnSh; dnMove = dnMv; dnClear = dnCl; WentUp= wup;
    }

local void (*periodicCall)() = NULL;
SetPeriodicCall(proc) void (*proc)(); { periodicCall = proc; }

#define CONS_NOT 0
#define CONS_INIT 1
#define CONS_HORIZ 2
#define CONS_VERT 3

SHORT conStartX, conStartY;
SHORT consState = CONS_NOT;

void Constrain(px,py) SHORT *px,*py; {
    SHORT dx,dy;
    if (curIMFlags&NOCON) { consState= CONS_NOT; return; }
    switch(consState) {
	case CONS_NOT: if (ShiftKey) { 
	    conStartX = *px;
	    conStartY = *py;
	    consState = CONS_INIT;
	    }
	    break;
	case CONS_INIT:
	    dx = *px - conStartX;
	    dy = *py - conStartY;
	    if ((dx!=0)||(dy!=0)){
		if (dx==0) consState = CONS_VERT;
		else if (dy==0) consState = CONS_HORIZ;
		else consState = CONS_NOT;
		}
	    break;
	default: 
	    if (!ShiftKey) consState = CONS_NOT;
	    else switch(consState) {
		case CONS_HORIZ: *py = conStartY; break;
		case CONS_VERT: *px = conStartX; break;
		}
	    break;
	}
    }

Point nogrid;

/* ------------------------------------------*/
mainMproc(pn,why,but,x,y) Pane *pn; MouseEvent why; SHORT but,x,y; {
    nx = x;
    ny = y;
    button = but;
    MagCoords(&nx,&ny);  /* convert to backing bitmap coords */ 
    nogrid.x = nx;
    nogrid.y = ny;
    Gridify(&nx,&ny);	 /* constrain to grid */
    Constrain(&nx,&ny);
    if (dispCoords&&titleShowing) if (updn==DN) DispXY(nx-sx,ny-sy);
    	else DispXY(nx,ny);
    switch(why) {
	case ENTER:  EnterW(pn,x,y); break;
	case LOOPED: 
	case MOVED:  if (but) TrackDown(); else TrackUp();break;
	case BDOWN:  GoDown(); break;
	case BUP:    GoUp(); break; 
	case LEAVE:  LeaveW(pn,x,y); break; 
	}
    if (periodicCall) (*periodicCall)();
    }

