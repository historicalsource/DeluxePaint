/*----------------------------------------------------------------------*/
/*  									*/
/*     pnts.h								*/
/*  									*/
/*----------------------------------------------------------------------*/

typedef struct {
    SHORT pntmx,pntmy;		/* (mx,my) current point */
    SHORT pntlx,pntly;		/* (lx,ly) last point  */
    SHORT pntsx,pntsy;		/* (sx,sy) start coord (button down) */
    SHORT pntex,pntey;		/* (ex,ey) end coord (button up) */
    SHORT pntpsx,pntpsy;	/* (psx,psy) previous button down */
    SHORT pntpex,pntpey;	/* (pex,pey) previous button up */
    } PointRec;

#define mx	 pnts.pntmx
#define my	 pnts.pntmy
#define lx	 pnts.pntlx
#define ly	 pnts.pntly
#define sx	 pnts.pntsx
#define sy	 pnts.pntsy
#define ex	 pnts.pntex
#define ey	 pnts.pntey
#define psx	 pnts.pntpsx
#define psy	 pnts.pntpsy
#define pex	 pnts.pntpex
#define pey	 pnts.pntpey
