/*----------------------------------------------------------------------*/
/*									*/
/*		menu.c -- Pull Down Menus				*/
/*									*/
/*----------------------------------------------------------------------*/

#include "system.h"
#include "prism.h"
#include <librarie\diskfont.h>

#define fudgeFact	12 /* make this 12 for V1.1 */

extern SHORT xShft,yShft,curFormat;
extern struct RastPort *mainRP;
extern void Undo(),TogGridSt();
extern void DoFontMI();
extern struct Window *mainW;
extern Box bigBox;
extern BOOL midHandle;
extern SHORT curpen; 
extern PaintMode paintMode;
extern struct Screen *screen;

#define local static

#define TXHEIGHT  8

struct TextAttr curMIFont = { "topaz.font",TXHEIGHT,0,0 };
    
#define NMEN 4

#define NOCOM 0		/* indicates no command key */
#define NOEXCL 0	/* indicates no mutual exclusion */
#define NOSELFILL NULL	/* indicates no selection highlite bitmap */
#define NOFONT NULL	/* no font defined yet */
#define NOSUBIT NULL	/* no sub items defined */

#define TXTFLAGS   (ITEMTEXT | ITEMENABLED | HIGHCOMP) /* or HIGHBOX*/
#define COMFLAGS  (TXTFLAGS | COMMSEQ)
#define ITInit(name)  {0,1,JAM1,0,1,NOFONT,name,NULL}

#define MIW 64
#define MIH 20		/* in 640x400 coords */

void NopMI(item,subi) SHORT item,subi; {
    KPrintF(" NopMI: item = %ld, subi = %ld \n",item,subi);
    }

#define SUBXOFFS 46
#define MIInit(next,itname) {next,0,0,MIW,MIH,TXTFLAGS,NOEXCL,(APTR)itname,NOSELFILL,NOCOM, NULL,0}
#define SubInit(next,itext,subm)  {next,0,0,MIW,MIH,TXTFLAGS,NOEXCL, (APTR)itext,NOSELFILL,NOCOM,subm,0}
#define FirstSub(next,itname)  {next,SUBXOFFS,10,MIW,MIH,TXTFLAGS,NOEXCL, (APTR)itname,NOSELFILL,NOCOM,NULL,0}
    
/*----------------------------------------------------------------------*/
/*     Picture Menu 							*/
/*----------------------------------------------------------------------*/

    
UBYTE saveString[] = "Save         ";
StuffSaveStr(s) UBYTE *s; { strncpy(&saveString[5], s, 8);}
    
local struct IntuiText itOp = ITInit("Load...");
local struct IntuiText itSav = ITInit(saveString);
local struct IntuiText itSavAs = ITInit("Save As...");
local struct IntuiText itUndo = ITInit("Undo");
local struct IntuiText itSet = ITInit("Settings");
local struct IntuiText itSymCent = ITInit("Symmetry Center");
local struct IntuiText itColCon = ITInit("Color Control");

local struct IntuiText itUsePal = 	ITInit("Use Brush Palette");
local struct IntuiText itRestPal = 	ITInit("Restore Palette");
local struct IntuiText itDefPal = 	ITInit("Default Palette");
local struct IntuiText itPalette = 	ITInit("Palette    p");
local struct IntuiText itCCycle = 	ITInit("Cycle      TAB");

local struct IntuiText itSpare = ITInit("Spare");
local struct IntuiText itSwap = ITInit("Swap      j");
local struct IntuiText itCopyTo = ITInit("Picture To Spare");
local struct IntuiText itMrgSpFront = ITInit("Merge in front");
local struct IntuiText itMrgSpBack = ITInit("Merge in back");

local struct IntuiText itPrint = ITInit("Print");
local struct IntuiText itQuit = ITInit("Quit");

#ifdef doraw
local struct IntuiText itRawIO = ITInit("Raw Bitmap IO");
local struct MenuItem rawIOsub[3] = {
    FirstSub(&rawIOsub[1],&itOp),
    MIInit(&rawIOsub[2], &itSav),
    MIInit(NULL, &itSavAs)
    };

void doRawIO(item,subi) int item,subi; {
    switch(subi) {
	case 0: OpenRaw(); break;
	case 1: SaveRaw(); break;
	case 2: SaveRawAs(); break;
	}
    }
#endif

local struct MenuItem colConSub[5] = {
    FirstSub(&colConSub[1], &itPalette),
    MIInit(&colConSub[2], &itUsePal),
    MIInit(&colConSub[3], &itRestPal),
    MIInit(&colConSub[4], &itDefPal),
    MIInit(NULL, &itCCycle)
    };

