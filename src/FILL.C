/*--------------------------------------------------------------*/
/* 			Basic Seed Fill				*/
/*								*/
/*     FillArea: fills all pixels which are the same color as	*/
/*	and 4-connected to the seed pixel			*/
/*--------------------------------------------------------------*/

#include <system.h>
#include <prism.h>

#define STSIZE	160
#define UP	1
#define DN	-1

#define local static

extern SHORT ReadPixel();

extern struct TmpRas tmpRas;
extern struct RastPort *curRP, tempRP;
extern Box clipBox;
extern UWORD masks[];
extern BOOL abortFlag;

local SHORT fxmax,fymax,fxmin,fymin;
local SHORT intoCol;


OvflMsg() { KPrintF(" FILL Overflow"); }

/** --------- Stack ----------------- */

typedef struct{SHORT dir,xl,xr,y;} Span;
local Span *stack;


/* ----------Queue----------------- */
SHORT oldest, nextVacant;
local initStack(ps) Span *ps; { stack = ps; oldest = nextVacant = 0; }    

#define StackNotEmpty  (oldest != nextVacant)

local popSpan(d,xll,xrr,yy) SHORT *d,*xll,*xrr,*yy; {
    Span *spn = &(stack[oldest]);
    *d = spn->dir;
    *xll = spn->xl;
    *xrr = spn->xr;
    *yy = spn->y;
    oldest = (oldest+1)%STSIZE;
    }

local pushSpan(d,xll,xrr,yy) SHORT d,xll,xrr,yy; {
    Span *spn = &(stack[nextVacant]);
    spn->dir = d;
    spn->xl = xll;
    spn->xr = xrr;
    spn->y =yy;
    nextVacant = (nextVacant+1)%STSIZE;
    if (nextVacant == oldest) {
	if (--nextVacant<0) nextVacant = STSIZE-1;
	OvflMsg();
	}
    }

#ifdef slowButCorrect

/* ----- "Inside" test for area fill  */
#define Inside(x,y) (ReadPixel(&tempRP,x,y)==intoCol)

local SHORT  firstRight(x,y) SHORT x,y; {
    while( (x<=fxmax)&&(!Inside(x,y))) x++ ;  return(x); }
local SHORT firstLeft(x,y) SHORT x,y; {
    while( (x>=fxmin)&&(!Inside(x,y))) x-- ;  return(x);  }
local SHORT lastRight(x,y) SHORT x,y; {
    while( (x<=fxmax)&&Inside(x,y)) x++ ; return((SHORT)(x-1)); }
local SHORT lastLeft(x,y) SHORT x,y; {
    while( (x>=fxmin)&&Inside(x,y)) x-- ;  return((SHORT)(x+1)); }

#else /* fast and complicated */

local UWORD *WrdAddr(x,y) SHORT x,y; {
    return((UWORD *)( ((ULONG)(tempRP.BitMap->Planes[0])) +
	    (ULONG)(y*tempRP.BitMap->BytesPerRow) +((x>>3)&0xfffe)));
    }

/* Search for the first bit not matching 'pat'  to the right */
local SHORT searchRight(x,y,pat) register SHORT x; SHORT y; register UWORD pat; {
    register UWORD *wp,wrd;
    wp = WrdAddr(x,y);
    wrd = (pat^(*wp++))&masks[x&15]; /* turns all pat bits to 0's */
    x = (x+16)&0xfff0;
    while ( (wrd == 0) && (x <= fxmax) ) {wrd = pat^(*wp++); x += 16; }
    x -= 16;
    if (wrd) while ( (wrd & 0x8000) == 0 ) { wrd <<= 1; x++; }
    else return((SHORT)(fxmax+1));
    return((SHORT)MIN(x,fxmax+1));
    }

