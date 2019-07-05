/*----------------------------------------------------------------------*/
/*  									*/
/*        airbrush.c							*/
/*  									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>
#include <pnts.h>

extern PointRec pnts;
extern BOOL abortFlag, symON;
extern void obmv(), obclr(), (*nop)();
extern SHORT xShft, yShft;

#define local  static

extern SHORT abRadius;

local SHORT ab2Rad = 2*INITABRADIUS, abRad2 = INITABRADIUS*INITABRADIUS;

/* ------ Air Brush Mode ------------------- */
local Point abpt;

local void abDoDot() { mDispOB(abpt.x, abpt.y); }
local airbSym(x,y) SHORT x,y; {
    abortFlag = NO;
    abpt.x = x; 
    abpt.y = y;
    SymDo(1,&abpt,abDoDot);
    }
    
local void AirBDots(x,y,proc) SHORT x,y; void (*proc)(); {
    SHORT dx,dy;
    for(;;) { /* repeat until generate a point */
	/* first get a r.v. uniform in a square area */
	dx = ((Rand16()&0x7fff)%ab2Rad) - abRadius;
	dy = ((Rand16()&0x7fff)%ab2Rad) - abRadius;
	/*  now filter into circular distribution */
	if ( (Random(abRad2)+Random(abRad2))/2 > ((SHORT)(dx*dx+dy*dy))) {
	    (*proc)(x+PMapX(dx),y+PMapY(dy));
	    break;
	    }
	}
    }

local airbsplat() {
    if (symON)  AirBDots(mx,my,airbSym);
    else  AirBDots(mx,my,mDispOB);
    CycPaint();
    }
    
/* ------ Exported procedure calls ----- */
SetAirBRad(n) int n; {
    n = MAX(2,n);  abRadius = n; ab2Rad = 2*n; abRad2 = n*n; 
    }

ChgABRadius(d) int d; { SetAirBRad(abRadius + d); }

IMAirBrush() {
    IModeProcs(obmv,obmv,obclr,airbsplat,nop,airbsplat,nop,nop);
    }



