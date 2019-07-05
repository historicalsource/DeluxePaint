/*----------------------------------------------------------------------*/
/*  									*/
/*        cursor.c -- Plug in different cursors				*/
/*  									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>

extern struct Window *mainW;
extern BOOL inside;
extern SHORT curpen;

extern UWORD fillCurs[],crossCurs[],zzz[],size[], pick[];

typedef struct { SHORT w,h,xoffs,yoffs; UWORD *bits; } CursDesc;

CursDesc cursDir[] = { 
    {13,17,7,16, fillCurs},
    {15,15,8,7, crossCurs},
    {16,25,8,12, zzz},
    {8,31,0,0, size},
    {11,43,1,0, pick}
    };

    
static SHORT nullSprite[2] = {0};

SHORT curCursor = DEFCURSOR; 	/* the actual cursor now showing */
SHORT curPaintCursor = DEFCURSOR;  	/* the painting cursor */
SHORT paintCursOn = YES;		/* is the painting cursor showing */

    
/* these ignore the Painting cursor */
HideCursor() { SetPointer(mainW, nullSprite, 0, 0, 0, 0);}
DefaultCursor() {ClearPointer(mainW);}

JamCursor(n) int n; {
    CursDesc *cd = &cursDir[n-2];
    SetPointer(mainW, cd->bits,cd->h,cd->w,-cd->xoffs,-cd->yoffs); 
    }

void SetCursor(n) {
    curCursor = n;
    if (n==NOCURSOR) HideCursor();
    else if (n==DEFCURSOR) DefaultCursor();  else JamCursor(n);
    }
    
/* this affects the Painting cursor */
void SetPaintCursor(n) int n; {
    curPaintCursor = n;
    if (!inside) SetCursor(DEFCURSOR); 
    else SetCursor((paintCursOn||!((n==DEFCURSOR)||(n==CROSSCURSOR)))? n: NOCURSOR);
    }

DefaultPaintCursor() {
    SetPaintCursor( (curpen==USERBRUSH)? DEFCURSOR: CROSSCURSOR);
    }

/* restore cursor to current Paint Cursor state */
ResetCursor() { 
    if ( (curPaintCursor ==DEFCURSOR) || (curPaintCursor == CROSSCURSOR))
	DefaultPaintCursor();
    else SetPaintCursor(curPaintCursor); 
    }

/* Change the Paint Cursor from showing to not or vice versa */
TogCursor() {  paintCursOn = !paintCursOn;  ResetCursor();  }

/* These must work event outside of paint window */

ZZZCursor() {  JamCursor(ZZZCURSOR); }
UnZZZCursor() { SetCursor(curCursor); }
