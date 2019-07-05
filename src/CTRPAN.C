/*--------------------------------------------------------------*/
/*								*/
/*        ctrpan.c  -- graphics control panel	  		*/
/*								*/
/*--------------------------------------------------------------*/

#include <system.h>
#include <prism.h>

extern void ToglMagw();
extern void (*nop)();

extern struct Window *mainW;
extern Box bigBox, mainBox, screenBox;
extern struct RastPort screenRP,tempRP;
extern BOOL symON,gridON,magOn;
extern SHORT nColors, solidpat[];
extern SHORT xShft, yShft, curDepth,curFormat;
extern struct TmpRas tmpRas;
extern SHORT curIMode, lastPermMode;
extern IModeDesc imodes[];
	
extern SHORT act1[],act2[],act3[],act4[],act5[],act6[],act7[];
extern SHORT act8[],act9[],act10[],act11[],act12[],act13[],act14[];
extern SHORT act15[],act16[],act17[],act18[];
extern SHORT act5H[],act5F[];
extern SHORT act6H[],act6F[];
extern SHORT act7H[],act7F[];
extern SHORT act8H[],act8F[];
extern SHORT penmenu[];
    
SHORT *actbm[18] = {act1,act2,act3,act4,act9,act10,act5,act6,act7,act8,
    act11,act12,act13,act14,act15,act16,act17,act18};

SHORT *acthbm[4] = {act5H,act6H,act7H,act8H};
SHORT *actfbm[4] = {act5F,act6F,act7F,act8F};

#define local

#define BLACK 0

/* Dimensions of activity menu */
#define actNPL	2

/* Dimensions of Pen Menu*/
#define penNPL	3

/* Dimensions of Color Pallet*/

SHORT actUH = 24;
SHORT colNColumns = 4;
SHORT colNRows = 8;
SHORT colUH = 6;
SHORT colDspH = 12;
SHORT cpWidth,actUW,colUW,penUW;
SHORT penH,actH;

#define noActBut  128

/*************************************************/
/* initialize Panes for debugging purposes */
Pane  penwin={0},actwin={0},cdspwin={0},colwin={0}; 
SHORT cpfgcol=-1,cpbgcol,activity,cpPen;
UBYTE actBut = noActBut;
BOOL  cpShowing;

extern void CPInvAct();

/** Dummy Calls **/
extern void mainCproc();
/*============== utilities===================== */

ScreenDest() {  PushGrDest(&screenRP,&screenBox); }

XorScreenBox(bx) Box *bx; {
    ScreenDest();
    SetAPen(&screenRP,1);
    SetXorFgPen(&screenRP);
    PFillBox(bx);
    SetDrMd(&screenRP,JAM2);
    PopGrDest();
    }

ColorBox(b,c) Box *b;SHORT c; {
    ScreenDest();
    TempMode(Replace,c,solidpat);
    PFillBox(b);
    RestoreMode();
    PopGrDest();
    }


local SHORT downIn;
local SHORT cpButton;
SHORT NXMag, NYMag;

/*--------------------------------------------------------------*/
/*        Pen Menu				  		*/
/*--------------------------------------------------------------*/
#ifdef thisIsInPrism_H
#define ROUND_B		1
#define SQUARE_B	2
#define DOT_B		3
#define RoundB(n)	((ROUND_B<<12)|n)
#define SquareB(n)	((SQUARE_B<<12)|n)
#define DotB(n)		((DOT_B<<12)|n)
#endif


typedef struct {BYTE x,y,w,h; } ByteBox;

#define NPenBoxes 10

/* these coordinates are in VDC */
ByteBox penBoxes[NPenBoxes] = {
    {0,0,8,12}, {8,0,10,12}, {18,0,12,14}, {30,0,14,14},
    {0,14,14,12}, {14,14,12,12}, {26,14,10,10}, {34,14,10,8},
    {4,26,14,12}, {23,25,22,14}
    };


SHORT penNums[NPenBoxes] = {
    0,RoundB(1),RoundB(2), RoundB(3),
    SquareB(5),SquareB(4),SquareB(3),SquareB(2),
    DotB(1),DotB(2)
    };
    
local void xorPenBox(n) int n; {
    Box bx;
    ByteBox *byb;
    if (!cpShowing) return;
    if ((n < 0)|| (n>10)) return;
    byb = &penBoxes[n];
    bx.x = PMapX(byb->x+2) + penwin.box.x;
    bx.y = PMapY(byb->y+2) + penwin.box.y;
    bx.w = PMapX(byb->w);
    bx.h = PMapY(byb->h);
    XorScreenBox(&bx);
    }

Box *BoxFrByteBox(bx,bbx) Box *bx; ByteBox *bbx; {
    return((Box *)MakeBox(bx,bbx->x,bbx->y,bbx->w,bbx->h));
    }
    
