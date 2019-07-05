/*--------------------------------------------------------------*/
/*        geom.c   -- geometric primitives 			*/
/*	lines, circles, filled circles				*/
/* Coordinates are in Virtual Device Coords (640x400)		*/
/*--------------------------------------------------------------*/

#include <system.h>
#include <prism.h>

extern SHORT xShft,yShft;
extern void (*CurPWritePix)();
#define local 

extern struct RastPort *curRP;

    
/*-----------Line drawing functions------------------*/
/* note: fun must take Physical Device coords        */

void PLineWith(x1,y1,x2,y2,fun) SHORT (*fun)(), x1,y1,x2,y2;     {
    SHORT i,dx,dy,incr1,incr2,del,xinc,yinc,linex,liney;
    dx = x2-x1;
    dy = y2-y1;
    if (dx<0) {xinc = -1; dx = -dx;} else xinc = 1;
    if (dy<0) {yinc = -1; dy = -dy;} else yinc = 1;
    linex=x1; liney=y1;
    (*fun)(linex,liney);
    if (dx>dy) {
	incr1 = dy+dy;  del = incr1-dx;  incr2= del-dx;
	for (i=0; i<dx; i++) {
	    linex += xinc;
	    if (del<0) del += incr1; else { liney += yinc; del += incr2; }
	    (*fun)(linex,liney);
	    if (CheckAbort()) return;
	    }
	}
    else {
	incr1 = dx+dx;  del = incr1-dy;  incr2= del-dy;
	for (i=0; i<dy; i++) {
	    liney += yinc;
	    if (del<0) del += incr1; else { linex += xinc; del += incr2; }
	    (*fun)(linex,liney);
	    if (CheckAbort()) return;
	    }
	}
    }

/* some cheapo clipping */
extern SHORT clipMaxX;
void PLine(x1,y1,x2,y2) SHORT x1,y1,x2,y2;  { 
    if (x2>clipMaxX) { 
	if (x1>clipMaxX) return;
	y2 = y1 +(clipMaxX-x1)*(y2-y1)/(x2-x1); 
	x2 = clipMaxX;
	}
    else if (x1>clipMaxX) { 
	y1 = y2 + (clipMaxX-x2)*(y1-y2)/(x1-x2); 
	x1 =  clipMaxX;
	}
    Move(curRP,x1,y1); Draw(curRP,x2,y2);
    }
    
/* ---------------- Circle Drawing -----------------------*/
SHORT xcen,ycen;
SHORT (*curfunc)();

void quadpts(x,y) SHORT x,y; {
    (*curfunc)(xcen+x,ycen+y);   
    if (y) (*curfunc)(xcen+x,ycen-y);
    if (x) {
	(*curfunc)(xcen-x,ycen+y);
	if (y) (*curfunc)(xcen-x,ycen-y);
	}
    }

local octpts(x,y) SHORT x,y; { quadpts(x,y);  if (x!=y) quadpts(y,x);  }
    
local octfillfunc(x,y) SHORT x,y; {
    SHORT w = 2*x+1;    SHORT h = 2*y+1;
    PatHLine(xcen-x,ycen+y,w);
    PatHLine(xcen-x,ycen-y,w);
    PatHLine(xcen-y,ycen+x,h);
    PatHLine(xcen-y,ycen-x,h);
    }
    
/* generates 1/8 of a circle and calls func at each point */        
/* this should ONLY be called if xShft == yShft 	  */
/* ie square pixels					  */

void PCircOct(xc,yc,rad,func) SHORT xc,yc,rad,(*func)(); {
    SHORT d,x,y;
    y = rad; x=0;
    xcen=xc;   ycen = yc;
    for (d = 3 - (rad<<1); x<y; x++) {
	(*func)(x,y);
	if (d<0) d += (x<<2) + 5;
	else { d += ((x-y)<<2) + 10; y--; }
	if (CheckAbort()) return;
	}
    if (x == y) (*func)(x,y);
    }

PCircWith(xc,yc,rad,func) SHORT xc,yc,rad,(*func)(); {
    if (xShft==yShft) { curfunc = func;	PCircOct(xc,yc,rad,octpts);}
    else PEllpsWith(xc,yc,rad,PMapY(VMapX(rad)),func);
    }
    
PFillCirc(xc,yc,rad)   SHORT xc,yc,rad;   {
    if (xShft==yShft) PCircOct(xc,yc,rad,octfillfunc); 
    else PFillEllps(xc,yc,rad,PMapY(VMapX(rad)));
    }

PCircle(xc,yc,rad) SHORT xc,yc,rad; {
    if (xShft==yShft)  PCircWith(xc,yc,rad,CurPWritePix);
    else PEllpse(xc,yc,rad,PMapY(VMapX(rad)));
    }
    
    
   