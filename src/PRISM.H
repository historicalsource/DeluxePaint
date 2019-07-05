/*----------------------------------------------------------------------*/
/*  									*/
/*     prism.h								*/
/*  									*/
/*----------------------------------------------------------------------*/

/* this affects <prism,pgraph,mainmag,.. > */
/*#define THEIRWAY*/

#define FIXALIGN static short d1; static short d2=2; static short d3 = 0;

#define F1Key 0x3b
#define F2Key 0x3c
#define F3Key 0x3d
#define F4Key 0x3e
#define F5Key 0x3f
#define F6Key 0x40
#define F7Key 0x41
#define F8Key 0x42
#define F9Key 0x43
#define F10Key 0x44
#define HELPKEY 0x01
#define CLeft  0x4b
#define CRight 0x4d
#define CUp  0x50
#define CDown 0x48


#define	ESC	0x1b
#define CR	0xd
#define BS	8
#define TAB	9
#define NoChar 0xff
#define YES 1
#define NO  0
#define POSITIVE 1
#define NEGATIVE 0
#define FAIL 1
#define SUCCESS 0

/*----------------------------------------------------------------------*/
/*   	Paint Modes							*/
/*----------------------------------------------------------------------*/
#define NPaintModes 8    
typedef SHORT PaintMode;

/* changing this enumerated type requires changes to arrays
  in menu,pgraph,dispnum, bmob  */

#define Mask 		0
#define Color		1
#define Replace 	2
#define	Smear		3
#define Shade		4
#define	Blend		5
#define	CyclePaint	6	/* same as Color, but paint colors cycle*/
#define Xor		7


/*----------------------------------------------------------------------*/
/*   	Blitter Address							*/
/*----------------------------------------------------------------------*/
#define  BLTADDR   0xDFF040

/*----------------------------------------------------------------------*/
/*   	Blitter Op Codes (for BltBitMap and BltRaster)			*/
/*----------------------------------------------------------------------*/
#define REPOP 0xCC	/* B	*/
#define NOTOP 0x33	/* !B  */
#define XOROP 0x66	/* B xor C */
#define XORMASK 0x6A	
#define OROP  0xEE	/* B+C   */
#define NOTOROP 0xBB  	/* (!B)+C */
#define ANDOP 0x88	/* BC   */
#define NOTANDOP 0x22	/* (!B)C   */
#define COOKIEOP  0xca  /* AB + (!A)C */
#define BMINUSC  0x44
#define CMINUSB  0x22

#define PLUSOP CMINUSB
#define MINUSOP ANDOP

/*--- blitter size word ----- */
#define BlitSize(bpr,h) ((h<<6)|(bpr>>1)) 


/* CONSTANTS for incrementing Color table values */
#define REDC  0x100
#define GREENC  0x10
#define BLUEC  0x1


#define BITNOT(v)	(v^0xffff)
#define SWAP(a,b)  	a ^= b;  b ^= a;  a ^= b;
#define ORDER(a,b)	if ((a)>(b)) {SWAP(a,b); }
#define MAX(a,b)   	((a)>(b)?(a):(b))
#define MIN(a,b)   	((a)<(b)?(a):(b))
#define ABS(a)		((a)>0?(a):-(a))
#define SIGNUM(a)	((a)>0)

#define WordsNeeded(w)  (((w)+15)>>4)
#define BytesNeeded(w) ( (((w)+15)>>3)&0xfffe) 
#define PlaneSize(w,h) (BytesNeeded(w)*(h))
#define BMPlaneSize(bm) ((bm)->BytesPerRow*(bm)->Rows)

/*----------------------------------------------------------------------*/
/*   		Virtual Coordinate System 				*/
/*----------------------------------------------------------------------*/
/* convert "virtual coords" to "physical device coords" */
#define PMapX(x)	( (x) >> xShft )
#define PMapY(y)	( (y) >> yShft )

/* convert "physical device coords" to "virtual coords"  */
#define VMapX(x)	( (x) << xShft )
#define VMapY(y)	( (y) << yShft )

/*----------------------------------------------------------------------*/
/*  Graphics Data Structures						*/
/*----------------------------------------------------------------------*/
typedef LONG LongFrac;
typedef struct { SHORT x,y; } Point;
typedef struct { SHORT w,h; } Dims;
typedef struct { SHORT x,y,w,h; } Box;

#ifndef GRAPHICS_GFX_H
#include <graphics\gfx.h>
#endif

/*----------------------------------------------------------------------*/
/*  FIll patterns							*/
/*----------------------------------------------------------------------*/
#define PATHIGH 8
#define PATWIDE 8
#define PATMOD 7

