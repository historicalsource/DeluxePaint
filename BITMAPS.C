/*----------------------------------------------------------------------*/
/*  									*/
/*     bitmaps.c  -- BitMap operations  (c) Electronic Arts 		*/
/*  									*/
/*   	6/22/85   Created	Dan Silva  				*/
/*   	7/11/85   Added MakeMask Dan Silva 				*/
/*   	8/1/85    Added prism functions Dan Silva			*/
/*----------------------------------------------------------------------*/
/* Note: all Bitmap allocating functions will first try to free		*/
/*  the planes of the BitMap. Initially all bm.Plane[] values should	*/
/*  be set to NULL. 							*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>
#define KPrintF printf
extern struct RastPort tempRP;
extern struct TmpRas tmpRas;
extern BOOL haveWBench;

BOOL dbgalloc = NO;
       	
prbm(bm)  struct BitMap *bm;  {
#ifdef DBGBM
    dprintf("BitMap: bpr=%ld, rows= %ld, deep = %ld \n",
	bm->BytesPerRow, bm->Rows, bm->Depth);
#endif
    }
    
static AllocErr(s) char *s; { 
#ifdef DBGBM
    if (dbgalloc) dprintf(" Alloc or Freeing error in %s\n",s);   
#endif
    }

#define MINAVAILMEM 13000

/* Allocate planes seperately-- wont work for BOBs */
/* This routine Does NOT free planes first */
TmpAllocBitMap(bm)  struct BitMap *bm;  {
    SHORT i;
    int psize = BMPlaneSize(bm);
    setmem(bm->Planes, 8*sizeof(LONG), 0);
    for (i=0; i<bm->Depth; i++) {
	bm->Planes[i]= (PLANEPTR)ChipAlloc(psize);
	if (bm->Planes[i]==NULL){
	    if (dbgalloc){   
#ifdef DBGBM
		dprintf(" AllocBitmap failure: \n");
		prbm(bm);
#endif
		}
	    FreeBitMap(bm);
	    return(FAIL); 
	    }
	}
    return(SUCCESS);
    }

AllocBitMap(bm)  struct BitMap *bm;  {
    BOOL res = TmpAllocBitMap(bm);
    if (res==FAIL) return(FAIL);
    if (AvailMem(MEMF_CHIP|MEMF_PUBLIC) < MINAVAILMEM) {
	FreeBitMap(bm);
	return(FAIL);
	}
    return(SUCCESS);
    }
    
UBYTE *BMAllocMask(bm) struct BitMap *bm; {
    return( (UBYTE *) ChipAlloc(bm->BytesPerRow * bm->Rows) );
    }

FreeBitMap(bm) struct BitMap *bm; {
    SHORT i;
    for (i=0; i<bm->Depth; i++)  if (bm->Planes[i] != NULL) {
	DFree(bm->Planes[i]);
    	bm->Planes[i] = NULL;
	}
    }
    
NewSizeBitMap(bm,deep,w,h)  struct BitMap *bm; SHORT deep,w,h;   {
    int ret;
    int bw = BytesNeeded(w);
    if ((bm->Planes[0] != NULL)&&(bm->Depth==deep)&&
	(bm->BytesPerRow==bw)&&(bm->Rows==h)) return(0);
    FreeBitMap(bm);
    bm->Depth = deep;
    bm->BytesPerRow = bw;
    bm->Rows = h;
    ret = AllocBitMap(bm);
    if (ret) AllocErr("NewSizeBitMap");
    return(ret);
    }

ClearBitMap(bm) struct BitMap *bm; {
    SHORT i;
    for (i=0; i<bm->Depth; i++) 	
	BltClear(bm->Planes[i],(bm->Rows<<16)+bm->BytesPerRow, 3);
    }

SetBitMap(bm,color) struct BitMap *bm; SHORT color; {
    tempRP.BitMap = bm;
    SetRast(&tempRP,color);
    }

/* USE WITH CAUTION: WILL OVERWRITE Planes[] POINTERS */
BlankBitMap(bm) struct BitMap *bm; {
    setmem(bm, sizeof(struct BitMap), 0);
    }

/* This assures that bitmap b is the same size as a */    
MakeEquivBM(a,b) struct BitMap *a,*b; {
    int ret;
    if (ret=NewSizeBitMap(b,a->Depth,a->BytesPerRow*8,a->Rows))
	AllocErr("MakeEquivBM");
    return(ret);
    }

FillBitMap(pb,v) struct BitMap *pb; int v; {
    SHORT i;
    for (i=0; i<pb->Depth; i++) setmem(pb->Planes[i],BMPlaneSize(pb),v);
    }