BOOL InByteBox(bbx,x,y)  ByteBox *bbx; SHORT x,y; {
    Box bx;
    return((BOOL)BoxContains(BoxFrByteBox(&bx,bbx),x,y));
    }
    
local int GetPenBox(x,y) SHORT x,y; {
    int i;
    x = VMapX(x)-2;
    y = VMapY(y)-4;
    for (i=0; i<NPenBoxes; i++)  if ( InByteBox(&penBoxes[i], x, y) )
	return(i);
    return(-1);
    }


local SHORT penForXY(x,y) SHORT x,y; { return(penNums[GetPenBox(x,y)]); }
   
local penMproc(ph,why,but,x,y) Pane *ph; MouseEvent why; SHORT but,x,y; {
    SHORT pNum,n;
    n = GetPenBox(x,y);
    switch(why) {
	case BDOWN: cpButton = but; downIn = n; xorPenBox(n); break;
	case BUP: xorPenBox(downIn); 
	    if (downIn>=0) {
		pNum = penNums[downIn];
		if (cpButton==1) CPChgPen(pNum);
		else SizePen(pNum);
		}
	    break;
	}
    }

BoxForPen(pn) SHORT pn; {
    int i;
    for (i=0; i<NPenBoxes; i++) if (pn == penNums[i]) return(i);
    return(-1);
    }

SHORT cpPenBox = -1;    
	
/* new is a pen# */
CPChgPen(new) SHORT new; {    
    xorPenBox(cpPenBox); 
    cpPen = new;
    cpPenBox = BoxForPen(new);
    xorPenBox(cpPenBox);
    if (cpPen!=USERBRUSH) if ((curIMode==IM_null)||(imodes[lastPermMode].flags&NOBR))
	NewIMode(IM_shade);
    SelPen(new);
    }

#define PENMENUW 25
#define PENMENUH 20

void PenWPaint()  {
    struct BitMap tmp;
    Box sbx;
    InitBitMap(&tmp,1,PENMENUW,PENMENUH);
    tmp.Planes[0] = (PLANEPTR)penmenu;
    if (!cpShowing) return;
    penwin.box.y = mainBox.y;
    penwin.box.x = screenBox.w - cpWidth;
    ColorBox(&penwin.box, BLACK);
    MagBits(&tmp, MakeBox(&sbx,0,0,PENMENUW,PENMENUH), screenRP.BitMap,
	penwin.box.x, penwin.box.y, NXMag,NYMag,&screenBox);
    xorPenBox(cpPenBox);
    }


/*--------------------------------------------------------------*/
/*        Activity Menu				  		*/
/*--------------------------------------------------------------*/

#define NACTBUTS 18

Delay(n) SHORT n; { SHORT i; for (i=0; i<500*n; i++) {} }

    
BOOL LRButton = NO;
local BOOL LSide;
local ActButton(but,i) SHORT but,i; {
    BOOL doLeft = (LRButton)? (but==1) : LSide;
    switch(i) {
	case 0:  NewIMode(IM_shade); break;
	case 1:  NewIMode(IM_draw); break;
	case 2:  NewIMode(IM_vect); break;
	case 3:  NewIMode(IM_curve1); break;
	case 4:  NewIMode(IM_fill); break;    
	case 5:  if (but==1) NewIMode(IM_airBrush); 
		else SizeAirBrush();  break;    
	case 6:  NewButIMode(doLeft ? IM_rect:IM_frect, cpButton); break;    
	case 7:  NewButIMode(doLeft ? IM_circ:IM_fcirc, cpButton); break;
	case 8:  NewButIMode(doLeft ? IM_oval:IM_foval, cpButton); break;    
	case 9:  NewButIMode(doLeft ? IM_poly:IM_fpoly, cpButton); break;    
	case 10: if (but==1) NewIMode(IM_selBrush); 
		else {  CPInvAct(selbAct);   UserBr();   
			Delay(5);    CPInvAct(selbAct);
		    }  	break;
	case 11: NewIMode(IM_text1); break;    
	case 12: if (but==1) TogGridSt(); else NewIMode(IM_gridSpec); break;    
	case 13: if (but==1) TogSymSt(); else DoSymReq(); break;    
	case 14: ToglMagw();  break;    
	case 15: MWZoom( (but==1)? 1: -1 ); break;
	case 16: Undo();  break;    
	case 17: mClearToBg(); break;
	}
    }

extern void xorActBut();

/* Im not quite sure why I need (y-1) here but it works */

local SHORT actBoxForXY(x,y) SHORT x,y; { 
	return((SHORT)(MIN(x,cpWidth-2)/actUW+((y-1)/actUH)*actNPL)); }
    
