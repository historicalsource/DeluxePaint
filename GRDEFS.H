/*----------------------------------------------------------------------*/
/*									*/
/*			grdefs.h -- Graphics Definitions 		*/
/*									*/
/*----------------------------------------------------------------------*/

#define BIGNUM	32767
#define ALLONES	0xffff
#define NIL	0
#define MXLWRD	160  	/* number of words in line buffers		*/

typedef char Boolean;
#define	TRUE	0xff
#define FALSE	0
#define	YES	0xff
#define NO	0
#define local	static
#define FAIL	-1
#define SUCCESS 0

typedef char Byte;
typedef unsigned int Word;
typedef char *String;
typedef long longFrac;

/* Utility functions */
#define BitNot(v)	(v^0xffff)
#define swap(a,b)  	a ^= b;  b ^= a;  a ^= b;
#define order(a,b)	if ((a)>(b)) {swap(a,b); }
#define max(a,b)   	((a)>(b)?(a):(b))
#define min(a,b)   	((a)<(b)?(a):(b))
#define abs(a)		((a)>0?(a):-(a))
#define signum(a)	((a)>0)

/* ----------------Rasterop Modes ---------- */
#define REPMODE 0	/* REPLACE pixels with new values 		*/
#define XORMODE 1	/* XOR source into dest 			*/
#define ORMODE 	2	/* OR source into dest		 		*/
#define XPMODE 	3	/* current transparent color doesnt paint, 
			    other colors write		 		*/
#define ANDMODE	4	/* AND source into dest		 		*/
#define ERSMODE	5	/* ERASE: and complement of source into dest	*/
#define COLMODE	6	/* use source as mask through which current 
			    color is painted into dest 			*/


#define BLACK	0
#define WHITE	0xffff
#define BLUE	0x5555
#define MAGENTA 0xaaaa
#define BGROUND 0xc3c3

/* ------ offset in bytes of word containing pixel #x */

#define xword(x)	(((x)>>LGPPB)&0xfffe)

/*  ------ offset in bytes of byte containing pixel #x */

#define xbyte(x)	(x>>LGPPB)

/* ------ words required for w pixels */

#define wrdsrqd(w)	((w+PPWM1)>>LGPPW)

/*  ------ set up current color word for scan line y */

#define colForY(y) 	colw = curpat[y&3] 



/*--------------------------------------------------------------*/
/*								*/
/*		Graphics Data Structures			*/
/*								*/
/*--------------------------------------------------------------*/

typedef union {
    struct { char lo,hi; } b;
    int w;
    } Wrdovl;

typedef struct{ int x,y; } Point;
	
typedef struct{ int w,h; } Dims;
	
typedef struct{ int x,y,w,h; } Box;
	
typedef struct{
	Word segment;	/* segment addr of bitmap		*/
	Word *base;	/* addr of start of data 		*/
	Word width;	/* width in words 			*/
	Box box;	/* image bounds 			*/
	Word *ytab; 	/* pointer to table  of y offsets 	*/
	Byte xpcolor;	/* transparent	color			*/
	Byte mode;	/* paint mode				*/
	} Bitmap;

#define INITBM	{NIL,NIL,0,{0,0,0,0},NIL,0,XPMODE}

/* General Displayable Object  DOB			*/

typedef struct {
    int (*display)();	/* pointer to display proc 	*/
    int x,y;		/* present coords of upper left */
    int xoffs,yoffs;	/* offset of grab point 	*/
    Boolean showing;	/* is it displayed?		*/
    Bitmap svbm;	/* save bitmap			*/
    } DOB;

/* Bitmap Object: if mbm is defined (segment != NIL)  */
typedef struct {
    DOB hdr;		/* header inherited from DOB	*/
    Bitmap sbm;		/* source bitmap 		*/
    Bitmap mbm;		/* mask bitmap(optional)	*/
    } BMObj;

#define INITDOB   {NIL,0,0,0,0,FALSE,INITBM}
#define INITPSB   {INITBM,INITBM,INITBM,INITBM}
#define INITBMOB  {INITDOB,INITBM,INITBM}

typedef struct{
    int curcol;
    int curxpc;
    Box box;
    int curmode;
    Bitmap *curbm;
    } GrState;

/* window: definitions */
typedef struct {
    unsigned border : 1; /* draw border */
    unsigned always : 1; /* call even if mouse didnt move */
    unsigned hangOn : 1; /* keep control while button down */
    } WFlags;
    
struct Winrec{
    Word client;	/* client context */
    Box box;		/* rectangle on screen */
    WFlags flags;	/* booleans */
    Boolean border;	/* does it have a border */
    int (*charProc)();  /* called with all typing      */
    int (*mouseProc)();	/* called on all mouse actions */
    int (*paintProc)();	/* called to repaint window */
    struct Winrec *next;
    };

