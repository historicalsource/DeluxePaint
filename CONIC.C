/*-------------------------------------------------------------	*/
/*								*/
/*			Conic Curves				*/
/*								*/
/*-------------------------------------------------------------	*/

#include <system.h>
#include <prism.h>

#define local static

extern SHORT xShft, yShft;
extern void (*CurPWritePix)();
extern void (*curfunc)();
extern void quadpts();
extern SHORT xcen,ycen;

/* table of move increments */
typedef struct { SHORT dx1,dy1,dx2,dy2;} movRec;

BOOL cncdbg = NO;
	        
local movRec moves[8] = {
    { 1, 0, 1, 1},
    { 0, 1, 1, 1},
    { 0, 1,-1, 1},
    {-1, 0, -1, 1},
    {-1, 0, -1,-1},
    { 0,-1,-1, -1},
    { 0,-1, 1, -1},
    { 1, 0, 1,-1}
    };

local movRec cmv;
local BYTE nxtdiag[8] = {1,0,3,2,5,4,7,6};
local BYTE nxtsq[8]   = {7,2,1,4,3,6,5,0};
local SHORT octchgs;
local LONG w,a,b,d,k1,k2,k3;
local SHORT eps = 1;
    
typedef union{
    LONG l;
    struct { SHORT hi,lo;} wrd;
    } LongInt;

BYTE curoct;
BYTE octTab[8] = {0,1,3,2,7,6,4,5};

#define NFRAC	4
#define scale(foo)	(foo<<NFRAC)
#define ival(foo) ( (SHORT) (foo>>NFRAC))
LONG longone = 0x100;

LONG LF(i) SHORT i; {
    LongInt lf;
    lf.wrd.lo = 0;
    lf.wrd.hi = i;
    return(lf.l);
    }
     
LONG lngtmp;
#define lswap(a,b) {lngtmp = a; a = b; b = lngtmp; }
#define neg(a)  (a = -a)

/*
static prstate() {
    if (cncdbg)
  printf("oct=%d, k1=%d, k2=%d, k3=%d, b=%d, a=%d,d=%d \n",
       curoct,ival(k1),ival(k2),ival(k3),ival(b),ival(a),ival(d));
    }
*/     

          
sqOctChange() {
    curoct = nxtsq[curoct];
    cmv = moves[curoct]; 
    w = k2-k1;
    k1 = -k1;
    k2 = k1+w;
    k3= 4*w-k3;
    b = -w-b;
    d = b-a-d;
    a -= 2*b+w;
    }

diagOctChange() {
    curoct = nxtdiag[curoct];
    cmv = moves[curoct]; 
    w = 2*k2-k3;
    k1 = w-k1;
    k2 -= k3;
    k3 = -k3;
    b += a - (k2>>1);
    d = b -(a>>1) -d +(k3>>3);
    a = (w>>1)-a;
    }
    
#define cabs(foo) (( (tmp=(foo)) > 0 ) ? tmp : -tmp )    

/*--------------------------------------------------------------*/
/* plot general conic whose equation is				*/
/*    alpha*y*y + beta*x*x + 2*gamma*x*y + 2*u*y -2*v*x = 0;	*/
/* the curve is displaced so that (0,0) maps to (x0,y0)		*/
/*--------------------------------------------------------------*/
    
void Conic(alpha,beta,gamma,u,v,x0,y0,x1,y1,func,maxoct,tol)
    LONG alpha,beta,gamma,u,v;
    SHORT x0,y0;		/* starting point*/
    SHORT x1,y1;		/* stopping point*/
    SHORT (*func)(); 		/* function to call */
    SHORT maxoct;		/* limit on number of octant changes */
    SHORT tol;			/* tolerance for termination test */
    {
    SHORT x,y,tmp;
    x = x0; y = y0; 
/*
    if (cncdbg)  printf("alpha =%d, beta= %d, gamma= %d, u= %d, v= %d \n",
       ival(alpha),ival(beta),ival(gamma),ival(u),ival(v));
*/
    curoct = 0;
    if (v<0) curoct += 4;
    if (u<0) curoct += 2;
    if (ABS(v)>ABS(u)) {curoct+=1; lswap(alpha,beta); lswap(u,v); neg(gamma);}
    curoct = octTab[curoct];
    if (curoct&1) { neg(beta); neg(alpha); }
    if ((curoct>2)&&(curoct<7)) neg(u); 
    if (curoct>3) neg(v);  if ( (curoct&3) == 2 ) neg(v); 
    k1 = 2*beta;
    k2 = k1 + 2*gamma;
    k3 = 2*alpha + 2*gamma + k2;
    b = 2*v - beta - gamma;
    a = 2*u - b;
    d = b - u - (alpha>>2);
    cmv = moves[curoct]; 
    octchgs = 0;
    (*func)(x,y);
    for (;;) {
	if (d<0){  /* move 1 */
	    x += cmv.dx1; y += cmv.dy1;
	    b -=  k1;     a +=  k2;	  d +=  b;
	    } 
	else {   /* move 2 */
	    x += cmv.dx2; y += cmv.dy2;
	    b -=  k2;     a +=  k3;	  d -=  a;
	    }
	(*func)(x,y);
	if ((cabs(x-x1)<tol)&&(cabs(y-y1)<tol)) break;
	if (b<0) { sqOctChange(); if ((++octchgs)>maxoct) break;}
	else if (a<0) { diagOctChange(); if ((++octchgs)>maxoct) break;}
	if (CheckAbort()) return;
	}
    if ((x!=x1)||(y!=y1)) (*func)(x1,y1);
    }


