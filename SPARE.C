/*----------------------------------------------------------------------*/
/*									*/
/*		spare.c  - Spare screen					*/
/*									*/
/*----------------------------------------------------------------------*/

#include "system.h"
#include "prism.h"

#define local static

extern struct BitMap hidbm;
extern struct ViewPort *vport;
extern SHORT nColors, curxpc;

extern struct RastPort *curRP;
extern MagContext magCntxt;
extern struct TmpRas tmpRas;
extern Box screenBox, clipBox;
extern SHORT curDepth;


extern BOOL spareThere;
extern BOOL newSpare;
extern SHORT spareXPCol;
extern struct BitMap sparebm;
extern SHORT didType;

local void NotEnough() {
    UndoSave();
    InfoMessage("Insufficient Memory","for Spare Screen");
    }
    
local BOOL AllocSpare() {
    LONG freeMem,imSize;
    if (!spareThere) {
	freeMem = AvailMem(MEMF_CHIP|MEMF_PUBLIC);
	imSize = hidbm.BytesPerRow*hidbm.Rows*hidbm.Depth;
	if (freeMem < (2*imSize)) { NotEnough(); return((BOOL)FAIL); }
	InitBitMap(&sparebm,curDepth,screenBox.w,screenBox.h);
	if (AllocBitMap(&sparebm)) { NotEnough(); return((BOOL)FAIL);} 
	SetBitMap(&sparebm,0);
	spareThere = YES;
	}
    return(SUCCESS);
    }

void SwapSpare() {
    struct BitMap mytemp;
    SHORT tmp;
    if (AllocSpare()) return;
    UndoSave();
    KillCCyc();
    mytemp = hidbm;
    hidbm = sparebm;
    sparebm = mytemp;
    tmp = spareXPCol;
    spareXPCol = curxpc;
    if (!newSpare) { CPChgBg(tmp); }
    newSpare = NO;
    UpdtDisplay();
    }

void CopyToSpare() {
    if (AllocSpare()) return;
    UndoSave();
    CopyBitMap(&hidbm,&sparebm);
    spareXPCol = curxpc;
    newSpare = NO;
    }
           

extern SHORT xorg,yorg;
    
local BOOL inFront;

local MakeMrgMask() {
    if (inFront) 
	MakeMask(&sparebm, tmpRas.RasPtr, spareXPCol, NEGATIVE);
    else  MakeMask(&hidbm, tmpRas.RasPtr, curxpc, POSITIVE);
    }
	
void MergeSpare() {
    BYTE *mask = tmpRas.RasPtr;
    Point spos,dpos;
    if (spareThere) {
	UndoSave();
	spos.x = xorg;	spos.y = yorg;
	dpos.x = dpos.y = 0;
	MakeMrgMask();
	MaskBlit(&sparebm, &spos, mask, curRP->BitMap,&dpos, &screenBox,
	    &clipBox, COOKIEOP, mask, 0xff, 0);
	MagUpdate(&screenBox);
	didType = DIDMerge;
	}
    }

void MrgSpareFront() { inFront = YES; MergeSpare(); }
void MrgSpareBack() { inFront = NO; MergeSpare(); }
    
MrgSpareUpdt() {
    Point pos;
    pos.x = pos.y = 0;
    MakeMrgMask();
    MaskBlit(&sparebm, &pos, tmpRas.RasPtr, &hidbm, &pos, &screenBox,
	&screenBox, COOKIEOP, tmpRas.RasPtr, 0xff, 0);
    }

