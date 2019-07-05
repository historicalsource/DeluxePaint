/*----------------------------------------------------------------------*/
/*									*/
/*        magops.c -- this is essentially an extension to cgraph 	*/
/*  which implements the graphics ops in magnified windows.		*/
/*									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>

#define local static

extern SHORT curxpc;
extern BMOB *curob;
extern Box bigBox;
extern struct RastPort *curRP;
extern BOOL usePen;
extern void (*CurPWritePix)();
extern SHORT xorg,yorg,magN;
extern BOOL firstDown;
extern SHORT clipMaxX;

Box bx;

mReadPix(x,y) SHORT x,y; { return(PReadPix(x+xorg,y+yorg)); }

/*------------ Paint current object------------*/

dispob(x,y) SHORT x,y; { DispBMOB(curob,x,y); }

ProcHandle SplatOp() {
    return((usePen==0)? CurPWritePix: &dispob);
    }

UpdtPtBox(x,y) SHORT x,y; {  UpdtSibs(MakeBox(&bx,x,y,1,1));   }

mDispOB(x,y) SHORT x,y; {
    (*SplatOp())(x+xorg,y+yorg);
    UpdtPtBox(x,y);
    }

mDrawDot(x,y) {  (*CurPWritePix)(x+xorg,y+yorg); UpdtPtBox(x,y); }

mMoveOB(x,y)  SHORT x,y; {
    SHORT oldx,oldy;
    oldx = curob->pict.box.x - xorg + curob->xoffs;
    oldy = curob->pict.box.y - yorg + curob->yoffs;
    MoveBMOB(curob, x+xorg, y+yorg); /*go to screen directly*/
    UpdtSibs(BoxTwoPts(&bx,x,y,oldx,oldy)); 
    }

mClearOB() {
    SHORT oldx,oldy;
    oldx = curob->pict.box.x - xorg + curob->xoffs;
    oldy = curob->pict.box.y - yorg + curob->yoffs;
    ClearBMOB(curob);
    UpdtPtBox(oldx,oldy);
    }

/*---------------magnified Line ------------*/
mLine(x1,y1,x2,y2) SHORT x1,y1,x2,y2; {
    firstDown = YES;
    if (usePen==0) PLine(x1+xorg, y1+yorg, x2+xorg, y2+yorg);
    else PLineWith(x1+xorg,y1+yorg, x2+xorg, y2 +yorg, SplatOp());
    UpdtSibs(BoxTwoPts(&bx,x1,y1,x2,y2));
    }
	
/* --------------magnified Rectangle -----------*/
mRect(b) Box *b; {
    PFillBox(MakeBox(&bx,b->x+xorg, b->y+yorg,b->w,b->h));
    UpdtSibs(b);
    }

/*---------------magnified Curve ------------*/
mCurve(x0,y0,x1,y1,x2,y2) SHORT x0,y0,x1,y1,x2,y2; {
    firstDown = YES;
    PCurve(x0+xorg,y0+yorg,x1+xorg,y1+yorg,x2+xorg,y2+yorg,64,SplatOp());
    UpdtSibs(BoxThreePts(&bx,x0,y0,x1,y1,x2,y2));
    }
    
local OvalUpdt(x,y,a,b) SHORT x,y,a,b; {
    UpdtSibs(MakeBox(&bx,x-a,y-b,2*a+4,2*b+4));
    }

/*---------------magnified Circle ------------*/
mCircle(x,y,rad) SHORT x,y,rad; { 
    firstDown = YES;
    PCircWith(x + xorg, y + yorg, rad, SplatOp()); 
    OvalUpdt(x,y,rad,rad);
    }

mFillCircle(x,y,rad) SHORT x,y,rad; {
    PFillCirc(x + xorg, y + yorg ,rad);
    OvalUpdt(x,y,rad,rad);
    }

iabs(x) int x; { return( (x>0)? x: -x ); }

/*---------------magnified Ellipse ------------*/
mOval(x1,y1,x2,y2) SHORT x1,y1,x2,y2; {
    SHORT halfw,halfh;
    firstDown = YES;
    halfw = iabs(x2-x1); halfh = iabs(y2-y1);
    PEllpsWith(x1 +xorg, y1 + yorg, halfw, halfh, SplatOp());
    OvalUpdt(x1,y1,halfw,halfh);
    }

mFillOval(x1,y1,x2,y2) SHORT x1,y1,x2,y2; {
    SHORT halfw,halfh;
    halfw = iabs(x2-x1); halfh = iabs(y2-y1);
    PFillEllps(x1 +xorg, y1 + yorg, halfw, halfh);
    OvalUpdt(x1,y1,halfw,halfh); 
    }

/*---------------Magnified Area Flood ------------*/
mFillArea(Bndry,x,y) BOOL Bndry;SHORT x,y; {
    UpdtON();
    if (Bndry) PFloodBndry(x+xorg,y+yorg); else  PFloodArea(x+xorg,y+yorg);
    UpdtSibs(&bigBox);
    UpdtOFF();
    }

/* Frame rectangle with border of thickness 1 */
mFrame(b) Box *b; {
    firstDown = YES;
    MakeBox(&bx,b->x + xorg,b->y + yorg,b->w,b->h);    
    PThinFrame(&bx);
    UpdtSibs(b);
    } 

/* Frame rectangle with border using current brush */
mOBFrame(b) Box *b; {
    SHORT xm = b->x + b->w -1;
    SHORT ym = b->y + b->h -1;
    mLine(b->x, b->y, xm, b->y);
    mLine(xm, b->y, xm, ym);
    mLine(xm, ym, b->x, ym);
    mLine(b->x,ym, b->x, b->y);
    } 

