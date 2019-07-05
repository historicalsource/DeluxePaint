/*----------------------------------------------------------------------*/
/*									*/
/*		prism.c -- Main program					*/
/*									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <librarie\dosexten.h>
#include <prism.h>
#include <librarie\diskfont.h>
#include <dphook.h>

#define local static

extern void mainRefresh();
extern struct Menu *MainMenu; 
extern BMOB curpenob,curbr;
extern void mainCproc(),mainPproc(),mainMproc(),DrawMode(),ShadMode();
extern Range cycles[];
extern void ZZZCursor(), UnZZZCursor(), Panic();

/* free up system memory which is in Limbo */
#define MEGA (1<<20)

FreeUpMem() {
    UWORD *ptr = (UWORD *)AllocMem( MEGA, MEMF_PUBLIC );
    if (ptr!=NULL) FreeMem(ptr, MEGA);
    }

/* ------- Procs called before and after overlay loading  */
BOOL refrAfterOverlay = YES;
BOOL loaded = NO;
OffRefrAfterOv() { refrAfterOverlay = NO; }
OnRefrAfterOv() { refrAfterOverlay = YES; }

void OvsPrepare() {
	if (loaded) {UndoSave(); ZZZCursor();} }
void OvsEnd() { 
	if (loaded) {
		if (refrAfterOverlay) {
	    	PaneRefrAll(); 
	    	}
		UnZZZCursor();
		}
	}

/*------------ Lots of Global Variables -----*/
    
OVSInfo ovsInfo = {OVS_LOAD_ALL, OvsPrepare, OvsEnd, Panic};

BOOL modeHelp = YES;
BOOL underWBench= NO,loadAFile = NO;
    
long GfxBase = 0;
long IntuitionBase=0;
long IconBase=0;

/* for "hook" */
struct BitMap *clientBitMap = NULL;
BOOL myDPHook;
DPHook *dphook = NULL;
UBYTE DPName[] = "DeluxePaint";

SharingBitMap() { return(!myDPHook ||(dphook->refCount>0)); }
HookActive() {return(myDPHook && (dphook->refCount>0));}

struct Window *mainW;
struct Screen *screen;
struct RastPort *mainRP;
struct ViewPort *vport;
BOOL haveWBench = YES;
Box mainBox;
Pane mainP = {0};
BOOL largeMemory = NO;		/* yes means 512 machine */
SHORT curFormat = 0; 		/* current screen format */
SHORT curDepth = 5;
SHORT curMBarH = 11;
SHORT xShft,yShft;

Box screenBox = {0};
struct BitMap hidbm = {0};
struct TmpRas tmpRas = {0};
struct RastPort tempRP = {0};
struct RastPort screenRP = {0};

UBYTE version[] = "(1.0)";
UBYTE dpaintTitle[] = "DPaint";

PaletteGlobals paletteGlobals = {0, 11,  1, cycles};

/* ----------- Palettes ------------------------------------- */
SHORT nColors;

SHORT default5[32]= {
    0,     0xfff, 0xe00, 0xa00, 0xd80, 0xfe0, 0x8f0, 0x080,
    0x0b6, 0x0dd, 0x0af, 0x07c, 0x00f, 0x70f, 0xc0e, 0xc08,
    0x620, 0xe52, 0xa52, 0xfca, 0x333, 0x444, 0x555, 0x666,
    0x777, 0x888, 0x999, 0xaaa, 0xbbb, 0xccc, 0xddd, 0xeee};	

#ifdef stevespalette
SHORT default5[32]= {
    0,     0xfff, 0xddd, 0xaaa, 0x888, 0x666, 0x444, 0x222,
    0xf88, 0xfb8, 0xff8, 0xbf8, 0x8f8, 0x8ff, 0x88f, 0xf8f,
    0xf00, 0xf80, 0xff0, 0x8f0, 0x0f0, 0x0af, 0x00f, 0xf0f,
    0xa00, 0x940, 0x880, 0x590, 0x060, 0x077, 0x007, 0x808};	
#endif
SHORT default4[16]= { 0, 0xfff, 0xc00, 0xf60, 0x090, 0x3f1,0x00f,0x2cd,
		0xf0c, 0xa0f, 0x950, 0xfca, 0xfe0, 0xccc,0x888,0x444};
SHORT default3[8] = { 0, 0xfff, 0xb00, 0x080, 0x24c, 0xeb0, 0xb52, 0x0cc};
SHORT default2[4] = {0,0xfff,0x55f,0xf80};
SHORT default1[2] = {0,0xfff};

#ifdef dansway
UBYTE initBGCol[]= {0,0,0,0,14,25}; /* initial background color [depth] */
UBYTE initFGCol[]= {1,1,1,1,0,0}; /* initial forground color [depth] */
#else /*grady's way */
UBYTE initBGCol[]= {0,0,0,0,0,0}; /* initial background color [depth] */
UBYTE initFGCol[]= {1,1,1,1,1,1}; /* initial forground color [depth] */
#endif