/* Search for the first bit not matching 'pat'  to the left */
local SHORT searchLeft(x,y,pat) register SHORT x; SHORT y; register UWORD pat; {
    register UWORD *wp,wrd;
    wp = WrdAddr(x,y);
    wrd = (pat^(*wp--))&(~masks[(x&15)+1]);
    x = (x&0xfff0) - 1;
    while ( (wrd == 0) && (x >= fxmin) ) { wrd = pat^(*wp--); x -= 16; }
    x += 16;
    if (wrd) while ( (wrd & 1) == 0 ) { wrd >>= 1; x--; }
    else return((SHORT)(fxmin-1));
    return((SHORT)MAX(x,fxmin-1));
    }

/* --- find the first '0' i.e. "inside" bit to left (right) */
#define firstLeft(x,y)  searchLeft(x,y,0xffff)
#define firstRight(x,y)  searchRight(x,y,0xffff)

/* --- find the last contiguous '0' i.e. "inside" bit to left (right) */
#define lastLeft(x,y)  (searchLeft(x,y,0) + 1)
#define lastRight(x,y)  (searchRight(x,y,0) - 1)
    
#endif

local fillStrip(xl,y,xr) SHORT xl,y,xr; {
    PatHLine(xl,y,xr-xl+1);
    Move(&tempRP,xl,y); Draw(&tempRP,xr,y);
    }

void FillArea(sx,sy) 
    SHORT sx,sy; 		/* seed point */
    {
    SHORT y,xr,xl,yn,xml,xmr,xtl,xtr,dir;    
    struct BitMap tempBM,*curbm;
    Span dstack[STSIZE];
    
    intoCol = ReadPixel(curRP,sx,sy);
    if (intoCol==curRP->FgPen) return;
    fxmin = clipBox.x;
    fymin = clipBox.y;
    fxmax = clipBox.x + clipBox.w -1;
    fymax = clipBox.y + clipBox.h -1;
    if ( sx<fxmin || sx>fxmax || sy<fymin || sy>fymax ) return;
    
/* make a mask in the TmpRas that has 1's where the target bitmap is NOT
   the color of the area being filled */
    curbm = curRP->BitMap;
    MakeMask(curbm, tmpRas.RasPtr, intoCol, NEGATIVE);
    movmem(curbm,&tempBM,sizeof(struct BitMap));
    tempBM.Depth = 1;
    tempBM.Planes[0] = tmpRas.RasPtr;
    tempRP.BitMap = &tempBM;
    intoCol = 0;
    SetAPen(&tempRP,1);
    
    initStack(&dstack);
    xl = lastLeft(sx,sy);  
    xr = lastRight(sx,sy);
    fillStrip(xl,sy,xr);
    pushSpan(DN,xl,xr,sy);  
    pushSpan(UP,xl,xr,sy);
    while (StackNotEmpty) {

	if (CheckAbort()) {  UnDisplay(); break;}

	popSpan(&dir,&xl,&xr,&y);
	yn =  y + dir;
	if ((yn<fymin)|(yn>fymax)) continue;

/** Note to me:  this search can run all the way to the edge if we are 
    hitting a horiz wall, and the outside is all non fill-color.  The 
    solution is to limit the search to the interval xl,xr.  **/

	/*-- find first 'inside' pixel on new line at or to right
	    of left pixel of old line */
	if ((xml = firstRight(xl,yn)) > xr) continue;

/**  the firstLeft need not search past xl, and if it hits 
nothing then the lastRight need not be done at all */

	/* -- find last pixel of last interval of 'inside' pixels
	    on new line and adjoining old line */
	xmr = lastRight(firstLeft(xr,yn),yn);

	/* -- find the left end of the left most adjoining interval*/
	xtl = xml = lastLeft(xml,yn);
	for (;;) {
	    xtr = lastRight(xtl,yn);
	    pushSpan(dir,xtl,xtr,yn);
	    fillStrip(xtl,yn,xtr);
	    if (xtr==xmr) break;
	    xtl = firstRight(xtr+1,yn);
	    } 
	xl -=2; xr +=2;
	/* check for u-turns */
	if ( (xml<=xl) && (firstLeft(xl,y)>=xml) ) pushSpan(-dir,xml,xl,yn);
	if ( (xmr>=xr) && (firstRight(xr,y)<=xmr) ) pushSpan(-dir,xr,xmr,yn);
	}
    }
	    