local struct MenuItem spareSub[4] = {
    FirstSub(&spareSub[1], &itSwap),
    MIInit(&spareSub[2], &itCopyTo),
    MIInit(&spareSub[3], &itMrgSpFront),
    MIInit(NULL, &itMrgSpBack)
    };

local struct MenuItem picMI[8] = {
    MIInit(&picMI[1], &itOp),
    MIInit(&picMI[2], &itSav),
    MIInit(&picMI[3], &itSavAs),
    MIInit(&picMI[4], &itSymCent),
    SubInit(&picMI[5], &itColCon, colConSub),
    SubInit(&picMI[6], &itSpare, spareSub),
    MIInit(&picMI[7], &itPrint),
    MIInit(NULL,&itQuit)
    };

extern void OpenPic(), SavePic(), SavePicAs();
extern void PaneStop(), PrintPic(), DoSymReq();
extern void TogColCyc(), SwapSpare();
extern void ShowPallet();

void SpecSymCenter(){ NewIMode(IM_symCent); }

local void doColCon(item,subi) int item,subi; {
    switch(subi) {
	case 0: ShowPallet(); break;
	case 1: UseBrPalette(); break;
	case 2: RestorePalette(); break;
	case 3: DefaultPalette(); break;
	case 4: TogColCyc(); break;
	}
    }

local void doSpare(item,subi) int item,subi; {
    switch(subi) {
	case 0: SwapSpare(); break;
	case 1: CopyToSpare(); break;
	case 2: MrgSpareFront(); break;
	case 3: MrgSpareBack(); break;
	}
    }

/* disable refreshing after overlays, since I do it as a matter of 
course after all io operations */
void DoOpen() { OffRefrAfterOv(); OpenPic(); OnRefrAfterOv(); }
void DoSave() { OffRefrAfterOv(); SavePic(); OnRefrAfterOv();}
void DoSaveAs() { OffRefrAfterOv(); SavePicAs(); OnRefrAfterOv(); }
local void (*picProc[])() =  { DoOpen, DoSave, DoSaveAs,
    SpecSymCenter, doColCon, doSpare, PrintPic, PaneStop };

local void DoPic(item,subi) int item,subi; { (*picProc[item])(item,subi); }

/*----------------------------------------------------------------------*/
/*     Brush Menu 							*/
/*----------------------------------------------------------------------*/
local struct IntuiText itSize = 	ITInit("Size");
local struct IntuiText itRot = 		ITInit("Rotate");
local struct IntuiText itFlip = 	ITInit("Flip");
local struct IntuiText itChgCol = 	ITInit("Change Color");
local struct IntuiText itBrHFlip =      ITInit("Horiz        x");
local struct IntuiText itBrVFlip =      ITInit("Vert         y");
local struct IntuiText itBrRot90 = 	ITInit("90 Degrees   z");
local struct IntuiText itAny = 		ITInit("Any Angle");
local struct IntuiText itBrStretch = 	ITInit("Stretch      Z");
local struct IntuiText itBrHalf = 	ITInit("Halve        h");
local struct IntuiText itBrDubl = 	ITInit("Double       H");
local struct IntuiText itBrDublX = 	ITInit("Double Horiz");
local struct IntuiText itBrDublY = 	ITInit("Double Vert");
local struct IntuiText itBrShr = 	ITInit("Shear");
local struct IntuiText itBrBgToFg = 	ITInit("Bg -> Fg");
local struct IntuiText itRemap = 	ITInit("Remap ");
local struct IntuiText itBend = 	ITInit("Bend");
local struct IntuiText itHoriz = 	ITInit("Horiz");
local struct IntuiText itVert = 	ITInit("Vert");

local struct MenuItem brSizeSub[5] = {
    FirstSub(&brSizeSub[1], &itBrStretch),
    MIInit(&brSizeSub[2], &itBrHalf),
    MIInit(&brSizeSub[3], &itBrDubl),
    MIInit(&brSizeSub[4], &itBrDublX),
    MIInit(NULL, &itBrDublY)
    };

local struct MenuItem brFlipSub[2] = {
    FirstSub(&brFlipSub[1], &itBrHFlip), 
    MIInit(NULL, &itBrVFlip)
    };

local struct MenuItem brRotSub[3] = {
    FirstSub(&brRotSub[1], &itBrRot90),
    MIInit(&brRotSub[2], &itAny),
    MIInit(NULL, &itBrShr)
    };

local struct MenuItem chgColSub[2] = {
    FirstSub(&chgColSub[1], &itBrBgToFg),
    MIInit(NULL, &itRemap),
    };

local struct MenuItem bendSub[2] = {
    FirstSub(&bendSub[1], &itHoriz),
    MIInit(NULL, &itVert),
    };