/*----------------------------------------------------------------------*/
/*  BoxBM -- Box + Bitmap						*/
/*----------------------------------------------------------------------*/
typedef struct { Box box; struct BitMap *bm; } BoxBM;

#define INITBOX {0,0,0,0}
#define INITBoxBM { INITBOX, NULL}

/*----------------------------------------------------------------------*/
/*  BMOB -- Masked Bitmap Object					*/
/*----------------------------------------------------------------------*/
#define SHOWING 1
#define TOOBIGTOPAINT 2
typedef struct {
    BoxBM pict,save;
    UBYTE *mask;
    SHORT xoffs,yoffs;
    UBYTE flags;
    UBYTE xpcolor;
    UBYTE minTerm;
    UBYTE planeUse,planeDef;
    } BMOB;

#define INITBMOB { INITBoxBM, INITBoxBM, NULL, 0, 0, NO, 0, COOKIEOP, 0xff, 0 }

/*----------------------------------------------------------------------*/
/*  Pane -- A Sub-window  Object					*/
/*----------------------------------------------------------------------*/
/* flag values */
#define ALWAYS 1
#define HANGON 2
#define BORDER 4

typedef void (*ProcHandle)();

typedef struct Pane0 {
    USHORT flags;
    Box box;
    ProcHandle charProc,mouseProc,paintProc;
    ULONG client;
    struct Pane0 *next;
    } Pane;
    
/* mouse events: you can choose to be called when the program loops 
   even though the mouse didnt move */

typedef enum { NONE,LOOPED,MOVED,BDOWN,BUP,ENTER,LEAVE } MouseEvent;

/* ----- Activities -------*/
#define	nullAct		0
#define	shadeAct	1
#define drawAct		2
#define vectAct		3
#define curvAct		4
#define fillAct		5
#define airbAct		6
#define rectAct		7
#define frectAct	8
#define circAct		9
#define fcircAct	10
#define ovalAct		11
#define fovalAct	12
#define polyAct		13
#define fpolyAct	14
#define selbAct		15
#define textAct		16
#define gridAct		17
#define symAct		18
#define magAct		19
#define zoomAct		20
#define undoAct		21
#define clrAct		22


typedef SHORT CursID;
#define	Arrow	0
#define	FillCan	1
#define Pencil  2


/* Pen TYPE ENcoding */
#define USERBRUSH -1
#define ROUND_B		1
#define SQUARE_B	2
#define DOT_B		3
#define AIR_B		4    
#define RoundB(n)	((ROUND_B<<12)|n)
#define SquareB(n)	((SQUARE_B<<12)|n)
#define DotB(n)		((DOT_B<<12)|n)

/*----------------------------------------------------------------------*/
/*   Magnify Context							*/
/*----------------------------------------------------------------------*/
typedef struct {
    SHORT magN;		/* Magnification factor			*/
    struct RastPort *srcRP;	/* The source RasterPort 		*/				
    Box  *srcBox; 	 /* The clip box of source in its Raster */   	
    Point srcPos;	/* The position of the view box relative*/
			/* to the backing bitmap 		*/
    struct BitMap *magBM;	/* The magnify BitMap	 		*/
    Box *magBox; 	/* Clip Box of magnify view in magBM.	*/
    Point magPos;	/* The position of magnify view rel	*/
			/* to backing bitmap 			*/
    void (*updtProc)();   /* Procedure to call to update the 	*/
			/* backing bitmap. (takes *Box as 	*/
			/* parameter, BBM relative);		*/
    } MagContext;


/* kinds of window */
#define MAINWIN 1
#define MAGWIN 2

/* Brush Transform states */
#define NOXFORM 0
#define STRETCHED 1
#define ROTATED 2
#define SHEARED 3
#define BENT 4

/*** CURSOR TYPES *********/
#define NOCURSOR  0
#define DEFCURSOR 1
#define FILLCURSOR 2
#define CROSSCURSOR 3
#define ZZZCURSOR 4
#define SIZECURSOR 5
#define PICKCURSOR 6

