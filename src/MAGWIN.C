/*----------------------------------------------------------------------*/
/*        magwin.c -- implements a magnify window object		*/
/*----------------------------------------------------------------------*/
#include <system.h>
#include <prism.h>

extern Pane mainP;
extern SHORT curpen,curxpc;
extern BMOB *curob;
extern struct BitMap hidbm;
extern Box bigBox;
extern SHORT xShft,yShft;
extern SHORT xcen,ycen;
extern void PWritePix();
extern Box screenBox;
extern struct RastPort *mainRP,*curRP,tempRP;
extern Box clipBox,bigBox;
extern SHORT  curIMFlags;
extern SHORT usePen;

#define local static;

SHORT xorg=0, yorg=0;
/* This Boolean tells whether we are actually painting or just showing 
feedback while the user adjusts the graphic */
BOOL Painting =NO;


SHORT curWin = MAINWIN; /* = MAGWIN or MAINWIN */

short magN = 1;
    
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------
All coordinates passed to the routines mLine, etc are backing bitmap 
relative (bbmX, bbmY). To get view relative coordinates
 srcX = bbmX - srcPos.x;
 srcY = bbmY - srcPos.y;

To get screen coordinates:
  screenX = bbmX + xorg;
  screenY = bbmY + yorg;

Where
    xorg = srcBox.x - srcPos.x;
    yorg = srcBox.y - srcPos.y;

The backing bitmap is not necessary: it is used for implementing
scrolling and undo in the main painting window.  In the case of using 
the magnifier to edit a tile pattern, the updtBackBM() call could 
display an array of NxM tiles.
----------------------------------------------------------------------*/

#ifdef floofloo
/*----------------------------------------------------------------------*/
/*   magnify Context							*/
/*----------------------------------------------------------------------*/
typedef struct {
SHORT magN;		/* magnification factor			*/
struct RastPort *srcRP;	/* The source RasterPort 		*/				
Box  *srcBox; 	 	/* The clip box of source in its Raster */   	
Point srcPos;		/* The position of the view box relative*/
			/* to the backing bitmap 		*/
struct BitMap *magBM;	/* The magnify BitMap	 		*/
Box *magBox; 		/* Clip Box of magnify view in magBM.	*/
Point magPos;		/* The position of magnify view rel	*/
			/* to backing bitmap 			*/
void (*updtProc)();   /* Procedure to call to update the 	*/
			/* backing bitmap. (takes *Box as 	*/
			/* parameter, BBM relative);		*/
} MagContext;
/*----------------------------------------------------------------------*/
#endif 

BOOL magShowing = NO;
MagContext *mgc = NULL;

MagErr(s) char *s; { KPrintF(" Null magcontext  in %s \n",s); }

UpdtCach() {
    if (mgc == NULL) MagErr("UpdtCache");
    FixUsePen();
    xorg = mgc->srcBox->x - mgc->srcPos.x;
    yorg = mgc->srcBox->y - mgc->srcPos.y;
    PopGrDest();
    PushGrDest(mgc->srcRP,mgc->srcBox);
    }

    
UpdtOFF() { Painting = NO;  UpdtCach(); }
UpdtON() { Painting = YES;  UpdtCach(); }

static void noop(){}
ProcHandle curUpdt = &noop;

/*----------------------------------------------------------------------*/
/* Plug in a new Magnify context					*/
/*----------------------------------------------------------------------*/
void SetMagContext(magc) MagContext *magc; {
    mgc  = magc;
    magN = mgc->magN;
    curUpdt = magc->updtProc;
    if (curUpdt==NULL) curUpdt = &noop;
    UpdtOFF();
    }

SetMag(nmag) SHORT nmag; {
    mgc->magN = magN = nmag;
    }
    
MagState(onoff) BOOL onoff; { 
    if (mgc == NULL) MagErr("Magstate");
    magShowing = onoff; 
    }
    