local struct MenuItem brushMI[7] = {
    MIInit(&brushMI[1], &itOp),
    MIInit(&brushMI[2], &itSavAs),
    SubInit(&brushMI[3], &itSize, brSizeSub),
    SubInit(&brushMI[4], &itFlip, brFlipSub),
    SubInit(&brushMI[5], &itRot, brRotSub),
    SubInit(&brushMI[6], &itChgCol, chgColSub),
    SubInit(NULL, &itBend, bendSub)
    };

extern void GetBrs(),  SaveBrsAs(), BrFlipX(), BrFlipY();
extern void BrRot90(), BrHalf(), BrDubl(),BrBgToFg(), BrRemapCols();

void DoBrs(item,subi) int item,subi; {
    switch(item) {
	case 0: OffRefrAfterOv(); GetBrs(); OnRefrAfterOv(); break;
	case 1: OffRefrAfterOv(); SaveBrsAs(); OnRefrAfterOv(); break;
	case 2: switch(subi) {
	    case 0: NewIMode(IM_strBrush); break;
	    case 1: BrHalf(); break;
	    case 2: BrDubl(); break;
	    case 3: BrDublX(); break;
	    case 4: BrDublY(); break;
	    }    break;
	case 3: switch(subi) {
	    case 0: BrFlipX(); break;
	    case 1: BrFlipY(); break;
	    }    break;
	case 4: switch(subi) {
	    case 0: BrRot90(); break;
	    case 1:  NewIMode(IM_rotBrush); break;
	    case 2:  NewIMode(IM_shrBrush); break;
	    }    break;
	case 5: switch(subi) {
	    case 0: BrBgToFg(); break;
	    case 1: BrRemapCols();break;
	    }   break;
	case 6: switch(subi) {
	    case 0: NewIMode(IM_hbendBrush); break;
	    case 1: NewIMode(IM_vbendBrush);break;
	    }   break;
	}
    }

/*----------------------------------------------------------------------*/
/*  Paint Mode Menu 							*/
/*----------------------------------------------------------------------*/

extern UBYTE *modeNames[];

#define ONES 0xffffffff
#define MICheck(next,itname,itnum,checkme) {next,0,0,MIW,MIH,TXTFLAGS|CHECKIT|(checkme), ONES^(1<<(itnum)),(APTR)itname,NOSELFILL,NOCOM, NULL,0}
    
local struct IntuiText itMask = 	ITInit("Object  F1  ");
local struct IntuiText itColor = 	ITInit("Color   F2  ");
local struct IntuiText itRplc = 	ITInit("Replace F3  ");
local struct IntuiText itSmear = 	ITInit("Smear   F4  ");
local struct IntuiText itShade = 	ITInit("Shade   F5  ");
local struct IntuiText itBlend = 	ITInit("Blend   F6  ");
local struct IntuiText itCycPaint = 	ITInit("Cycle   F7  ");
        
local struct MenuItem modeMI[7] = {
    MICheck(&modeMI[1], &itMask,0,0),
    MICheck(&modeMI[2], &itColor,1,0),
    MICheck(&modeMI[3], &itRplc,2,0),
    MICheck(&modeMI[4], &itSmear,3,0),
    MICheck(&modeMI[5], &itShade,4,0),
    MICheck(&modeMI[6], &itBlend,5,0),
    MICheck(NULL, &itCycPaint,6,0)
    };

SHORT userPntMode = 0;
    
UserPaintMode(m) int m; {
    int pntmd;
    if ((m==Blend)||(m==Smear)||(m==Shade)) NewIMode(IM_shade);
    if (m==CyclePaint) { OnCycPaint(); pntmd = Color;}
    else { OffCycPaint(); pntmd = m; }
    SetPaintMode(pntmd);  
    userPntMode = m;
    }

void DoMode(item,subi) int item,subi; { 
    if (!LegalMode(item)) {
	modeMI[item].Flags &= (~CHECKED);
	modeMI[userPntMode].Flags |= CHECKED;
	return;
	}
    UserPaintMode(item);  
    }

#define MODEMENNUM 2
ModeMINum(m) int m; {  return((m<<5)+MODEMENNUM );  }

void SetModeMenu(i) int i; {
    int j,miNum;
    if (!LegalMode(i)) return;
    modeMI[userPntMode].Flags &= (~CHECKED);
    modeMI[i].Flags |= CHECKED;
    for (j=0; j<7; j++) {
	miNum = ModeMINum(j);
	if (LegalMode(j)) OnMenu(mainW,miNum);
	else OffMenu(mainW,miNum);
	}
    UserPaintMode(i);
    DispPntMode();
    }
    

