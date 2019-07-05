/*----------------------------------------------------------------------*/
/*									*/
/*		dispnum.c --Coordinate readout				*/
/*									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>

extern SHORT xShft, yShft;
extern BOOL titleShowing;
extern struct RastPort screenRP;
extern SHORT curFormat;
extern SHORT curMBarH;	    
#define local static

/*--------------------------------------------------- */
/* pad string a out (on left) to n chars and store in b */

strpad(a,b,n,neg) char *a,*b; SHORT n; BOOL neg; {
    SHORT i,m,pad;
    m = MIN(n,strlen(a));
    pad = n-m;
    for (i=pad; i<n; i++) b[i] = *a++;
    b[n] = 0;
    for(i=0; i<pad; i++) b[i] = ' ';
    if (neg &&(pad>0)) b[pad-1] = '-';
    }

TextXY(s,x,y) char *s; SHORT x,y; {
    Move(&screenRP,PMapX(x),PMapY(y));
    Text(&screenRP,s,strlen(s));
    }
/*
DispColStr(s,x,y,col) char *s; SHORT x,y,col; {
    SetAPen(&screenRP,col);
    SetDrMd(&screenRP,JAM1);
    TextXY(s,x,y);
    }
*/
    
DispStr(s,x,y) char *s; SHORT x,y; {
    SetAPen(&screenRP,0);
    SetBPen(&screenRP,1);
    SetDrMd(&screenRP,JAM2);
    TextXY(s,x,y);
    }
    
DispNum(n,x,y,ln) {
    char str[12],rtjust[12];
    BOOL neg;
    if (n<0) { neg = YES; n = -n;} else neg = NO;
    stcu_d(str,n,9);
    strpad(str,rtjust,ln,neg);
    DispStr(rtjust,x,y);	/* draw text */
    }

/* Bitmap name = xarrow,  Amiga-BOB format.   */
/* Width = 11, Height = 5 */ 


/* Bitmap name = xarrow,  Amiga-BOB format. */
/* Width = 11, Height = 5 */ 

short xarrow[5] = {  0xFE60,  0xFF20,  0x0,  0xFF20,  0xFE60  }; 

/* Bitmap name = yarrow,  Amiga-BOB format. */
/* Width = 5, Height = 10 */ 

short yarrow[10] = {  0xD800,  0xD800,  0xD800,  0xD800,  0xD800, 
  0xD800,  0x5000,  0x0,  0x8800,  0xD800  }; 

local struct Image xarrIm = {0,2,11,5,1,xarrow,1,1,NULL};
local struct Image yarrIm = {0,0,5,10,1,yarrow,1,1,NULL};

UBYTE *modeNames[] = { "Object","Color","Replace","Smear","Shade",
"Blend","Cycle" };

extern PaintMode userPntMode;

local SHORT pntx[3] = { 232, 134, 158 };
local SHORT pnty[3] = { 14, 14, 7 };

#define titleY   
DispPntMode()  {
    SHORT vx,vy;
    if (titleShowing) {
	SetAPen(&screenRP,1);
	vx = pntx[curFormat];
	vy = pnty[curFormat];
	RectFill(&screenRP,PMapX(vx), 0, PMapX(vx+220), curMBarH-2);
	DispStr(modeNames[userPntMode], vx, vy);
/**
	if (curFormat == 2) {
	    SetAPen(&screenRP,0);
	    Move(&screenRP, 0, curMBarH-2);
	    Draw(&screenRP, 639, curMBarH-2);
	    }
**/
	}
    }

ClearXY() {
    if (titleShowing) {
	SetAPen(&screenRP,1);
	RectFill(&screenRP,PMapX(388), PMapY(0), PMapX(564), curMBarH-2);
	}
   }

/***
extern char dpaintTitle[];
DispDPaint() { 
    if (titleShowing) DispColStr(dpaintTitle,VMapX(3),VMapY(7),2);  }
***/

extern char version[];
DispAvailMem() {
    int m,vy,l;
    if (titleShowing) {
	vy = pnty[curFormat];
	DispStr(version,0,vy);
	l = TextLength(&screenRP, version, strlen(version));
	m = AvailMem(MEMF_PUBLIC);
	DispNum(m,VMapX(l),vy,7);
	}
    }

#define XMIDP 452
#define YMIDP 544
SHORT numW[] =  { 3*8*2,3*8,3*9};

DispXY(x,y) {
    SHORT vy,w;
    if (titleShowing) {
	vy = pnty[curFormat];
	w = numW[curFormat];
	DispNum(x, XMIDP - w, vy, 3);
	DrawImage(&screenRP,&xarrIm, PMapX(XMIDP), PMapY(0));
	DispNum(y, YMIDP - w, vy, 3);
	DrawImage(&screenRP,&yarrIm, PMapX(YMIDP), PMapY(0));
	}
    }