SHORT *defPals[] = {NULL,default1,default2,default3,default4,default5};
SHORT LoadBrColors[32]; /* colors loaded with the brush */
SHORT prevColors[32];	/* previous palette before loading picture
			or doing Use Brush Palette */

/* ---- Current Brush ---------------------------------------- */
SHORT curpen;
struct BitMap brBM={0},brSVBM={0};
struct BitMap penBM={0},penSVBM={0};
BMOB curpenob = { { INITBOX, &penBM}, {INITBOX,&penSVBM},
		NULL,0,0,NO,0, COOKIEOP, 0xff,0};
BMOB curbr = { { INITBOX, &brBM}, {INITBOX,&brSVBM},
		NULL,0,0,NO,0, COOKIEOP, 0xff,0};

/* ----- Text and font Info -------------------------------------*/

/* ----- Text data ---------- */
SHORT cx,cy,cwidth;
char textBuf[80];
SHORT textX, textY;
SHORT textX0,cw,ch,cyoffs;
SHORT ptext = 0;
SHORT svmd = 0;
BOOL tblinkOn = NO;
SHORT tbCount = 0;

#define TXHEIGHT 8
struct TextAttr TestFont =   {  "topaz.font",  TXHEIGHT, 0, 0, };
LONG DiskfontBase= NULL;
struct AvailFonts *fontDir = NULL;
SHORT nFonts = 0;
SHORT curFontNum = -1;
struct TextFont *font = NULL;

MyCloseFont(font) struct TextFont *font; {
    struct Node *nd;
    if (font!=NULL) {
	if ( (font->tf_Flags&(FPF_ROMFONT|FPF_DISKFONT)) == FPF_DISKFONT) {
	    nd = &font->tf_Message.mn_Node;
	    if ((nd->ln_Pred->ln_Succ == nd) &&
		(nd->ln_Succ->ln_Pred == nd)) RemFont(font);
	    }
	CloseFont(font); 
	}
    }

/*--- file io pathnames and filenames ---------- */
UBYTE picpath[31] = "";
UBYTE picname[31] = "";
UBYTE brspath[31] = "brush";
UBYTE brsname[31] = "";

/*---- Original brush and transform ( brxform )-----------*/
struct BitMap origBM = {0};
BMOB origBr = { { INITBOX, &origBM}, {INITBOX,NULL},
		NULL,0,0,NO,0, COOKIEOP, 0xff,0};
SHORT curBrXform = NOXFORM;

/* This gets rid of the "original" bitmap and sets the state to
  " no transform" */
ClearBrXform() { 
    FreeBitMap(&origBM); 
    DFree(origBr.mask); 
    origBr.mask = NULL; 
    curBrXform = NOXFORM;
    }

/* --- BMOB stuff ------ */
SHORT lastSmX, lastSmY;
SHORT dbgbmob = 0;
struct BitMap tmpbm = {0};
BoxBM temp = {{0,0,0,0},&tmpbm};
struct BitMap tmpbm2 = {0};
BoxBM temp2 = {{0,0,0,0},&tmpbm2};

/* --------------spare bitmap----------------*/
BOOL spareThere = NO;
BOOL newSpare = YES;
BOOL didMerge = NO;
SHORT spareXPCol = 0;
struct BitMap sparebm = {0};
FreeSpare() {
    if (spareThere) FreeBitMap(&sparebm); 
    spareThere = NO;
    }


/* --------------Filled polygon----------------*/
SHORT nPolPnts = 0;
Point *polPnts = NULL;
struct AreaInfo areaInfo;
WORD *areaBuff = NULL;

GunTheBrush() {  /* dump the current brush to free storage */
    RestoreBrush();
    FreeBMOB(&curbr);
    SelPen(0); 
    }
 
void Panic() {
    GunTheBrush();
    }

SetOverlay(tight) BOOL tight;  {
    ovsInfo.type = tight? OVS_DUMB: OVS_LOAD_ALL;
    InitOVS(&ovsInfo);
    }


/*---------- Copyright Notice ------------------*/
DoText(s,x,y) char *s; int x,y; {
	Move(&screenRP,x,y);
    Text(&screenRP,s,strlen(s));
	}

CopyWText(x,y)  {
	DoText("       -- Deluxe Paint --           ", x,y);
	DoText("        by Daniel Silva             ", x,y+30);
	DoText("   (c) 1986 by Electronic Arts", x,y+60);
 	}