/*----------------------------------------------------------------------*/
/*     Font Menu 							*/
/*----------------------------------------------------------------------*/
local struct IntuiText itLoadFonts = ITInit("Load Fonts");
local struct MenuItem miLoadFonts =  MIInit(NULL, &itLoadFonts);

extern struct AvailFonts *fontDir;
local void DoFontMI(item,subi) int item,subi;  {
    if (fontDir == NULL) LoadFontDir();
    else LoadAFont(item);
    }


/*----------------------------------------------------------------------*/
/*     Prefs Menu 							*/
/*----------------------------------------------------------------------*/
local struct IntuiText prefIT[] = {
	ITInit(" Brush Handle"),
	ITInit(" Coordinates"),
	ITInit(" Fast Feedback")
	};

#define  CKFLAGS TXTFLAGS/*|CHECKIT*/
#define CkInit(next,itname) {next,0,0,MIW,MIH,CKFLAGS,NOEXCL,(APTR)itname,NOSELFILL,NOCOM, NULL,0}

local struct MenuItem prefsMI[3] = {
    MIInit(&prefsMI[1], &prefIT[0]),
    MIInit(&prefsMI[2], &prefIT[1]),
    MIInit(NULL, &prefIT[2]),
    };

extern BOOL dispCoords, fastFB, modeHelp;

local UWORD prefState = 0;
    
local void TogCoords() { dispCoords = !dispCoords; }
local void TogMidHandle() { midHandle = !midHandle;    }
local void TogFFB() { fastFB = !fastFB; FixUsePen();   }

local void (*prefsProc[])() = {TogMidHandle, TogCoords, TogFFB};
local void DoPrefs(item,subi) SHORT item,subi; {
    UWORD bit;
    (*prefsProc[item])(subi);
    bit = 1<<item;
    prefState ^= bit; 
    prefIT[item].IText[0] = (prefState&bit) ? '*': ' ';
    }

/*----------------------------------------------------------------------*/
/*     Main Menu Bar 							*/
/*----------------------------------------------------------------------*/
#define MenW 48

/* initializer for Menu */
struct Menu menus[5] = {     
    { &menus[1],0,0,MenW,10,MENUENABLED,"Picture",picMI},
    { &menus[2],0,0,MenW,10,MENUENABLED,"Brush",brushMI}, 
    { &menus[3],0,0,MenW,10,MENUENABLED,"Mode",modeMI},  
/* FontMenu must point at this: */
    { &menus[4],0,0,MenW,10,MENUENABLED,"Font",&miLoadFonts}, 
    { NULL,0,0,MenW,10,MENUENABLED,"Prefs",prefsMI}
    };

struct Menu *FontMenu = &menus[3]; 
struct Menu *MainMenu = menus;

void (*MenProcs[])() = {DoPic,DoBrs,DoMode,DoFontMI,DoPrefs};

DoMenEvent(code) SHORT code;  {
    SHORT item,subi;
    if (code != (SHORT)MENUNULL ) {
	item = ITEMNUM(code);
	subi = SUBNUM(code);
	(*MenProcs[MENUNUM(code)])(item,subi);
	}
    DispPntMode();
    LoadIMode(); /*bring in overlay */
    }
    
#ifdef nowInDpinit /***##################### */

ComWidth() { return( (curFormat>0)? COMMWIDTH: LOWCOMMWIDTH); }

SHORT MaxMiLength(mi) struct MenuItem *mi; {
    struct IntuiText *it;
    SHORT w,max;
    for (max = 0; mi != NULL;  mi = mi->NextItem) {
	it = (struct IntuiText *)(mi->ItemFill);
	w = TextLength(mainRP,it->IText,strlen(it->IText)) + fudgeFact;
	if (mi->Flags&COMMSEQ) w += ComWidth();
	max = MAX(w,max);
	}
    return((SHORT)(max-10));
    }
    
InitMItems(mi,minW) struct MenuItem *mi; SHORT minW; {
    SHORT i , y, x;
    SHORT wid = MAX(MaxMiLength(mi) + widCorrect[curFormat], minW );
    y = mi->TopEdge;
    x = mi->LeftEdge;
    for ( i=0 ; mi != NULL;  ++i, mi = mi->NextItem) {
	if ((mi->Flags&ITEMTEXT)&&(mi->Flags&CHECKIT))
	 ((struct IntuiText *)(mi->ItemFill))->LeftEdge = checkW[curFormat];
	mi->TopEdge = y; 
	mi->LeftEdge = x; 
	mi->Width = wid;
	if ((mi->Flags)&COMMSEQ) mi->Width += ComWidth();
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

#endif