/* this results in two totally identical BitMap structures */
/* pointing at the same data */
DupBitMap(a,b) struct BitMap *a,*b; {
    FreeBitMap(b);
    movmem(a,b,sizeof(struct BitMap));
    }

/* this makes a copy of the data too */     
CopyBitMap(a,b) struct BitMap *a,*b; {
    if(MakeEquivBM(a,b)){ AllocErr("CopyBitMap"); return(FAIL); }
    WaitBlit();
    BltBitMap(a,0,0,b,0,0,a->BytesPerRow*8,a->Rows,REPOP,0xff,NULL);
    return(SUCCESS);
    }

/*--------------------------------------------------------------*/
/*								*/
/*			Make Mask				*/
/*								*/
/*--------------------------------------------------------------*/
/* 

This makes masks for a bitmap associated with a particular
"special" color.  It will either make a mask which has 1's in it ONLY 
where the bitmap has that special color (a POSITIVE mask) or it will 
make a mask which has 1's in it where the bitmap is NOT that special 
color ( a NEGATIVE mask ).

To make a positive mask for the special color 10110, 
where p<i> is the <i>th plane of the Bitmap: 

	mask  = p4 & (^p3) & p2 & p1 & (^p0);

To a negative mask mask for a Bitmap for a particular "transparent" 
color, you want a 1 where the color of the bitmap is NOT the 
transparent color, zero where it IS is the transparent color .
    
	e.g. if xpcol = 10110 then want
    
	mask = (^p4) | p3 | (^p2) | (^p1) | p0
    
---------------------------------------------------------------------**/
MakeMask(bm,mask,color,sign) 
    struct BitMap *bm;		/* bitmap for which mask is to be made*/
    UBYTE *mask;		/* holds the result: should be as big as
    						 one plane of bm */
    SHORT color;		/* special color	*/
    BOOL sign;			/* TRUE = positive, FALSE = negative */
    {
    SHORT depth,i,mt,bltSize;
    UBYTE op1,op2;
    depth = bm->Depth;
    bltSize = BlitSize(bm->BytesPerRow,bm->Rows);
    if (sign) { mt = REPOP; op1 = ANDOP;   op2 = NOTANDOP;  }
    else     { mt = NOTOP;  op1 = NOTOROP; op2 = OROP;  	}
    if (!(color&1)) mt ^= 0xff;
    for (i=0; i<depth; i++) {
	BltABCD(NULL, bm->Planes[i], mask, mask, bltSize, mt);
	color >>= 1;
	mt = (color&1) ? op1: op2; 
	}
    WaitBlit();
    }


#ifdef theOldWay
MakeMask(bm,mask,color,pos) 
    struct BitMap *bm;		/* bitmapr for which mask is to be made*/
    UBYTE *mask;		/* should be as big as one plane of bm */
    SHORT color;		/* special color	*/
    BOOL pos;			/* TRUE = positive, FALSE = negative */
    
    {
    struct BitMap mbm;
    SHORT depth,i,mt,w;
    PLANEPTR plane0;
    UBYTE op1,op2;
    mbm.BytesPerRow = bm->BytesPerRow;
    mbm.Rows = bm->Rows;    mbm.Depth = 1;    mbm.Flags = 0;
    mbm.Planes[0] = (PLANEPTR)mask;
    depth = bm->Depth;
    w = bm->BytesPerRow*8;
    if (pos) { 	SetBitMap(&mbm,0xff); 	op1 = ANDOP;   op2 = NOTANDOP;  }
    else     { 	ClearBitMap(&mbm); 	op1 = NOTOROP; op2 = OROP;  	}
    bm->Depth = 1;
    plane0 = bm->Planes[0];
    for (i=0; i<depth; i++) {
	bm->Planes[0] = bm->Planes[i];
	mt = (color&1) ? op1: op2; 
	WaitBlit();
	BltBitMap(bm,0,0,&mbm,0,0,w,mbm.Rows,mt,0x01,NULL);
	color >>= 1;
	}
    WaitBlit();
    bm->Depth = depth;
    bm->Planes[0] = plane0;
    }
#endif
/* Changes all pixels in a bitmap which are color number "acol" to
   be color number "bcol" */


BMMapColor(sbm,dbm,acol,bcol) struct BitMap *sbm, *dbm; SHORT acol,bcol; {
    BYTE *tmpmask = tmpRas.RasPtr;
    SHORT w = sbm->BytesPerRow*8;
    MakeMask(sbm,tmpmask,acol,POSITIVE);
    tempRP.BitMap = dbm;
    SetAPen(&tempRP,bcol); 
    SetDrMd(&tempRP,JAM1);
    BltTemplate(tmpmask, 0, sbm->BytesPerRow, &tempRP, 0,0,w,sbm->Rows); 
    }

    

