/*----------------------------------------------------------------------*/
/*  									*/
/*  ReMap Brush colors to better fit current color palette		*/
/*  									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>

extern SHORT nColors, curpen, curDepth;
extern struct RastPort tempRP;
extern BMOB curbr;
extern short LoadBrColors[];

/* A simple minded distance measure */
#ifdef linearsum
SHORT cdist(a,b) SHORT a,b; { 
    int i;
    SHORT sum = 0;
    for (i=0; i<3; i++) {
	sum += ABS((a&0xf) - (b&0xf));
	a>>=4;	b>>=4;
	}
    return(sum);
    }

#else

/* Another distance measure: sum of square R G B deltas */
SHORT cdist(a,b)  SHORT a,b;{ 
    SHORT t, sum = 0;
    int i;
    for (i=0; i<3; i++) {
	t = (a&0xf) - (b&0xf);
	sum += t*t;
	a>>=4;	b>>=4;
	}
    return(sum);
    }

#endif
	
SHORT dbgcmap = 1;


/* This maps bitmap colors to a good fit in the current color palette.
    if the brush has more planes than the current format, it will be 
    reduced  to the correct number of planes 
    
    The algorithm is: first sort the colors in the source bitmap by
    number of occurences in the bitmap.  Then, starting with the most 
    commonly occuring color, find the closed color in the current 
    palette and assign it to that color, eliminating it from further 
    use. Repeat this for the sorted list of source colors, until all are
    assigned.  If mapping down to fewer planes, use this algorithm 
    until all the colors in the color palette are used up, then 
    continue but no thenceforth let ALL the colors in the color 
    pallet be candidates for the closest match.*/
    
/* returns new transparent color */    
SHORT BMRemapCols(sbm,oldpal,oldxpc, newpal, newDepth)
    struct BitMap *sbm;
    SHORT *oldpal, oldxpc, *newpal, newDepth;
    {
    int x,y,i,j,k, jmax;
    SHORT col,cbit,mindist,minj,maxv,d;
    SHORT newxpc = -1;
    SHORT sort[32], nSrcCols, nDstCols, w,h;
    LONG histog[32];
    struct BitMap tmpbm;
    ULONG used = 0;

    if (curpen!=USERBRUSH) return(-1);

    ZZZCursor();
    w = sbm->BytesPerRow*8;
    h = sbm->Rows;

    /* count number of instances of each color in brush*/
    tempRP.BitMap = sbm;
    setmem(histog,32*sizeof(LONG),0);
    nSrcCols = (1<<sbm->Depth);
    nDstCols = (1<<newDepth);
    for (x = 0; x < w; x++) 
	for (y=0; y < h; y++) histog[ReadPixel(&tempRP,x,y)]++;

#ifdef DBGCM
    if (dbgcmap) {
	dprintf("Histogram: \n");
	for (i=0; i<nSrcCols; i++) dprintf(" %ld,",histog[i]);
	dprintf("\n");
	}
#endif    
    /* sort (dumbly) the colors in order of decreasing incidence*/

    for (i=0; i<nSrcCols; i++)  {
	maxv = -1;
	for (j=0;j<nSrcCols; j++) 
	    if (histog[j] > maxv) { jmax = j; maxv = histog[j]; }
	if (maxv<=0) { sort[i] = -1; break; }
	sort[i] = jmax;
	histog[jmax] = -1;
	}

#ifdef DBGCM
    if (dbgcmap) {
	dprintf("Sort: \n");
	for (i=0; i<nSrcCols; i++) dprintf(" %ld,",sort[i]);
	dprintf("\n");
	}
#endif    

    /* now work from most common colors, finding the closest match
      in current color map */
    
    BlankBitMap(&tmpbm); /* zero data structure out */
    
    if (CopyBitMap(sbm, &tmpbm)) goto finish;
    
    /* change to current depth */
    if (NewSizeBitMap(sbm, newDepth, w, h)) goto finish;
    
    for (k=0; k<nSrcCols; k++) {
	i = sort[k];
	if (i<0) break;
	col = oldpal[i];
	cbit = 1;
	mindist = 30000;
	minj = -1;

	/*---search for the best remaining match in current pallet */

	/* After all of the palette colors are used up, then make them
	  all available for further selection */
	if (k >= nDstCols) used = 0;
	for(j=0; j<nDstCols; j++) {
	    if ((used&cbit)==0) {
		d = cdist(col,newpal[j]);
		if (d<mindist) { minj = j; mindist = d;  }
		}
	    cbit <<= 1;
	    }

#ifdef DBGCM
	if (dbgcmap) dprintf("Changing %ld to %ld \n", i, minj);
#endif

	BMMapColor(&tmpbm, sbm, i, minj);
	if (i== oldxpc) newxpc = minj; 
	used |= (1<<minj);
	}
    FreeBitMap(&tmpbm);
    
    finish: UnZZZCursor();
    return(newxpc);
    }

    
void BrRemapCols()
    {
    SHORT xpc, colors[32];
    GetColors(colors);
    xpc = BMRemapCols(curbr.pict.bm, LoadBrColors, curbr.xpcolor, colors, curDepth);
    if (xpc == -1) return;
    curbr.xpcolor = xpc;
    BMOBMask(&curbr); 
    /* set the brush colors to the current palette */
    GetColors(LoadBrColors);
    }