/*----------------------------------------------------------------------*/
/*   -Interaction Modes (IMODES)					*/
/*----------------------------------------------------------------------*/
#define IM_none		0xff
#define	IM_null		0
#define	IM_shade	1
#define IM_draw		2
#define IM_vect		3
#define IM_curve1	4
#define IM_curve2	5
#define IM_rect		6
#define IM_frect	7
#define IM_circ		8
#define IM_fcirc	9
#define IM_oval		10
#define IM_foval	11
#define IM_selBrush	12
#define IM_magSpec	13
#define IM_fill		14
#define IM_text1	15
#define IM_text2	16
#define IM_gridSpec     17
#define IM_symCent	18
#define IM_readPix	19
#define IM_strBrush	20
#define IM_rotBrush	21
#define IM_shrBrush	22
#define IM_sizePen	23
#define IM_poly		24
#define IM_poly2	25
#define IM_fpoly	26
#define IM_fpoly2	27
#define IM_airBrush	28
#define IM_getFPat	29
#define IM_hbendBrush	30
#define IM_vbendBrush	31

#define NIMODES	32

typedef UBYTE IMode;

typedef struct {
    UWORD flags;	/* see below */
    UBYTE cursor;	/* what cursor to use */
    UBYTE activity;	/* ctr panel activity for this */
    UBYTE symNpoints;	/* number of points to be transformed*/
    IMode nextIMode;	/* for chaining */
    void (*startProc)(); /* call this to plug mode procs*/
    } IModeDesc;


/* Flag Definitions */
#define	PERM	1	/* for permanent modes */
#define	NOGR 	(1<<1)	/* dont do grid for this mode */
#define NOBR 	(1<<2)	/* this mode doesn't use the brush (e.g. frect )
		this is used in determining how big of a rectangle has
		been touched on the screen*/
#define NOSYM	(1<<3)	/* dont do symmetry at all */
#define	SYMUP	(1<<4)	/* show symmetry feedback when button is up*/
#define SYMDN	(1<<5)	/* show symmetry feedback when button is down*/
#define SLAVE	(1<<6)	/* not a "master" mode */
#define NOLOCK	(1<<7)	/* Cant lock by double clicking */
#define EVTIM	(1<<8)	/* Call every time while button down*/
#define PNTWDN 	(1<<9)	/* Turn on paint (UpdtON) while button is down*/
#define COMPLETE (1<<10)	/* Call completeMode when space bar hit */
#define RIGHTBUT (1<<11)	/* Set later if mode invoked w/right button*/
#define NOERASE (1<<12)	       /* Dont erase with right button*/
#define NOCON 	(1<<13)	       /* Dont constrain with shift key*/
#define SYMALL	SYMUP|SYMDN
#define DEFC DEFCURSOR


/*----------------------------------------------------------------------*/
/*   Color Cycles							*/
/*----------------------------------------------------------------------*/
#define MAXNCOLORS 32
#define MAXNCYCS 4
typedef struct {
    SHORT count;
    SHORT rate;
    BOOL active;
    UBYTE low,high;	/* bounds of range */
    } Range;


/*----------------------------------------------------------------------*/
/*   Air Brush								*/
/*----------------------------------------------------------------------*/

#define INITABRADIUS 12

extern BOOL haveWBench;

#define dprintf 	if (haveWBench) printf


/*----------------------------------------------------------------------*/
/*  Palette interface							*/
/*----------------------------------------------------------------------*/

/* A typical client.*/
/* Usage: PaletteTool(struct Window *, PaletteGlobals *);          	*/

typedef struct {
    WORD paletteX, paletteY;
	/* LeftEdge,TopEdge for palette window.
    	 * Client can change these to force window to appear elsewhere.
     	* If user drags window, these are updated before return to client.*/
    WORD paletteRange;		/* Index of range currently examined.*/
    Range *ranges;		/* LIM_RANGES Range's */
    } PaletteGlobals;


/*----------------------------------------------------------------------*/
/*  Blitter registers							*/
/*----------------------------------------------------------------------*/

typedef struct {
    USHORT	bltcon0,bltcon1,fwmask,lwmask;
    UBYTE	*bltptc,*bltptb,*bltpta,*bltptd;
    USHORT	bltsize;
    SHORT	ioskip2[3];
    SHORT	bltmdc,bltmdb,bltmda,bltmdd;
    SHORT	ioskip3[4];
    USHORT	cdata,bdata,adata;
    } BlitterRegs;

/*----------------------------------------------------------------------*/
/*  DID types ( for UNDO )						*/
/*----------------------------------------------------------------------*/

#define DIDNothing 0
#define DIDClear 1
#define DIDMerge 2
#define DIDHPoly 3


/*----------------------------------------------------------------------*/
/*  OVSInfo : for configuring the overlay system			*/
/*----------------------------------------------------------------------*/
#define OVS_DUMB 0
#define OVS_SMART 1
#define OVS_LOAD_ALL 2

typedef struct {
    LONG type;
    void (*sleepCursor)();
    void (*wakeCursor)();
    void (*panic)();
    } OVSInfo;
    