local actMproc(ph,why,but,x,y) Pane *ph; MouseEvent why; SHORT but,x,y; {
	SHORT n = actBoxForXY(x,y);
	switch(why) {
	    case BDOWN: cpButton = but; downIn = n; 
		LSide = (VMapX(x)%24) + (VMapY(y-NYMag)% 24)< 24; xorActBut(n); 
		break;
	    case BUP: xorActBut(downIn);  
		ActButton(cpButton,downIn);
		break;
	}
    }


/* the "n" here is button number, not activity */
actX(n) SHORT n; { return(actwin.box.x + NXMag + actUW*(n%actNPL)); }
actY(n) SHORT n; { return(actwin.box.y + NYMag + actUH*(n/actNPL)); }

#define actPUH	12
#define actPUW  12
		
DispActBut(pic,n)
    UBYTE *pic;  /* pointer to the bits to be diplayed */
    int n; 	/* button # */
    {	
    int x,y;
    Box sbx;
    struct BitMap tmp;
    InitBitMap(&tmp,1,11,11);
    tmp.Planes[0] = pic;
    SetDrMd(&screenRP,JAM2);
    x = actX(n); y = actY(n);
    MagBits(&tmp,MakeBox(&sbx,0,0,11,11), screenRP.BitMap,
	x, y, NXMag,NYMag,&screenBox);
    }
   

/* #define slowway  */

#ifdef slowway
DispActMenu() {
    SHORT i;
    ColorBox(&actwin.box, BLACK);
    for (i=0; i< NACTBUTS; i++ ) DispActBut(actbm[i],i);
    }
#endif

#ifdef DBGCP
SHORT dbgcp = NO;
tstbm(bm) struct BitMap *bm; {
    SHORT w,h;
    w = bm->BytesPerRow*8;
    h = bm->Rows;
    SetAPen(&screenRP,0);
    RectFill(&screenRP,0,20, w-1, 20 + h - 1);
    BltBitMap(bm,0,0,screenRP.BitMap,0,20,w,h,REPOP,0xff,NULL);
    }
#endif

void DispActMenu() {
    SHORT i; Box sbx;
    struct BitMap tmp;
    SHORT h = actPUH*NACTBUTS/actNPL;
    InitBitMap(&tmp,1,32,h);
    if (tmpRas.RasPtr==NULL) return;
    tmp.Planes[0] = tmpRas.RasPtr;
    ColorBox(&actwin.box,BLACK); 
    ClearBitMap(&tmp);
    tempRP.BitMap = &tmp;
    SetDrMd(&tempRP,JAM2);
    SetAPen(&tempRP,1);
    for (i=0; i< NACTBUTS; i++ ) {
	BltTemplate(actbm[i],0,2,&tempRP,
	    actPUW*(i%actNPL)+1, actPUH*(i/actNPL)+1, 11, 11);
	}
    MagBits(&tmp,MakeBox(&sbx,0,0,25,h), screenRP.BitMap,
	actwin.box.x, actwin.box.y, NXMag, NYMag, &screenBox);
    WaitBlit();
    }

void xorActBut(but) SHORT but; {
    Box bx;
    if (!cpShowing) return;
    if (but==noActBut) return;
    XorScreenBox(MakeBox(&bx,actX(but),actY(but),actUW-NXMag,actUH-NYMag));
    }
    
/* two different activities are now associated with one button */
/* given an activity, determine the button # by the following: */
/* bit 5 denotes left button activity, bit 6 denotes right button*/
#define LBACT 32
#define RBACT 64
    
UBYTE butForAct[] = {
	noActBut,0,1,2,3,4,5,32+6,64+6,32+7,64+7,32+8,64+8,32+9,64+9,
	10,11,12,13,14,15,16,17 };

void CPInvAct(act) SHORT act; { xorActBut((SHORT)butForAct[act]); }

void CPShowBut(abut) SHORT abut; {
    SHORT butN;
    if (!cpShowing) return;
    if (abut==noActBut) return;
    butN = abut&0x1f;
    if (abut&LBACT) DispActBut(acthbm[butN-6], butN); 
    else {
	if (abut&RBACT) DispActBut(actfbm[butN-6], butN);
	else xorActBut(butN);
	}
    }
    
void CPClearBut()  {
    SHORT butN;
    if (!cpShowing) return;
    if (actBut==noActBut) return;
    butN = actBut&0x1f;
    if ((actBut&(LBACT|RBACT)))	DispActBut(actbm[butN], butN);
    else xorActBut(butN);
    }
	
void CPChgAct(new) SHORT new; {    
    if (new==activity) return;
    CPClearBut(); 
    activity = new;
    actBut = butForAct[new];
    CPShowBut(actBut);
    }

    
CPShowAct(act) { CPShowBut(butForAct[act]); }
    