UnMag(z) SHORT z; { return (( z + magN - 1)/magN); }
    
/*----------- Box "b" is in Backing Bitmap coords ---------------------*/

void MagUpdate(b) Box *b; {
    Box sbox; 
    SHORT dx,dy,pix;
    if (!magShowing) return;
    sbox.x = b->x + xorg;	sbox.y = b->y + yorg;
    sbox.w = b->w;	    	sbox.h = b->h;
    dx = (b->x - mgc->magPos.x) * magN + mgc->magBox->x;
    dy = (b->y - mgc->magPos.y) * magN + mgc->magBox->y;
	
/* this does give a fair speedup, and costs about 240 Bytes */

    if ((sbox.w == 1) && (sbox.h==1)) { /* special speedup for dots */
	pix = ReadPixel(mgc->srcRP, sbox.x,sbox.y);
	tempRP.BitMap = mgc->magBM;
	SetAPen(&tempRP,pix);
	PushGrDest(&tempRP, mgc->magBox);
	PFillBox(MakeBox(&sbox,dx,dy,magN,magN));
	PopGrDest();
	return;
	}
	
    MagBits(
	mgc->srcRP->BitMap,	/* source bitmap */
	&sbox,		/* box on screen in unMagnified source window*/
	mgc->magBM,	/* bitmap containing mag window 	*/
	dx,		/* position of the magnified version of */
	dy,		/* sbox in mag bitmap  */
	magN,magN,	/* amount of magnification */
	mgc->magBox	/* clipping box of the Mag window */
	);
    }

/*------- b is in Physical coords, in the screen bitmap----*/
UpdtSibs(b) Box *b; {
    Box updtBox;
    updtBox = *b;
    if ((usePen!=0)&& ((curIMFlags&NOBR)==0)) { 
	updtBox.x -= curob->xoffs;
	updtBox.y -= curob->yoffs;
	updtBox.w += curob->pict.box.w-1;
	updtBox.h += curob->pict.box.h-1;
	}
    if (magShowing) MagUpdate(&updtBox);
    (*curUpdt)(&updtBox);
    }

/* clip a point to the CurrentBBM box */
SHORT limit(a,l,u,d) SHORT a,l,u,d; {  if (a<l) a+=d; if (a>u) a-=d; 
	return(a); }

/* not yet implemented
MWClipPt(x,y,dx,dy) SHORT *x,*y,dx,dy; { 
    Box *bx = &screenBox;
    *x = limit(*x,0,bx->w-1,dx);
    *y = limit(*y,0,bx->h-1,dy);
    }
*/

SetCurWin(pn) Pane *pn; { curWin = pn->client;   }

/* convert window rel coords to bbm-rel magnified coords */        
MagCoords(x,y) SHORT *x,*y; { 
    if (curWin == MAGWIN) {
	*x = mgc->magPos.x + (*x)/magN;
	*y = mgc->magPos.y + (*y)/magN;
	}
    else { *x += mgc->srcPos.x; *y += mgc->srcPos.y; }
    }

#define BIG 4000
magRect(b) Box *b; {
    Box c,d;
    c.x = (b->x - mgc->magPos.x) * magN + mgc->magBox->x;
    c.y = (b->y - mgc->magPos.y) * magN + mgc->magBox->y;
    c.w = magN * b->w;
    c.h = magN * b->h;
    BoxAnd(&d,mgc->magBox,&c);
    c = clipBox;
    SetClipBox(&bigBox);
    PFillBox(&d);
    SetClipBox(&c);
    }
    
minvXHair(x,y) SHORT x,y; {
    Box bx;
    WaitTOF();
    if (curWin == MAINWIN) PInvertXHair(x + xorg, y + yorg);
    else {
	TempXOR();
	magRect(MakeBox(&bx,0,y,screenBox.w,1));
	magRect(MakeBox(&bx,x,0,1,screenBox.h));
	RestoreMode();
	}
    }
	

