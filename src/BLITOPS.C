/*----------------------------------------------------------------------*/
/*									*/
/*	Blitops.c	(c) Electronic Arts	1985			*/
/*									*/
/*									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>

#undef sx
#undef sy

#define local  static

USHORT masks[17] = {
	0xffff,0x7fff,0x3fff,0x1fff,
	0x0fff,0x07ff,0x03ff,0x01ff,
	0x00ff,0x007f,0x003f,0x001f,
	0x000f,0x0007,0x0003,0x0001,0000
	};

BlitterRegs *ar = (BlitterRegs *)0xDFF040;

#define yA  	0xf0
#define yB	0xcc
#define yC	0xaa
#define nA	0x0f
#define nB 	0x33
#define nC	0x55

#define USEA 0x0800
#define USEB 0x0400
#define USEC 0x0200
#define USED 0x0100
#define MASKWRITE  	0x0F00  /* USEA|USEB|USEC|USED */
#define CONSTWRITE	0x0B00	/* USEA|USEC|USED */
    
#define COPYA 0xf0	/* minterm = A */

#define WordOffs(x)     (((x)>>3)&0xfffe)

extern UWORD *curFilPat;
    
/*----------------------------------------------------------------------*/
/* HLineBlt: fill a horizontal span of pixels with the current area	*/
/* fill pattern.							*/
/* NOTE: this should be changed to use rp's minterms			*/
/*----------------------------------------------------------------------*/
HLineBlt(rp,lx,y,rx,mint) struct RastPort *rp; int lx,y,rx;SHORT mint; {
    int bytes,lwrd,i;
    LONG offset;
    SHORT blsize,apen;
    UWORD *patadr;
    struct BitMap *dbm = rp->BitMap;

    lwrd = WordOffs(lx);
    bytes = WordOffs(rx)- lwrd + 2;
    offset = y*dbm->BytesPerRow + lwrd;
    blsize = (1<<6)|(bytes>>1);
    WaitBlit();
    OwnBlitter(); 
    ar->fwmask = masks[lx&15];
    ar->lwmask = ~masks[(rx&15)+1]; 
    ar->bltcon0 = USEC|USED|mint;  
    ar->bltcon1 = 0;
    ar->adata = 0xffff;
    ar->bltmdd = ar->bltmdc = dbm->BytesPerRow - bytes;
    if (curFilPat) {
	patadr = curFilPat + (y&PATMOD);
	for (i = 0; i< dbm->Depth; i++) {
	    WaitBlit();
	    ar->bdata = curFilPat?*patadr: 0xFFFF;
	    patadr += PATHIGH;
	    ar->bltptd = ar->bltptc = dbm->Planes[i] + offset;
	    ar->bltsize = blsize;  /* start the blit */
	    }
	}
    else { /* no fill pattern: solid color */
	apen = rp->FgPen;
	for (i = 0; i< dbm->Depth; i++) {
	    WaitBlit();
	    ar->bdata = (apen&1)?0xffff: 0;
	    ar->bltptd = ar->bltptc = dbm->Planes[i] + offset;
	    ar->bltsize = blsize;  /* start the blit */
	    apen >>= 1;
	    }
	}
    WaitBlit();
    DisownBlitter(); 
    }


#ifdef oldversion    
/*----------------------------------------------------------------------*/
/* HLineBlt: fill a horizontal span of pixels with the current area	*/
/* fill pattern.							*/
/* NOTE: this should be changed to use rp's minterms			*/
/*----------------------------------------------------------------------*/
HLineBlt(rp,lx,y,rx,mint) struct RastPort *rp; int lx,y,rx;SHORT mint; {
    int bytes,lwrd,i;
    LONG offset;
    SHORT pattern,blsize,bit,apen,bpen,wrd;
    struct BitMap *dbm = rp->BitMap;

    lwrd = WordOffs(lx);
    bytes = WordOffs(rx)- lwrd + 2;
    offset = y*dbm->BytesPerRow + lwrd;
    if (rp->AreaPtrn != NULL)
	pattern = rp->AreaPtrn[y&((1<<rp->AreaPtSz)-1)];
    else pattern = 0xffff;
    apen = rp->FgPen;
    bpen = rp->BgPen;
    blsize = (1<<6)|(bytes>>1);
    WaitBlit();
    OwnBlitter(); 
    ar->fwmask = masks[lx&15];
    ar->lwmask = ~masks[(rx&15)+1]; 
    ar->bltcon0 = USEC|USED|mint;  
    ar->bltcon1 = 0;
    ar->adata = 0xffff;
    ar->bltmdd = ar->bltmdc = dbm->BytesPerRow - bytes;
    bit = 1;
    for (i = 0; i< dbm->Depth; i++) {
	wrd = 0;
	if (bit&apen) wrd = pattern;
	if (bit&bpen) wrd |= (~pattern);
	WaitBlit();
	ar->bdata = wrd;
	ar->bltptd = ar->bltptc = dbm->Planes[i] + offset;
	ar->bltsize = blsize;  /* start the blit */
	bit <<= 1;
	}
    WaitBlit();
    DisownBlitter(); 
    }
#endif

BltABCD(a, b, c, d, bltSize, minterm) 
    UBYTE *a,*c,*b,*d;
    SHORT bltSize;
    UBYTE minterm;
    {
    SHORT con0;
    con0 = (a?USEA:0)|(b?USEB:0)|(c?USEC:0)|USED|(minterm&0xff);
    WaitBlit();
    OwnBlitter(); 
    ar->fwmask = 0xffff;
    ar->lwmask = 0xffff; 
    ar->bltcon0 = con0;
    ar->bltcon1 = 0;
    ar->bltmda = ar->bltmdb = ar->bltmdc = ar->bltmdd = 0;
    ar->bltpta = a;
    ar->bltptb = b;
    ar->bltptc = c;
    ar->bltptd = d;
    ar->bltsize = bltSize;  /* start the hardware blit */
    WaitBlit();
    WaitBlit();
    DisownBlitter(); 
    }

#define A_AND_B   yA&yB

/* sum bit is on if odd number of sources are 1 */
#define ADD_ABC	(yA&nB&nC)|(nA&yB&nC)|(nA&nB&yC)|(yA&yB&yC)
	
/* carry if at least 2 of three sources are 1's */
#define CARRY_ABC (yA&yB&nC)|(yA&nB&yC)|(nA&yB&yC)|(yA&yB&yC)
#define POST_CARRY_ABC  0xB2	/* black magic.. trust me */



#ifdef usingAddBM
/*  b += a  */
AddBM(a,b,car)  
    struct BitMap *a, *b;
    UBYTE *car;	/* one plane scratch area( carry bit) */
    {
    int i;
    int bpr = a->BytesPerRow;
    int h = a->Rows;
    int depth = a->Depth;
    int bsz = BlitSize(bpr,h);
    /* carry = 0 */
    BltClear(car,(a->Rows<<16)+a->BytesPerRow, 3);
    for (i=0; i<depth; i++) {
	/* --- add i'th bit and carry, store as new b[i] */
	 BltABCD(a->Planes[i], b->Planes[i], car, b->Planes[i], bsz, ADD_ABC );
	/* --- compute new carry bit */
	 BltABCD(a->Planes[i], b->Planes[i], car, car, bsz, POST_CARRY_ABC);
	}
    }
#endif


