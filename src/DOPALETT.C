/*----------------------------------------------------------------------*/
/*  									*/
/*      dopalette -- invoke the palette tool				*/
/*  									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>

extern SHORT nColors;
extern Range cycles[];
extern struct Window *mainW;
extern Range *shadeRange;
extern Box bigBox;
extern BOOL skipRefresh;
extern PaletteGlobals *paletteGlobals;

/* map a  number i, in [0..63], to logarithmic range  */
/* this maps 63 to 8192(OnePerTick), and maps 0 to 36, about 227 ticks 
    per change, i.e. nearly 4 seconds . */
    
rateFromIndex(i) int i; {
    int base;
    i++;
    base = 1 << ((i >> 3) + 5);
    return (base + ((i & 7) * base) / 8);
    }

/* this is the exact inverse of the above function*/
indexFromRate(n) int n; {
    int j;
    int hibit, mod;
    hibit = (1 << 15);
    for (j = 15;  j >= 0;  j--)	{
	if (hibit & n)  {
	    mod = n - hibit;
	    return ((j - 5) * 8 + (mod * 8) / hibit - 1);
	    }
	hibit >>= 1;
	}
    }	
/* ----- Display color pallet Control ----- */
void ShowPallet() { 
    int i;
    BOOL res;
    UndoSave();
    PauseCCyc();
    for (i=1; i<MAXNCYCS; i++) cycles[i].rate = indexFromRate(cycles[i].rate);
    FreeTmpRas();
    for (;;) { 
	res = PaletteTool(mainW,&paletteGlobals);
	WaitRefrMessage();
	if (res) break;
	UpdtDisplay();
	}
    skipRefresh = YES; /* where this extra refresh comes from I don't  know */
    AllocTmpRas();
    PaneRefresh(&bigBox);
    for (i=1; i<MAXNCYCS; i++) cycles[i].rate = rateFromIndex(cycles[i].rate);
    ResumeCCyc();
    SetCycPaint();
    DispPntMode();
    if (cycles[0].low ==cycles[0].high) shadeRange = NULL;
      else shadeRange = cycles;
    }

