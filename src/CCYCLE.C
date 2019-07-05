/*----------------------------------------------------------------------*/
/*  									*/
/*        ccycle -- Color cycling and cycle paint			*/
/*  									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>

extern struct ViewPort *vport;
extern struct RastPort *curRP;
extern SHORT nColors;
extern BOOL erasing;

#define local static 


/* cycling frame rates */
#define OnePerTick    	16384
#define OnePerSec	OnePerTick/60

Range cycles[MAXNCYCS] = {
    /* count, rate, active, low, high */
    {0, 0, YES, 20, 31 },		/* shade range */
    {0, OnePerSec*10, YES, 3,7 },
    {0, OnePerSec*10, YES, 0,0 },
    {0, OnePerSec*10, YES, 0,0 }
    };

local BOOL cycling = NO;
local BOOL wasCyc = NO;
local BOOL paused = NO;
local LONG timer = 0;

local WORD colors[MAXNCOLORS];
local WORD saveCols[MAXNCOLORS];

SHORT curlow=0,curhigh=31;
Range *curCyc = NULL;
Range *shadeRange = cycles;

extern void SetCycPaint();


LoadCols(cols) WORD cols[]; { LoadRGB4(vport,cols,nColors); }

local void StartCCyc()  { 
    if (cycling) return;
    GetColors(saveCols); GetColors(colors);   
    StartVBlank(); 
    cycling = YES;
    }
    
local void StopCCyc() { 
    if (!cycling) return;
    StopVBlank(); 
    LoadCols(saveCols); 
    cycling = NO; 
    }
	
KillCCyc() {
    if (cycling) StopCCyc();
    else if (paused) wasCyc = NO;
    }

TogColCyc() { 
    if (paused) wasCyc = !wasCyc; else
    if (!cycling)  StartCCyc();   else StopCCyc();
    }
PauseCCyc() { wasCyc = cycling;  if (cycling) StopCCyc(); paused = YES; }
ResumeCCyc() { if (wasCyc) StartCCyc(); paused = NO; }

BOOL NoCycle = NO;

local void CycleColors()    {            
    int i,j;  Range *cyc;
    WORD temp;
    BOOL anyChange = NO;
    if (!cycling) return; 
    for (i = 1; i<MAXNCYCS; i++) {
	cyc = &cycles[i];
	if (cyc->active) {
	    cyc->count += cyc->rate;
	    if (cyc->count >= OnePerTick ) {
		anyChange = YES;
		cyc->count -= OnePerTick;
		temp = colors[cyc->high];
		for (j = cyc->high; j >cyc->low; j--) colors[j] = colors[j-1];
		colors[cyc->low] = temp;
		}
	    }
	}
    if (anyChange&&(!NoCycle)) LoadCols(colors);
    }

/*----------- Vertical Blank Routine to Cycle state.colors --------------*/
local LONG mystack[64];          /* stack space for the int handler */
local char myname[]="MyVb";       /* Name of the int handler */
local struct Interrupt intServ;

typedef void (*VoidFunc)();

local MyVBlank() {
    timer++;
    CycleColors();
    return(0);
    }
    
local StartVBlank()   {
    intServ.is_Data = (APTR) mystack;
    intServ.is_Code = (VoidFunc)&MyVBlank;
    intServ.is_Node.ln_Succ = NULL;
    intServ.is_Node.ln_Pred = NULL;
    intServ.is_Node.ln_Type = NT_INTERRUPT;
    intServ.is_Node.ln_Pri = 0;
    intServ.is_Node.ln_Name = myname;
    AddIntServer(5,&intServ);
    }

local StopVBlank() { RemIntServer(5,&intServ); }


/* ---- Incrementally Change Color table Values --------------- */    

local SetCTab(c,i) SHORT c,i;{SetRGB4(vport, i, (c>>8)&0xf, (c>>4)&0xf, c&0xf); }

IncRGB(inc)SHORT inc; {
    SHORT n = curRP->FgPen;
    if (!cycling)   SetCTab(GetRGB4(vport->ColorMap,n)+inc , n);
    else {
	cycling = NO;
	saveCols[n] += inc;
	movmem(saveCols,colors,MAXNCOLORS*sizeof(SHORT));
	LoadCols(colors);
	cycling = YES;
	}
    }


BOOL cycPntOn = NO;


/*
TogCycPaint() {   cycPntOn = !cycPntOn; SetCycPaint();    }
*/


local Range *FindCycle(n) SHORT n; {
    int i;
    Range *cyc;
    for (i=1; i<MAXNCYCS; i++) {
	cyc = &cycles[i];
	if ( (n>=cyc->low) && (n<=cyc->high) ) return(cyc);
	}
    return(NULL);
    }

static SHORT cycFgPen = -1;

void SetCycPaint() {
    cycFgPen = curRP->FgPen;
    curCyc = FindCycle(cycFgPen);
    if (curCyc!=NULL) { curlow = curCyc->low; curhigh = curCyc->high;  }
    else { curlow = 0; curhigh = nColors-1; }
    }


OnCycPaint(){ cycPntOn = YES; SetCycPaint(); }
OffCycPaint() { cycPntOn = NO; SetCycPaint(); }

/*----------------------------------------------------------------------*/
/*        cycle Painting						*/
/*----------------------------------------------------------------------*/

/* calling this routine will increment the current painting color */
/* You dont want to be updating the control panel every time and this 
doesn't. Call EndCycPaint() to do that. */

CycPaint() {
    SHORT n;
    if (cycPntOn&&(curCyc!=NULL)&&(!erasing)) {
	n = curRP->FgPen + 1;
	if (n > curhigh) n = curlow;
	SetFGCol(n); 
	}
    }

EndCycPaint(){ CPChgCol(curRP->FgPen); }

    
