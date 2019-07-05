/*--------------------------------------------------------------*/
/*																*/
/*						Symmetry 								*/
/*																*/
/*				2/26/85 ------  D.Silva						   	*/
/*--------------------------------------------------------------*/
#include <system.h>
#include <prism.h>

extern SHORT xShft, yShft;
		    
#define local 

#define min(a,b)  ( (a)<(b) ? (a): (b) )
#define max(a,b)  ( (a)>(b) ? (a): (b) )
#define abs(a) ( (a>0) ? (a): -(a) )

/* define a 32 bit fixed point number with 16 bits of fraction */

typedef struct{SHORT x,y;} SPoint;
        
extern LongFrac LFMULT();
        
#define epsi 2
local LongFrac pi =0x3243FL;
local LongFrac halfpi =0x1921FL;
local LongFrac twopi = 0x6487EL;

/* sin of x (x in radians )  */
LongFrac lfsin(x) LongFrac x; {
    int n;
    LongFrac res,inc,xsq;
    BOOL neg = NO;
    if (x<0) {x = -x; neg = !neg;}
    x = x%twopi;
    if (x>pi) {x -= pi; neg = !neg;}
    if (x>halfpi) x = pi-x; 
    res = inc = x; 
    xsq = LFMULT(x,x); 
    n = 2;
    while (abs(inc)>epsi) {
		inc = -LFMULT(inc,xsq)/(n*(n+1));
		res += inc;
		n += 2;
		}
    return(neg?-res:res);
    }

/* cos of x (x in radians )  */
#define lfcos(x) 		lfsin(x+halfpi)

#define NSYMMAX 40

SHORT nSym;

local SHORT pxsym,xsym,ysym,nSymhalf;

local SHORT sintab[20],costab[20];
    
#define ISin(ang)  ((ang>nSymhalf)? -sintab[nSym-ang]: sintab[ang])
#define ICos(ang)  ((ang>nSymhalf)?  costab[nSym-ang]: costab[ang])

#define SCF		14
#define oneHalf  (1<<(SCF-1))

BOOL mirror = NO;

SymSetNMir(n,mir) int n; BOOL mir; {
    LongFrac a;
    int i;
    mirror = mir;
    nSym=max(min(n,NSYMMAX),1);
    nSymhalf = nSym/2;
    for (i=0; i<=nSymhalf; i++) {
		a = (i*twopi)/n;
		sintab[i]= (SHORT)  (lfsin(a) >> (16-SCF)) ;
		costab[i]= (SHORT)  (lfcos(a) >> (16-SCF)) ;
		}
    }
    
SymSet(n,mir,xc,yc) int n,xc,yc; BOOL mir; {
    mirror = mir;
    pxsym = xc;
    xsym = VMapX(xc); 
    ysym = VMapY(yc);
    SymSetNMir(n,mir);
    }

SymCenter(xc,yc) int xc,yc; { pxsym = xc; xsym = VMapX(xc);  ysym = VMapY(yc); }

/*--------------------------------------------------------------*/
/*		SymDo													*/
/*																*/
/*		Calls proc for each symmetry position, with the array	*/
/*  pts[] updated to reflect the symmetry 						*/
/*																*/
/*--------------------------------------------------------------*/

void SymDo(np,pts,proc) 
    int np;				/* Number of Points */
    SPoint *pts;		/* Array of points to update */
    int(*proc)();    		/* Proc to call for each symmetry pos */
    {
    SPoint dp[20],svpts[20];
    SHORT sj,cj,i,j;
    SHORT ddx,ddy;
	LONG lx,ly;
    movmem(pts,svpts,np*sizeof(SPoint));
    for (i=0; i<np; i++) { 
		dp[i].x = VMapX(pts[i].x)-xsym; 
		dp[i].y = VMapY(pts[i].y)-ysym; 
		}
    for (j=0; j<nSym; j++) {
		sj = ISin(j); cj = ICos(j);
		for (i=0; i<np; i++) {
		    ddx = dp[i].x;    ddy = dp[i].y;
			lx = PMapX(xsym + ( (ddx*cj+ddy*sj+oneHalf) >> SCF ));
		    ly = PMapY(ysym +( (-ddx*sj+ddy*cj+oneHalf) >> SCF ));
			pts[i].x = lx; 
			pts[i].y = ly;
			}
		(*proc)();
		if (CheckAbort()) break;
		if (mirror) {
		    for (i=0; i<np; i++) pts[i].x = 2*pxsym - pts[i].x;
		    (*proc)();
		    }
		if (CheckAbort()) break;
		}
    movmem(svpts,pts,np*sizeof(SPoint));
    }

