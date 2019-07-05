/*----------------------------------------------------------------------*/
/*									*/
/*		BMRot90 -- Rotate Bitmap By 90 Degrees			*/
/*									*/
/*----------------------------------------------------------------------*/

#include "system.h"
#include "graphics\gfx.h"
#include <prism.h>

extern SHORT curpen;
extern BMOB curbr;


BOOL BMRot90(source,sw, destination)
    struct BitMap *source; /* pointer to source bitmap */
    int sw;		/* width of source bitmap data in  pixels*/
    struct BitMap  *destination;
	{
	register UBYTE src, *dst_row, dbit;
	UBYTE  *src_plane, *dst_plane, *src_row;
	int p, dx, dy, nx, ny;
	int sh,sbpr,dbpr;
	sbpr =  source->BytesPerRow;
	dbpr =  destination->BytesPerRow;
	sh = source->Rows;
	for (p = 0;  p < source->Depth;  p++)	{
	    src_plane = (UBYTE *) ( source->Planes[p] + sh*sbpr );
	    dst_plane = (UBYTE *) destination->Planes[p];
	    nx = 0;
	    dbit = 0x80;
	    if (CheckAbort()) return(FAIL);
	    for (dx = sh;  dx > 0 ;  dx--)	{
		src_plane -= sbpr;
		dst_row = dst_plane;
		src_row = src_plane;
		src = *src_row++;
		ny = 0;
		for (dy = 0;  dy < sw;  dy++) {
		    if ( src & 0x80) *dst_row |= dbit;
		    src <<= 1;
		    dst_row += dbpr;
		    if ( ++ny == 8 ) { ny = 0; src = *src_row++; }
		    }
		if (++nx == 8) { nx = 0; dst_plane++; dbit = 0x80; }
		else dbit >>= 1;
		}
	    }
	return(SUCCESS);
	}

/*----------------------------------------------------------------------*/
/*  90 Degree Rotation of brush 									*/
/*----------------------------------------------------------------------*/

void BrRot90() {
    SHORT sav;
    struct BitMap tmp;
    if (curpen!=USERBRUSH) return;
    ZZZCursor();
    ClearBrXform();	/* could be smarter than this, but.. */

    InitBitMap(&tmp,curbr.pict.bm->Depth,curbr.pict.bm->Rows,curbr.pict.box.w);
    if (AllocBitMap(&tmp)) return;
    ClearBitMap(&tmp);
    if (BMRot90(curbr.pict.bm, curbr.pict.box.w, &tmp)==SUCCESS) {
	DupBitMap(&tmp,curbr.pict.bm);
	sav = curbr.pict.box.w;
	curbr.pict.box.w = curbr.pict.box.h;
	curbr.pict.box.h = sav;
	sav = curbr.yoffs;
	curbr.yoffs = curbr.xoffs;
	curbr.xoffs = curbr.pict.box.w - 1 - sav;
	BMOBMask(&curbr);
	}
    else FreeBitMap(&tmp);
    UnZZZCursor();
    }