typedef struct Winrec Window;

typedef Byte MouseEvent;

/* mouse events: you can choose to be called when the program loops 
    even though the mouse didnt move */

#define NONE	0	/* nothing happened (i dont call you) */    
#define LOOPED	1	/* you wanted me to call you every time*/
#define	MOVED	2	/* the mouse moved while in this window */
#define	BDOWN	3	/* button went down */
#define	BUP	4	/* button went up */
#define ENTER	5	/* entered window */
#define LEAVE	6	/* left window */

/* Magnified Bitmap window   (subclassed off of Window)		 */

struct MagWinGr {
    Bitmap *bbm;
    struct MagWinRec *first;
    };

struct MagWinRec{
    Window win;
    Point wb;			/* coords of window rel to bitmap */
    int	mag;			/* magnification */
    struct MagWinGr *group;	/* pointer to group sharing backing bitmap */
    struct MagWinRec *sib;	/* sister in group */
    };
    
typedef struct MagWinGr MWGroup;
typedef struct MagWinRec MagWindow;
        
typedef struct {
    Bitmap *bm;
    Box box;
    int xorg,yorg;
    } GrCntxt;

/* ----- Activities -------*/
#define	nullAct		0
#define	shadeAct	1
#define drawAct		2
#define vectAct		3
#define curvAct		4
#define rectAct		5
#define frectAct	6
#define circAct		7
#define fcircAct	8
#define ovalAct		9
#define fovalAct	10
#define selbAct		11
#define magAct		12
#define fillAct		13
#define textAct		14
#define gridAct		15
#define symAct		16
#define ioAct		17
#define clearAct	18

#define NPENS 	9
#define USERPEN	-1


/*--------------------------------------------------------------*/
/*								*/
/*		Menu Box 			 		*/
/*								*/
/*--------------------------------------------------------------*/

struct MenuRec {
    Box		box;	/*window - relative */
    int		type;	/* what kind of item */
    struct MenuRec  *next;
    };  

typedef struct MenuRec MenuItem, *MIPtr;

typedef struct {
    Window	win;
    Boolean	showing; /* is it on the screen? */
    Boolean	isTemp;
    MenuItem	*curmi;
    MenuItem	*items;
    int		effNWIDE; /* set to 320 initially */
    } MBox;

/*------ Passive Types----- */

#define	LablTyp		0
typedef struct	{
    MenuItem	hdr;
    char 	*name;
    } Labl;

#define	ColBoxTyp	1	
typedef struct	{
    MenuItem	hdr;
    Word	fillPat;
    } ColBx;
	    
/* -------  Active Types----- */

#define	ButnTyp		2
typedef struct	{
    MenuItem	hdr;
    char 	*name;
    Boolean	islit;
    int		(*proc)();
    } Butn;
	    
#define	TBoxTyp		3
typedef struct	{
    MenuItem	hdr;
    char 	*string;
    int		nmax;
    int		(*proc)();
    } TBox;

#define MarTyp		4
typedef struct	{
    MenuItem	hdr;
    char 	**names;
    Boolean	dolines;
    Boolean	vertical;
    int		spacing;	/* used only by horizontal */
    int		(*proc)();
    int		curitem,lit,nitems; /* used by menu processr */
    } Mar;
        	    
#define	ScBarTyp	5
typedef struct	{
    MenuItem	hdr;
    int		(*proc)();
    } ScBar;

#define	NBoxTyp		6
typedef struct	{
    MenuItem	hdr;
    int 	pval;		/*  value to be displayed*/
    int		nmax;		/*  max num of chars */
    int		(*proc)();
    } NBox;

#define MAXCHRS		9
#define NEWITEM		10
#define NullItem	-1


#define USERBRUSH	255

/* configuration */
#define NCONFIG 6

#define MED2	0
#define ART4	1
#define LOW4	2
#define	HRBW  	3
#define TEC1	4	/* 320x200x4 */
#define TEC2	5	/* 640x200x4 */

typedef int CursID;
#define	Arrow	0
#define	FillCan	1
#define Pencil  2

/* input device types */
#define NoDev		0
#define Joystick	1
#define	Koala		2
#define	MMouse		3

/* input device vector*/

extern int (*rdInDev)();
#define rdmouse(xaddr,yaddr) 	((*rdInDev)(xaddr,yaddr))

/* Hardware Type*/
#define	PCSR	0	/*  IBM PC  */
#define	PCJR	1	/*  IBM PC/jr */
#define PCXT	2	/*  IBM PC/XT */
#define IBMAT	3	/*  IBM AT */
#define TA1000	4	/*  Tandy 1000 */

