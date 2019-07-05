/*----------------------------------------------------------------------*/
/*									*/
/*		dpinit.c -- initialization code				*/
/*									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>
#include <librarie\dosexten.h>
#include <dphook.h>
#include <workbenc\startup.h>
	
#define CLOSEWBNCH

#define local static
#define MaxWidth	640
#define MaxHeight	400

extern BOOL modeHelp;
extern void mainRefresh();
extern struct Menu *MainMenu; 
extern BMOB curpenob,curbr,origBr;
extern void mainCproc(),mainPproc(),mainMproc();
extern long GfxBase;
extern long IntuitionBase;
extern long IconBase;

struct Window *OpenWindow();
struct InputEvent *Intuition();
struct Screen *OpenScreen();

extern struct Window *mainW;
extern struct Screen *screen;
extern struct RastPort *mainRP;
extern struct ViewPort *vport;
extern BOOL haveWBench;
extern BOOL underWBench;
extern Box mainBox;
extern Pane mainP;
extern SHORT curFormat; 		/* current screen format */
extern SHORT curDepth;
extern SHORT curMBarH;
extern SHORT xShft,yShft;
extern SHORT nColors;
    
extern SHORT prevColors[32];
extern UBYTE initBGCol[], initFGCol[];
extern Box screenBox;
extern struct BitMap hidbm;
extern struct TmpRas tmpRas;
extern struct RastPort tempRP;
extern struct RastPort screenRP;

extern UBYTE picname[];
extern struct TextAttr TestFont;
extern SHORT fmt,deep;
extern struct Process *myProcess;
extern struct Window *svWindowPtr;
extern UBYTE dpaintTitle[];

local struct NewScreen ns = {
    0, 0, 		/* start position 		*/
    640, 200, 3,	/* width, height, depth 	*/
    0, 1,		/* detail pen, block pen 	*/
    HIRES,		/* viewing mode 		*/
    CUSTOMSCREEN,	/* screen type 			*/
    &TestFont,		/* font to use 			*/
    dpaintTitle,	/* default title for screen 	*/
    NULL 		/* pointer to additional gadgets */
    };

local struct NewWindow nw = {0};

TryOpenLib(name) char *name; {
    LONG libadr = OpenLibrary(name, 0);
    if (libadr == NULL) { InfoMessage("Can't open library",name); exit(); }
    return(libadr);
    }

CloseWB() {
    struct Window *tw,*dosWin;
    if (!underWBench) {
	nw.Type = WBENCHSCREEN;
	nw.Width = nw.Height = 1;
	tw = OpenWindow(&nw);
	dosWin = tw->Parent;
	CloseWindow(tw);
	CloseWindow(dosWin);
	}
    if (CloseWorkBench()) haveWBench = NO;
    else InfoMessage(" Couldn't close", "WorkBench");
    }

BailOut() { 
    myProcess->pr_WindowPtr = (APTR)svWindowPtr;
    OpenWorkBench(); 
    CloseLibrary(GfxBase);
    CloseLibrary(IntuitionBase);
    exit(); 
    }

extern MagContext magCntxt;
extern Pane magP;
extern void magRefresh();
extern void RecChange();

InitMag() {
    magCntxt.srcRP = mainRP;
    magCntxt.srcBox = &mainP.box;
    magCntxt.magBM = mainRP->BitMap;
    magCntxt.magBox = &magP.box;
    magCntxt.updtProc = &RecChange;
    magCntxt.srcPos.x = 0;
    magCntxt.srcPos.y = curMBarH;
    magCntxt.magN = PMapX(10);
    PaneCreate(&magP, &screenBox ,HANGON, mainCproc, mainMproc, magRefresh, MAGWIN);
    SetMagContext(&magCntxt);
    MagState(NO);
    }

struct TextFont *screenFont= NULL;
        
InitScreenFont() {
    screenFont = (struct TextFont *)OpenFont(&TestFont);
    SetFont(&screenRP, screenFont);
    }

/*------initialize current brush ----------------*/
extern struct BitMap brSVBM,brBM,penSVBM,penBM;
extern BMOB curbr,curpenob;

/** init default picture path **/
extern UBYTE picpath[], *picpths[];
local UBYTE *picpths[3] = {"lo-res","med-res","hi-res"};
InitPicPath() {strcpy(picpath,picpths[curFormat]); }

extern struct BitMap *clientBitMap;
extern DPHook *dphook;
extern BOOL myDPHook;
extern UBYTE DPName[];
extern BOOL largeMemory, loadAFile;

#ifdef DOWB
extern struct WBStartup *WBenchMsg;
#endif
#define HALFMEG (1<<19)
    