#define CW_WIDTH  280
#define CW_HEIGHT 120
CopyWNotice() {
	int x,y;
	x = (screenBox.w-PMapX(48) - CW_WIDTH)/2;
	y = (screenBox.h - CW_HEIGHT)/2;
    SetOPen(&screenRP,1);
    SetAPen(&screenRP,3);
	RectFill(&screenRP, x, y, x+CW_WIDTH-1, y+CW_HEIGHT-1);
    BNDRYOFF(&screenRP);
    SetAPen(&screenRP,0);
    SetDrMd(&screenRP,JAM1);
	CopyWText(x+6, y + 30);
	SetAPen(&screenRP,1);
	CopyWText(x+5, y + 29);
 	}


/* -------------------------------------------------------- */
struct Process *myProcess;
struct Window *svWindowPtr;

/*----------------------------------------------------------------------*/
/* Yes, this is it, the MAIN PROGRAM:					*/
/*----------------------------------------------------------------------*/
main(argc,argv) int argc; char *argv[]; {
    DPInit(argc,argv);  /* initialization code */
    NewIMode(IM_draw);
    SetAirBRad(PMapX(24)); /* also brings in the drawing overlay*/
#ifdef DOWB
    if (loadAFile) LoadPic();
#endif
    CopyWNotice();
	loaded = YES;
	PListen(); 		/* this is the program */
    if (!haveWBench) BootIT();
    else CloseDisplay();
    }

/* Allocate and Free temp Raster --------------- */
AllocTmpRas() {
    int  plsize = mainRP->BitMap->BytesPerRow*mainRP->BitMap->Rows;
    DFree(tmpRas.RasPtr);
    tmpRas.RasPtr = (BYTE *)ChipAlloc(plsize);
    if (tmpRas.RasPtr==NULL) InfoMessage(" Couldnt alloc", "tmpRas. ");
    tmpRas.Size = plsize;
    mainRP->TmpRas = &tmpRas;	/* temp raster for area filling*/
    }

FreeTmpRas() {
    DFree(tmpRas.RasPtr);
    tmpRas.RasPtr = NULL;
    }

/** misc utilities that should probably be elsewhere ----- */

ConcatPath(path,ext) UBYTE *path,*ext; {
    int lp;
    lp = strlen(path);
    if ( (lp>0)&& (path[lp-1] != ':'))  strcat(path,"/");
    strcat(path,ext);
    }

/** Set raster port for xor-ing with current foreground
   color (instead of with 0xffff as COMPLEMENT mode does */
#define NOOP 0xaa
SetXorFgPen(rp) struct RastPort *rp; {
    SHORT pen = rp->FgPen;
    SHORT bit = 1;
    SHORT i;
    for (i=0; i<rp->BitMap->Depth; i++) {
    	rp->minterms[i] = (bit&pen) ? XORMASK : NOOP;
	bit <<=1;
	}
    }


/**  ------ Palette operations  ----- */

GetColors(cols) SHORT *cols; {
    int i;
    for (i = 0; i<nColors; i++) cols[i] = GetRGB4(vport->ColorMap,i);
    }

LoadCMap(cols) SHORT *cols; { LoadRGB4(vport,cols,nColors); }

BasicBandW() { SetRGB4(vport,0,0,0,0); SetRGB4(vport,1,15,15,15);}

InitPalette() { GetColors(prevColors); }
RestorePalette() { LoadCMap(prevColors);}
DefaultPalette() { InitPalette(); LoadCMap(defPals[curDepth]); } 

SetColReg(n,col) int n, col; {
    SetRGB4(vport, n, (col>>8)&15, (col>>4)&15, col&15);
    }

local SHORT svcol0,svcol1,svcol2,svcol3;    

/** this gets called before going into menu (at MenuVerify) */
FixMenCols() {
    SHORT colors[32];
    FreeTmpRas();/* make room for menu to save under area*/
    PauseCCyc();
    if (curDepth>2) {
	GetColors(colors);
	svcol0 = colors[0];
	svcol1 = colors[1];
	svcol2 = colors[nColors-2];
	svcol3 = colors[nColors-1];
	SetColReg(0,0);
	SetColReg(1,0xfff);
	SetColReg(nColors-2,0x8ac); /* 0 */
	SetColReg(nColors-1,0x900); /* 0xfff */
	}
    }

/** ---- This gets called after menu */
UnFixMenCols() {
    AllocTmpRas();
    if (curDepth>2) {
	SetColReg(0,svcol0);
	SetColReg(1,svcol1);
	SetColReg(nColors-2,svcol2);
	SetColReg(nColors-1,svcol3);
	}
    ResumeCCyc();
    }

local ULONG lastAvMem = NULL;

/**
PrAvail(s) char *s; {
    ULONG avmem = AvailMem(MEMF_PUBLIC|MEMF_CHIP);
    printf("Avail Mem %-27s = %ld (%7ld)\n",s,avmem, avmem - lastAvMem);
    lastAvMem = avmem;
    }
**/

