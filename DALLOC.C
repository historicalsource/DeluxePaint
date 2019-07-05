/*--------------------------------------------------------------*/
/*								*/
/*		dalloc.c  					*/
/*								*/
/*	lets you specify flags but remembers the length for	*/
/*	freeing.						*/
/*--------------------------------------------------------------*/

#include <exec\types.h>
#include <exec\nodes.h>
#include <exec\memory.h>

/* first 4 bytes of allocated area contain the length of the area.
This length is the number of bytes allocated by AllocMem, ie includes
the 4 bytes for itself */
    
/* size in bytes */
UBYTE *DAlloc(size,flags) LONG size; USHORT flags; {
    LONG *p;
    LONG asize = size+4;
    p = (LONG *)AllocMem(asize,flags);
    if (p==NULL){
	return((UBYTE *)p);
	}
    *p++ = asize;	/* post-bump p to point at clients area*/
    return((UBYTE *)p);
    }
    
UBYTE *ChipAlloc(size) LONG size; {
    return(DAlloc(size,MEMF_PUBLIC|MEMF_CHIP));
    }
    
DFree(p) UBYTE *p; {
    if (p!=NULL) {
	p -= 4;
	return(FreeMem(p, *((LONG *)p))); 
	}
    }