void ActWPaint() {
    if (!cpShowing) return;
    actwin.box.y = mainBox.y + penH;
    actwin.box.x = screenBox.w - cpWidth;
    DispActMenu();
    if (gridON) CPShowAct(gridAct);
    if (symON) CPShowAct(symAct);
    if (magOn) CPShowAct(magAct);
    CPShowBut(actBut);
    }


/*--------------------------------------------------------------*/
/*      Current  Color Display			  		*/
/*--------------------------------------------------------------*/
local void cDspMproc(ph,why,but,x,y) Pane *ph; MouseEvent why; SHORT but,x,y; {
    if (why == BUP) if (but==2) ShowPallet();
    else  NewIMode(IM_readPix);
    }

local void cDspPaint()  {
    SHORT x,y; Box bx;
    if (!cpShowing) return;
    cdspwin.box.y = mainBox.y + penH + actH;
    cdspwin.box.x = screenBox.w - cpWidth;
    ColorBox(&cdspwin.box, BLACK);
    ScreenDest();
    x = cdspwin.box.x;
    y = cdspwin.box.y;
    TempMode(Replace,cpbgcol,solidpat);
    PFillBox(MakeBox(&bx,x+1,y+1,cpWidth-2,PMapY(24)-1));
    SetFGCol(cpfgcol);
    PFillCirc(x+(cpWidth-1)/2,y+PMapY(12),cpWidth/6);
    RestoreMode();
    PopGrDest();
    }


/*--------------------------------------------------------------*/
/*        Color Menu				  		*/
/*--------------------------------------------------------------*/

/*--- Paint the Color menu --------------------*/

void XorCol() {
    Box bx;
    if  (!cpShowing) return;
    bx.x = (cpfgcol/colNRows)*colUW + colwin.box.x;
    bx.y = (cpfgcol%colNRows)*colUH + colwin.box.y;
    bx.w = colUW+1;
    bx.h = colUH+1;
    ScreenDest();
    SetAPen(&screenRP,1);
    TempXOR();
    PThinFrame(&bx);
    RestoreMode();
    PopGrDest();
    }

void ColWPaint(ph,b) Pane *ph; Box *b; {
    SHORT pn,x,y,i,j;
    Box bx;
    if (!cpShowing) return;
    colwin.box.y = mainBox.y + penH + actH + colDspH;
    colUH = (screenBox.h - colwin.box.y)/colNRows;
    colwin.box.x = screenBox.w - cpWidth;
    ColorBox(MakeBox(&bx,colwin.box.x,colwin.box.y,colwin.box.w,
	colwin.box.h), BLACK);
    x=colwin.box.x+1;
    y=colwin.box.y+1;
    pn = 0;
    bx.w = colUW-1;  
    bx.h = colUH-1;   
    bx.x = x;
    ScreenDest();
    SetDrMd(&screenRP,JAM2);
    for (j=0; j<colNColumns; j++) {
	bx.y = y;
	for (i=0; i<colNRows; i++) {
	    SetFGCol(pn++);
	    PFillBox(&bx);
	    bx.y += colUH;
	    }
	bx.x += colUW;
	}
    PopGrDest();
    XorCol();
    }

    
local int colForXY(x,y) SHORT x,y; { 
    return(MIN(y,colUH*colNRows-1)/colUH+(MIN(x,cpWidth-2)/colUW)*colNRows);  
    }
	
local ColNewXY(button,x,y) SHORT button,x,y;{
    SHORT item = colForXY(x,y);
    if (button==1) if (item != cpfgcol)  CPChgCol(item);  else;
    else if (item!= cpbgcol) CPChgBg(item); 
    }

local colMproc(ph,why,but,x,y) Pane *ph; MouseEvent why; SHORT but,x,y; {
    if (why == BDOWN) { ColNewXY(but,x,y);
	    if (but==1) SetFGCol(cpfgcol); else  SetXPCol(cpbgcol);
	    }
   }

CPChgCol(new) SHORT new;{ 
    SetFGCol(new); 
    if (new!=cpfgcol) {
	if (cpfgcol>=0) XorCol();
	cpfgcol = new;
	cDspPaint();
	XorCol();
	}
    SetCycPaint(); /* keep the current cycle up to date */
    }
    
CPChgBg(new) SHORT new; { if (new<nColors) {SetXPCol(cpbgcol=new); 
	cDspPaint();}
    }

/* ------------------------- */
InstCP() {
    PaneInstall(&penwin); 
    PaneInstall(&actwin); 
    PaneInstall(&cdspwin);
    PaneInstall(&colwin);
    cpShowing = YES;
    } 

RmvCP()  {
    PaneRemove(&penwin);  
    PaneRemove(&actwin);  
    PaneRemove(&cdspwin);
    PaneRemove(&colwin);
    cpShowing = NO;
    }

ShowCtrPan(st) BOOL st; { if (st!=cpShowing) TogBoth(); }