DPInit(argc,argv) int argc; char *argv[]; {
    SHORT fmt,deep;
    SHORT initbg,initfg;
    BOOL closeWB= NO,doOverlay = NO, keepWB = NO;
    LONG avMem;
    
    FreeUpMem(); /* free up any memory with 0 ref count */
    
    avMem = AvailMem(MEMF_PUBLIC);
    largeMemory =  (avMem>(256*1024));
    
    GfxBase = TryOpenLib("graphics.library");
    IntuitionBase = TryOpenLib("intuition.library");
    IconBase = TryOpenLib("icon.library");

    myProcess = (struct Process *)FindTask(0);
    svWindowPtr = (struct Window *)myProcess->pr_WindowPtr;

    fmt = 0; deep = 5;		/* default to LORES, 5 planes */

#ifdef DOWB
    /* --- Test if we were called from the CLI or WORKBENCH --- */
    if (argc==0) {
	underWBench = YES;
	if (WBenchMsg->sm_NumArgs>0) {
	    strcpy(picname,WBenchMsg->sm_ArgList->wa_Name);
	    loadAFile = YES;
	    }                       
	}
    else { underWBench = NO; 	}
#endif    
    
    /* -----  See if Deluxe Video or anybody has planted hook */

    dphook = (DPHook *)FindHook(DPName);
    
    if ((dphook!=NULL)/*&&largeMemory*/) {
	fmt = dphook->format;
	clientBitMap = dphook->bitmap;
	if (clientBitMap != NULL) {
	    hidbm = *clientBitMap; /* copy record, not pointer */
	    deep = MIN(hidbm.Depth,5);
	    doOverlay = YES;
	    }
	else InfoMessage(" Null Client"," Bitmap");
	}
    else {
	if (argc>1)
	    switch(*argv[1]) {
		case 'l': fmt = 0; break;
		case 'm': fmt = 1; break;
		case 'h': fmt = 2; break;
		}
    
	if (argc>2) deep = MAX(MIN((*argv[2]-'0'),5),1);

	if (argc>3)  switch (*argv[3]) {
	    case 'c': closeWB = YES; break;
	    case 'o': doOverlay = YES; break;
	    case 'n': keepWB = YES; break;
	    default: doOverlay = closeWB = YES; break;
	    }
		
	if (!largeMemory) { /* 256K machine */
	    if (fmt==2) BailOut(); /* no way */
	    else closeWB = doOverlay = YES;
	    }

	else if ((avMem<HALFMEG)&&(fmt==2)&&(deep>3)) closeWB = doOverlay = YES;
    
	}

    if (closeWB&&(!keepWB)) CloseWB();

    SetOverlay(doOverlay);
    
    SetFormat(fmt,deep); /* opens Screen, mainW, allocs hidbm, tmpRas */

    
    if (dphook==NULL) { /* --- plant my own hook --- */
	myDPHook = YES;
	dphook = (DPHook *)DAlloc(sizeof(DPHook),MEMF_PUBLIC);
	dphook->bitmap = &hidbm;
	dphook->dpScreenBitMap = mainRP->BitMap;
	dphook->format = fmt;
	dphook->refCount = 0;
	dphook->version = 1;
	SetHook(DPName,dphook);
	}
    else {
	myDPHook = NO;
	dphook->refCount++;
	}
    
    dphook->vport = vport;
    dphook->brush = &curbr;
    
    InitMenu(); 
    
    BasicBandW();

    SetMenuStrip(mainW, MainMenu);

    InitRastPort(&tempRP);	/* multi-purpose RasterPort */
    InitRastPort(&screenRP);	/* always points at bare screen bitmap */
    screenRP.BitMap = mainRP->BitMap;
   
    InitScreenFont();
 
    initbg = initBGCol[curDepth];
    initfg = initFGCol[curDepth];
    SetRast(mainRP,initbg);
    tempRP.BitMap = &hidbm;

    if (clientBitMap==NULL) SetRast(&tempRP,initbg);

    PaneCreate(&mainP,&mainBox,HANGON,mainCproc,mainMproc,
		mainRefresh,MAINWIN);
    PaneInstall(&mainP);

    InitPGraph();
    PushGrDest(mainRP,&mainBox);
    SolidPattern();

    CPInit();	
    InitMag();

    CPChgPen(0);
    CPChgCol(initfg);
    CPChgBg(initbg);

    SymSet(6, YES, PMapX(MaxWidth/2), PMapY(MaxHeight/2));

    TogBoth();	/* show the control panel initially */
    ResetCursor();
    SetModeMenu(modeHelp ? Color: Mask);
    DispPntMode();
	}

local USHORT fmtMode[3] = {0,HIRES,HIRES|LACE};
local SHORT fmtXShft[3] = {1,0,0};
local SHORT fmtYShft[3] = {1,1,0};

#ifdef ldkfg
CloseDisp() {        
    ShowTitle(screen,NO);
    ClearMenuStrip(mainW);
    CloseFont(screenFont);
    KillCCyc();
    FreeSpare();
    BMOBFreeTmp();
    FreeBMOB(&origBr);
    FreeBMOB(&curbr);
    FreeBMOB(&curpenob);
    FreeBitMap(&hidbm);
    FreeTmpRas();
    myProcess->pr_WindowPtr = (APTR)svWindowPtr;
    CloseWindow(mainW);
    CloseScreen(screen); 
    }

