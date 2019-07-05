/*----------------------------------------------------------------------*/
/*  									*/
/*        brxform -- Brush transformations				*/
/*  									*/
/*  	 BrFlipX, BrFlipY, BrRot90, BrRemapCols, BrBgToFg		*/
/*  									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>

#define local

extern UBYTE *BMAllocMask();
extern SHORT curxpc;
extern struct RastPort *curRP;
extern struct TmpRas tmpRas;
extern SHORT curpen;
extern BMOB curbr;

/*----------------------------------------------------------------------*/
/*	Maintain "Original" brush, before transforming			*/
/*----------------------------------------------------------------------*/
extern BMOB origBr;
extern struct BitMap origBM;
extern SHORT curBrXform;
    
/* copy the picture information of a BMOB */    
BOOL CopyBMOBPict(a,b) BMOB *a,*b; {
    BOOL res =  CopyBitMap(a->pict.bm,b->pict.bm);
    if (res) { /* dprintf(" CopyBMOBPict: alloc err \n");*/ return(FAIL);}
    b->pict.box = a->pict.box;
    DFree(b->mask);
    b->mask = BMAllocMask(curbr.pict.bm);
    if (b->mask==0) return(FAIL);
    movmem(a->mask,b->mask, BMPlaneSize(a->pict.bm));
    b->xoffs = a->xoffs;
    b->yoffs = a->yoffs;
    b->xpcolor = a->xpcolor;
    return(SUCCESS);
    }

/* save a copy of the brush preparatory to Transorming it */    
BOOL SaveBrush(newxf)  {
    BOOL res=SUCCESS;
    origBr.pict.bm = &origBM;
    FreeBitMap(curbr.save.bm);
    if (curBrXform != newxf) res = CopyBMOBPict(&curbr, &origBr);
    if (!res) curBrXform = newxf;
    else curBrXform = NOXFORM;
    return(res);
    }


/*----Restores the brush to its original state before transforming. */
/* this frees the storage for the transformed version */
void RestoreBrush() {
    if (curBrXform == NOXFORM) return;
    FreeBitMap(curbr.pict.bm);
    DFree(curbr.mask);
    curbr.mask = origBr.mask;
    origBr.mask = NULL;
    DupBitMap(origBr.pict.bm, curbr.pict.bm);
    setmem(origBr.pict.bm,sizeof(struct BitMap),0);
    curbr.pict.box = origBr.pict.box;
    curbr.xoffs = origBr.xoffs;
    curbr.yoffs = origBr.yoffs;
    curBrXform = NOXFORM;
    curbr.flags = 0;
    }


/*----------------------------------------------------------------------*/
/*  Map BG color to FG Color						*/
/*----------------------------------------------------------------------*/

void BrBgToFg() {
    if (curpen!=USERBRUSH) return;
    BMMapColor(curbr.pict.bm,curbr.pict.bm, curxpc, curRP->FgPen);
    BMOBMask(&curbr);
    }


/*----------------------------------------------------------------------*/
/*  Reverse brush in X - direction					*/
/*----------------------------------------------------------------------*/

FlipX(sbm,sw,dbm)  struct BitMap *sbm,*dbm; SHORT sw; {
    SHORT x,h;
    h = sbm->Rows;
    for (x = 0; x < sw; x++ ) 
	BltBitMap(sbm,x,0,dbm,sw-x-1,0,1,h,REPOP,0xff,tmpRas.RasPtr);   
    }
    
BOOL ObFlipX(ob) BMOB *ob; {
    struct BitMap tmpbm;
    BlankBitMap(&tmpbm);
    if (CopyBitMap(ob->pict.bm, &tmpbm)) return(FAIL);
    FlipX(&tmpbm,ob->pict.box.w, ob->pict.bm);
    ob->xoffs = ob->pict.box.w - ob->xoffs -1;
    BMOBMask(ob);
    FreeBitMap(&tmpbm);
    return(SUCCESS);
    }

void BrFlipX() {
    if (curpen!=USERBRUSH) return;
    ZZZCursor();
    if (ObFlipX(&curbr)) goto finish;
    if (curBrXform != NOXFORM) ObFlipX(&origBr);
    finish: UnZZZCursor();
    }

/*----------------------------------------------------------------------*/
/*  Reverse brush in Y - direction					*/
/*----------------------------------------------------------------------*/
FlipY(sbm,dbm)  struct BitMap *sbm,*dbm; {
    SHORT y,w,h;
    w = sbm->BytesPerRow*8;
    h = sbm->Rows;
    for (y = 0; y < h; y++ ) 
	BltBitMap(sbm,0,y,dbm,0,h-y-1,w,1,REPOP,0xff,tmpRas.RasPtr);   
    }


BOOL ObFlipY(ob) BMOB *ob; {
    struct BitMap tmpbm;
    BlankBitMap(&tmpbm);
    if (CopyBitMap(ob->pict.bm, &tmpbm)) return(FAIL);
    FlipY(&tmpbm, ob->pict.bm);
    ob->yoffs = ob->pict.box.h - ob->yoffs -1;
    BMOBMask(ob);
    FreeBitMap(&tmpbm);
    return(SUCCESS);
    }

void BrFlipY() {
    if (curpen!=USERBRUSH) return;
    ZZZCursor();
    if (ObFlipY(&curbr)) goto finish;
    if (curBrXform != NOXFORM) ObFlipY(&origBr);
    finish: UnZZZCursor();
    }


