/*----------------------------------------------------------------------*/
/* packer.c Convert data to "cmpByteRun1" run compression.     11/06/85 */
/*									*/
/* By Jerry Morrison and Steve Shaw, Electronic Arts.			*/
/* This software is in the public domain.				*/
/*									*/
/*	control bytes:							*/
/*	 [0..127]   : followed by n+1 bytes of data.			*/
/*	 [-1..-127] : followed by byte to be repeated (-n)+1 times	*/
/*	 -128       : NOOP.            					*/
/*----------------------------------------------------------------------*/
#include "iff\packer.h"

#define DUMP	0
#define RUN	1

#define MinRun 3	
#define MaxRun 128
#define MaxDat 128

LONG putSize;
#define GetByte()	(*source++)
#define PutByte(c)	{ *dest++ = (c);   ++putSize; }

char buf[256];	/* [TBD] should be 128?  on stack?*/

BYTE *PutDump(dest, nn)  BYTE *dest;  int nn; {
	PutByte(nn-1);
	CopyMem(buf,dest,nn);
	putSize += nn;
	return(dest += nn);
	}

BYTE *PutRun(dest, nn, cc)   BYTE *dest;  int nn, cc; {
	PutByte(-(nn-1));
	PutByte(cc);
	return(dest);
	}


#define OutDump(nn)   dest = PutDump(dest, nn)
#define OutRun(nn,cc) dest = PutRun(dest, nn, cc)

/*----------- PackRow --------------------------------------------------*/
/* Given POINTERS TO POINTERS, packs one row, updating the source and
   destination pointers.  RETURNs count of packed bytes.*/
LONG PackRow(pSource, pDest, rowSize)
    BYTE **pSource, **pDest;   LONG rowSize; {
    register BYTE *source, *dest;
    register char c,lastc = '\0';
    BOOL mode = DUMP;
    register short nbuf = 0;		/* number of chars in buffer */
    register short rstart = 0;		/* buffer index current run starts */

    source = *pSource;
    dest = *pDest;
    putSize = 0;
    buf[0] = lastc = c = GetByte();  /* so have valid lastc */
    nbuf = 1;   rowSize--;	/* since one byte eaten.*/

    for (;  rowSize;  --rowSize) {
	buf[nbuf++] = c = GetByte();
	if (mode) { /* mode == RUN */
	     if ( (c != lastc)|| ( nbuf-rstart > MaxRun)) {
		/* output run */
		/* OutRun(nbuf-1-rstart,lastc) */
		PutByte(rstart-nbuf+2);
		PutByte(lastc);
		buf[0] = c;
		nbuf = 1; rstart = 0;
		mode = DUMP;
		}
	    }
	else { /* mode == DUMP */
	    /* If the buffer is full, write the length byte,
		then the data */
	    if (nbuf>MaxDat) {
		OutDump(nbuf-1);  
		buf[0] = c; 
		nbuf = 1;   rstart = 0; 
		}
	    else {
		if (c == lastc) {
		    if (nbuf-rstart >= MinRun) {
			if (rstart > 0) { /* OutDump(rstart) */
			    PutByte(rstart-1);
			    CopyMem(buf,dest,rstart);
			    putSize += rstart;
			    dest += rstart;
			    }
			mode = RUN;
			}
		    else if (rstart == 0)  mode = RUN; /* no dump in progress,
			so can't lose by making these 2 a run.*/
		    }
		else  rstart = nbuf-1;		/* first of run */ 
		}
	    }

	lastc = c;
	}

    switch (mode) {
	case DUMP: OutDump(nbuf); break;
	case RUN: OutRun(nbuf-rstart,lastc); break;
	}
    *pSource = source;
    *pDest = dest;
    return(putSize);
    }