ShutDown() {
    FreeFonts();
    CloseLibrary(GfxBase);
    CloseLibrary(IntuitionBase);
    OpenWorkBench();
    }

#endif
    
CloseDisplay() {
    ShowTitle(screen,NO);
    ClearMenuStrip(mainW);
    FreeFonts();
    KillCCyc();
    FreeSpare();
    BMOBFreeTmp();
    FreeBMOB(&origBr);
    FreeBMOB(&curbr);
    FreeBMOB(&curpenob);
    
    if (clientBitMap == NULL) FreeBitMap(&hidbm);
    if (myDPHook){ RemHook(DPName);DFree(dphook);}
    else dphook->refCount--;
    
    FreeTmpRas();
    myProcess->pr_WindowPtr = (APTR)svWindowPtr;
    CloseWindow(mainW);
    CloseScreen(screen); 
    CloseLibrary(GfxBase);
    CloseLibrary(IntuitionBase);
    OpenWorkBench();
    }

/*----------------------------------------------------------------------*/
/*									*/
/*  Memory requirement							*/
/*
    2*depth*pagesize	for visible display and hidbm
    pagesize		for tmpRas
    (depth*pagesize)/WorkSpaceFactor	for brush (minimal)
    MINSPACE		spare for overlays,menus, etc, fixed size

    total = depth*(2*pagesize + pagesize/WorkSpaceFactor) + MINSPACE + pagesize;
    max depth = (memavail-pagesize-MINSPACE)/(2*pagesize + pagesize/WorkSpaceFactor);

	 		    (mem-pagesize-MINSPACE)
	=	-----------------------------------------
	 	  (2*pagesize + pagesize/WorkSpaceFactor)
------------------------------------------------------------------------*/

/* require 1/3 of screen worth of workspace */
#define WorkSpaceFactor 3

/* minimum additional space of 12k */
#define MINSPACE 12*1024

/*----assumes screenBox already setup */
int MaxDepth(fmt) SHORT fmt; {
    ULONG psz = PlaneSize(screenBox.w,screenBox.h);
    ULONG mem = AvailMem(MEMF_CHIP|MEMF_PUBLIC);
    return((int) ( (mem-psz-MINSPACE)/(2*psz + psz/WorkSpaceFactor)) );
    }
 
SetFormat(fmt,depth) 
    SHORT fmt;	  /* 0 => 320x200xn, 1 => 640x200xn, 2 => 640x400xn*/
    SHORT depth;  /* number of planes 	*/ 
    {
    if (fmt>0) depth = MIN(depth,4);
    ns.ViewModes = fmtMode[fmt];
    curFormat = fmt;
    xShft = fmtXShft[fmt];
    yShft = fmtYShft[fmt];
    screenBox.w = ns.Width = PMapX(MaxWidth);
    screenBox.h = ns.Height = PMapY(MaxHeight);
    curMBarH =  /*(curFormat==2)? 19:*/ 11;
    mainBox.x = 0;
    mainBox.y = curMBarH;
    mainBox.w = screenBox.w;
    mainBox.h = screenBox.h - mainBox.y;

    depth = MIN(MaxDepth(fmt),depth);
    if (depth==0) BailOut(); 
    
    ns.Depth = curDepth = depth;
    
    screen = OpenScreen(&ns); 
    if (screen == NULL)  BailOut();
    
    nw.Screen = screen;	
    nw.DetailPen = 0;
    nw.BlockPen = 1;

#ifdef THEIRWAY
    nw.LeftEdge = screenBox.x;
    nw.TopEdge = screenBox.y + curMBarH ;
    nw.Width = screenBox.w;
    nw.Height = screenBox.h - curMBarH;
#else
    nw.LeftEdge = screenBox.x;
    nw.TopEdge = screenBox.y;
    nw.Width = screenBox.w;
    nw.Height = screenBox.h;
#endif

    nw.IDCMPFlags = RAWKEY |MOUSEBUTTONS |MENUPICK |
	MENUVERIFY | REFRESHWINDOW| ACTIVEWINDOW | INACTIVEWINDOW;
    nw.Flags = ACTIVATE |BACKDROP |BORDERLESS |REPORTMOUSE|
	/* NOCAREREFRESH |*/  SIMPLE_REFRESH | RMBTRAP; 
    nw.Type = CUSTOMSCREEN;	/* type of screen in which to open */	
    mainW = OpenWindow(&nw);	/* open a window */

    if (mainW == NULL) BailOut();

    /* fix so Dos message requestors come up in my window */
    myProcess->pr_WindowPtr = (APTR)mainW; 
    
    mainRP = mainW->RPort;
    vport = &mainW->WScreen->ViewPort;
    
    nColors = 1<<depth;

    /* initialize hidden raster */
    if (clientBitMap==NULL) NewSizeBitMap(&hidbm,depth,screenBox.w,screenBox.h); 
    AllocTmpRas();
    mainRP->TmpRas = &tmpRas;	/* temp raster for area filling*/
    InitPicPath();
    DefaultPalette();
    }