/*--------------------------------------------------------------*/
/*  Conic Curve given start point, stop point, and middle point */
/* which defines the tangents at the start and stop point	*/
/*--------------------------------------------------------------*/
PCurve(x0,y0,x1,y1,x2,y2,lam,func) 
    SHORT x0,y0,x1,y1,x2,y2;
    USHORT lam;  /* 8 bit binary fraction between 0 and 1 */
    SHORT (*func)();
    {
    LONG a1,a2,a3,b1,b2,b3,det,lambda,alam,halam;
    LONG alpha,beta,gamma,u,v;
/*
    if (cncdbg) printf("conic x0=%d,y0=%d,x1=%d,y1=%d,x2=%d,y2=%d \n",
	    x0,y0,x1,y1,x2,y2);

*/
    lambda =  lam;
    alam = longone - lambda;
    halam = alam>>1;
    a1 = y0-y1; b1 = x1-x0;
    a2 = y2-y1; b2 = x1-x2;
    a3 = y2-y0; b3 = x0-x2;
    det = a1*b3 - a3*b1;
/*    if (cncdbg) printf(" det = %u|%d \n",det); */
    if (det==0) 
	{ alpha = beta = gamma = (LONG) 0 ; u = -scale(b3); v = scale(a3); }
    else {
	alpha = alam*b1*b2 + lambda*b3*b3;
	beta = alam*a1*a2 + lambda*a3*a3;
	gamma = halam*(a1*b2+a2*b1) + lambda*a3*b3;
	u = halam*(b1*det);
	v = -halam*(a1*det);
	if (det<0) { neg(alpha); neg(beta); neg(gamma); neg(u); neg(v); }
	}
    Conic(alpha,beta,gamma,u,v,x0,y0,x2,y2,func,8,2);
    }
    
void PEllpsWith(xc,yc,a,b,func) SHORT xc,yc,a,b; SHORT (*func)(); {
    LONG alpha,beta,gamma,u,v;
    if ((a<2)||(b<2)) return;
    alpha = scale(((LONG)a)*((LONG)a));
    beta =  scale(((LONG)b)*((LONG)b));
    gamma = (LONG)0;
    u = ((LONG)b)*alpha;
    v = (LONG)0;
    Conic(alpha,beta,gamma,u,v,xc,yc+b,xc,yc+b,func,8,1);
    }

/* traverse 1 quadrant of ellipse, calling "func" at each point */
void PQEllps(xc,yc,a,b,func) SHORT xc,yc,a,b; SHORT (*func)(); {
    LONG alpha,beta,gamma,u,v;
    if ((a<1)||(b<1)) return;
    alpha = scale(((LONG)a)*((LONG)a));
    beta =  scale(((LONG)b)*((LONG)b));
    gamma = (LONG)0;
    u = ((LONG)b)*alpha;
    v = (LONG)0;
    xcen = xc;
    ycen = yc;
    Conic(alpha,beta,gamma,u,v,0,b,a,0,func,3,2);
    }

PQEllpsWith(xc,yc,a,b,func) SHORT xc,yc,a,b; void (*func)(); {
    curfunc = func;
    PQEllps(xc,yc,a,b, &quadpts); 
    }

/* fast 1 dot thick ellipse */
PEllpse(xc,yc,a,b) SHORT xc,yc,a,b; {
    curfunc = CurPWritePix;
    PQEllps(xc,yc,a,b, &quadpts); 
    }

static SHORT qfill(x,y) SHORT x,y; {
    SHORT w = 2*x+1;
    PatHLine(xcen-x,ycen+y,w);
    PatHLine(xcen-x,ycen-y,w);
    }
    
PFillEllps(xc,yc,a,b) SHORT xc,yc,a,b; {  PQEllps(xc,yc,a,b,qfill);  }



