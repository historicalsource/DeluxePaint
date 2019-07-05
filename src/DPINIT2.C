/*----------------------------------------------------------------------*/
/*									*/
/*		dpinit2.c -- initialization				*/
/*									*/
/*----------------------------------------------------------------------*/
#include <system.h>
#include <prism.h>

#define local static
#define fudgeFact 10

extern struct Menu *MainMenu;
extern SHORT curFormat;

extern struct RastPort *mainRP;
extern struct TextAttr curMIFont;
extern SHORT xShft,yShft;
extern struct Menu menus[];
extern struct Screen *screen;

SHORT widCorrect[3] = {-4,0,12};
SHORT menuHeight[3] = {10,10,16};
SHORT checkW[3] = {LOWCHECKWIDTH,CHECKWIDTH,CHECKWIDTH};
SHORT curMIH;

SHORT ComWidth() { return((SHORT)( (curFormat>0)? COMMWIDTH: LOWCOMMWIDTH)); }

SHORT MaxMiLength(mi) struct MenuItem *mi; {
    struct IntuiText *it;
    SHORT w,max;
    for (max = 0; mi != NULL;  mi = mi->NextItem) {
	it = (struct IntuiText *)(mi->ItemFill);
	w = TextLength(mainRP,it->IText,strlen(it->IText))+12;
	if (mi->Flags&COMMSEQ) w += ComWidth();
	if (mi->Flags&CHECKIT) w += checkW[curFormat];
	max = MAX(w,max);
	}
    return((SHORT)(max-10));
    }
    
InitMItems(mi,minW) struct MenuItem *mi; SHORT minW; {
    SHORT i , y, x;
    SHORT wid = MAX(MaxMiLength(mi) /*+widCorrect[curFormat]*/, minW );
    y = mi->TopEdge;
    x = mi->LeftEdge;
    for ( i=0 ; mi != NULL;  ++i, mi = mi->NextItem) {
	if ((mi->Flags&ITEMTEXT)&&(mi->Flags&CHECKIT))
	 ((struct IntuiText *)(mi->ItemFill))->LeftEdge = checkW[curFormat];
	mi->TopEdge = y; 
	mi->LeftEdge = x; 
	mi->Width = wid;
	if (mi->Flags&COMMSEQ) mi->Width += ComWidth();
	mi->Height = curMIH;
	((struct IntuiText *)(mi->ItemFill))->ITextFont = &curMIFont;
	y += curMIH;
	if (mi->SubItem!=NULL) InitMItems(mi->SubItem,0);
	}
    }


InitMenu() {
    SHORT i = 0,xl = PMapX(12);
    struct Menu *mn = MainMenu;
    curMIH =  menuHeight[curFormat];
    for (mn = menus; mn != NULL; ++i,mn = mn->NextMenu ) {
	mn->LeftEdge = xl; 
	mn->Width = TextLength(screen->BarLayer->rp,
	    mn->MenuName,strlen(mn->MenuName)) +
	    widCorrect[curFormat] + fudgeFact;
	xl += mn->Width;
	mn->Flags = MENUENABLED;
	InitMItems(mn->FirstItem, mn->Width);
	}
    }



/* ----------------- Control Panel Inititialization -------------------*/
    
local BYTE nrowTab[] = {1,2,4,4,8,8};
local BYTE ncolTab[] = {1,1,1,2,2,4};

extern SHORT NXMag,NYMag;
extern BOOL cpShowing;
extern SHORT penH,actH,colDspH,colBoxH,actUH,colNRows,curDepth,colNColumns;
extern SHORT colUW,penUW,actUW,cpWidth;
extern Pane penwin,actwin,cdspwin,colwin;
extern void penMproc(), PenWPaint(), actMproc(), ActWPaint();
extern void cDspMproc(), cDspPaint(),colMproc();
extern void mainCproc(), ColWPaint();

/* Initialize the control panel geometry and create Windows */

CPInit(){
    Box bx;
    SHORT colBoxH;
    cpShowing = NO;
    NXMag =  NYMag = 1;
    if (curFormat==2) NYMag = 2;
    if (curFormat!=0) NXMag = 2;

    /* penH + actH + colDspH = colBoxH = 400 */

    penH = PMapY(40);
    actH = PMapY(216); 

    colDspH = PMapY(24);
    colBoxH = PMapY(120);

    actUH = PMapY(24);
    colNRows = nrowTab[curDepth];
    colNColumns = ncolTab[curDepth];
    cpWidth = PMapX(48)+1;
    colUW = (cpWidth-1)/colNColumns;
    penUW = (cpWidth-1)/3;
    actUW = (cpWidth-1)/2;
    PaneCreate(&penwin,MakeBox(&bx,0,0,cpWidth,penH),HANGON,&mainCproc,
	penMproc,PenWPaint,NULL);
    PaneCreate(&actwin,MakeBox(&bx,0,penH,cpWidth,actH),HANGON,&mainCproc,actMproc,
	ActWPaint,NULL);
    PaneCreate(&cdspwin,MakeBox(&bx,0,penH+actH,cpWidth,colDspH),HANGON,&mainCproc,
	cDspMproc, cDspPaint,NULL);
    PaneCreate(&colwin,MakeBox(&bx,0,penH+actH+colDspH,cpWidth,colBoxH),HANGON,
	mainCproc,colMproc,ColWPaint,NULL);
    }

