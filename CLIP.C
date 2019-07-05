/*--------------------------------------------------------------*/
/*								*/
/*			  CLIP.C				*/
/*								*/
/*	Provides a clipping interface to the Amiga area fill.	*/
/*	Uses a modified verion of the Sutherland-Hodgman	*/
/*	Reentrant Polygon Clipper.				*/
/*								*/
/*	ClipInit(xmin,ymin,xmax,ymax)   -- sets clip box	*/
/*	ClipDraw  	-- corresponds to AreaDraw		*/
/*	ClipMove  	-- corresponds to AreaMove		*/
/*	ClipEnd   	-- corresponds to AreaEnd		*/
/*								*/
/*	Dan Silva 3-6-85  created				*/
/*	"	  3-10-85 changed to 2-stage from 4-stage	*/
/*			  to speed up.				*/
/*								*/
/*		Copyright 1985, Electronic Arts			*/
/*--------------------------------------------------------------*/

#include "system.h"


/* #define dbgClip */

#define YES  1
#define NO   0
#define NIL  0
#define local static

typedef char Boolean;
typedef struct { int x,y; } Point;

typedef struct {
    short lastcode,isFirst,fcode;
    Point last,f;
    } clrec;
    
local clrec ud = {0,YES,0,{0,0},{0,0}};
local clrec rl = {0,YES,0,{0,0},{0,0}};

local struct RastPort *clrp;

/* ------ clipping box -------*/
local short clxmin = 0;
local short clymin = 0;
local short clxmax = 319;
local short clymax = 199;
local short anyOutput = NO;


local ClipUD(x,y) int x,y; {
    short code = 0;
    short xp,yclip;
#ifdef dbgClip
    printf("ClipUD x= %d, y = %d\n",x,y);
#endif
    if (y<clymin) code|= 1; else if (y>clymax)  code |= 2;
    if (ud.isFirst) {
	ud.f.x = x; ud.f.y = y;
	ud.fcode = 0;
	ud.isFirst = NO;
	}
    else {
	if (!(code|ud.lastcode)) ClipRL(x,y,0);  /* trivial accept */
	else if (!(code&ud.lastcode)) {   /* --cant reject-- */
	    if (ud.lastcode) { /* last point was out of bounds */
		yclip = (ud.lastcode&1)?clymin: clymax;
		xp = ud.last.x + (yclip-ud.last.y)*(x-ud.last.x)/(y-ud.last.y);
		ClipRL(xp,yclip,ud.lastcode);
		}
	    xp = x; yclip = y;
	    if (code) {  /*---- this point is out---- */
		yclip = (code&1)?clymin: clymax;
		xp = ud.last.x + (yclip-ud.last.y)*(x-ud.last.x)/(y-ud.last.y);
		}
	    ClipRL(xp,yclip,code);
	    }
	}
    ud.last.x = x;
    ud.last.y = y;
    ud.lastcode = code;
    }

local short lastrlcode;

local ClipRL(x,y,udcode) 
    int x,y;		/* point clipped by UD */
    short udcode; 	/* records how it was clipped by UD */
    {
    short code = 0;
    short yp,xclip,lcode,ocode;
#ifdef dbgClip
    printf("ClipRL x= %d, y = %d, code = d\n",x,y,udcode);
#endif
    if (x<clxmin) code|= 4; else if (x>clxmax)  code |= 8;
    if (rl.isFirst) {
	rl.f.x = x; rl.f.y = y;
	rl.fcode = udcode;
	rl.isFirst = NO;
	}
    else {
	if (!(code|lastrlcode)) Output(x,y,udcode);  /* trivial accept */
	else if (!(code&lastrlcode)) 
	    {/* ---- cant reject ----- */
	    lcode = rl.lastcode&udcode;
	    if (lastrlcode) { /* last point was out of bounds */
		xclip = (lastrlcode&4)?clxmin: clxmax;
		yp = rl.last.y+(xclip-rl.last.x)*(y-rl.last.y)/(x-rl.last.x);
		Output(xclip,yp,lcode|lastrlcode);
		}
	    yp = y; xclip = x; ocode = udcode;
	    if (code) {  /* ----- this point is out -----*/
		xclip = (code&4)?clxmin: clxmax;
		ocode = lcode|code;
		yp = rl.last.y + (xclip-rl.last.x)*(y-rl.last.y)/(x-rl.last.x);
		}
	    Output(xclip,yp,ocode);
	    }
	}
    rl.last.x = x;
    rl.last.y = y;
    rl.lastcode = udcode;
    lastrlcode = code;
    }


/*---- the final stage: output the points to the area fill */

local Boolean lastOutCode;
local Point first;
local short firstCode;


local Output(x,y,incode) int x,y,incode; {
    short saveState;
#ifdef dbgClip
    printf("Output x= %d, y = %d, code = %d\n",x,y,incode);
#endif
    if (!anyOutput) { 
	AreaMove(clrp,x,y); 
	anyOutput= YES; 
	first.x = x;
	first.y = y;
	firstCode = incode;
	}
    else 
	{
	if ((lastOutCode&incode))
	    { /* "virtual" edges--ie along screen boundary */
	    saveState = clrp->Flags&AREAOUTLINE;
	    clrp->Flags &= ~AREAOUTLINE;
	    AreaDraw(clrp,x,y);
	    clrp->Flags |= saveState;
	    }
	else AreaDraw(clrp,x,y);
	}
    lastOutCode = incode;
    }

local Close() {
#ifdef dbgClip
    printf(" Close\n");
#endif
    if (!ud.isFirst) ClipUD(ud.f.x,ud.f.y);
    if (!rl.isFirst) ClipRL(rl.f.x,rl.f.y,rl.fcode);
    if (anyOutput) Output(first.x,first.y,firstCode);
    ud.isFirst = rl.isFirst = YES;
    anyOutput = NO;
    }

/* ---------- Interface to Outside World -------------*/

ClipInit(xmin,ymin,xmax,ymax) short xmin,ymin,xmax,ymax;
    {
    clxmin = xmin;    clymin = ymin;    
    clxmax = xmax;    clymax = ymax;
    }

local short drawYet = NO;

ClipDraw(rp,x,y) struct RastPort *rp; short x,y; {
#ifdef dbgClip
    printf(" ClipDraw: x = %d, y = %d\n",x,y);
#endif
    clrp = rp;
    ClipUD(x,y);
    drawYet = YES;
    }

ClipMove(rp,x,y) struct RastPort *rp; short x,y; {
#ifdef dbgClip
    printf(" ClipMove: x = %d, y = %d\n",x,y);
#endif
    if (drawYet) Close(); else { ud.isFirst = rl.isFirst = YES; }
    clrp = rp;
    ClipUD(x,y);
    drawYet = NO;
    }

ClipEnd(rp) struct RastPort *rp; {
    BOOL wasAny = anyOutput;
#ifdef dbgClip
    printf(" Clip END -----------------------\n");
#endif
    
    if (drawYet)  Close();
    WaitBlit();
    if (wasAny) AreaEnd(rp);
    drawYet = NO;
    }

static short dum;
static short dum2 = 0;
