/** dpiff.h ******************************************************************/
/*                                                                           */
/* DPAINT IFF file I/O procedures                                            */
/*                                                                           */
/*****************************************************************************/
#ifndef DPIFF_H
#define DPIFF_H

#include "iff\ilbm.h"

#define BAD_FIT  -9

extern IFFP IffErr();	/* returns an IFFP error code; see IFF.H */

/* FLAGS */
#define MBM_HAS_GRAB 1
#define MBM_HAS_PUSE 2
#define MBM_HAS_CMAP 4
#define MBM_HAS_RANGES 8

/* Range id */
#define ID_CRNG		MakeID('C','R','N','G')

typedef BOOL (*ResizeProc)();

typedef struct {
    LONG flags;
    struct BitMap *bitmap;
    SHORT w;			/* pixel width of bitmap */
    UBYTE xAspect, yAspect;
    Masking masking;
    UBYTE *mask;
    UBYTE xpcolor;
    Point2D pos;
    Point2D grab;
    Range *ranges;
    SHORT nRange;
    } MaskBM;
    
extern BOOL PutMaskBM(LONG, MaskBM *, WORD *, BYTE *, LONG);
/*		     file, maskBM, colorMap, buffer, bufsize */
 
extern BOOL GetMaskBM(LONG, MaskBM *, WORD *, ResizeProc,  BYTE *,LONG );
/*     file  maskBM  colorMap   reSize()  buffer  bufsize	*/
