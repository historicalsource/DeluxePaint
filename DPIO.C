/*----------------------------------------------------------------------*/
/*									*/
/*		dpio.c -- Picture and Brush IO				*/
/*									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>
#include "librarie\dos.h" /* for file io */
#include <dpiff.h>
#include <fnreq.h>

extern struct Window *mainW;
extern SHORT nColors;
extern struct TmpRas tmpRas;
extern SHORT  xShft, yShft, nColors;
extern struct ViewPort *vport;
extern BMOB curbr;
extern struct BitMap hidbm;
extern Box screenBox;
extern BOOL skipRefresh;
extern SHORT curFormat,curxpc,curpen,curDepth;
extern SHORT LoadBrColors[], prevColors[];
extern Range cycles[];
    
UBYTE loadStr[] = "Load";
UBYTE saveStr[] = "Save";

#define local static


BOOL CombPathName(comb,path,finame) UBYTE *comb,*path,*finame; {
    if (strlen(finame) <= 0) return(NO);
    strcpy(comb,path);
    ConcatPath(comb,finame);
    return(YES);
    }

BOOL DoFnReq(title,doname,path,name) UBYTE *title,*doname,*path,*name; {
    BOOL res;
    UndoSave();
    FreeTmpRas(); /* make room for Intuition to save screen under req*/
    res = (BOOL)FN_Request(mainW,PMapX(20),PMapY(24),
				path,name,NULL,title,NULL, doname);
    AllocTmpRas();
    magRefresh();
    mainRefresh();
    return(res);
    }

/*----------------------------------------------------------------------*/
/*  Picture								*/
/*----------------------------------------------------------------------*/
extern UBYTE picpath[31];
extern UBYTE picname[31];
    
ZapPicName() { picname[0] = 0; }

BOOL MakePicName(nm) UBYTE nm[]; {return(CombPathName(nm,picpath, picname));}

BOOL DoPicFnReq(title,doname) UBYTE *title, *doname;{
    return(DoFnReq(title,doname,picpath,picname)); 
    }

    
/* ILBM IO */

/* try to resize the pic to fit the one being read in */
local BOOL badFit;
BOOL ResizePic(mbm,bmhdr) MaskBM *mbm; BitMapHeader *bmhdr; {
    if ((bmhdr->h == hidbm.Rows)&&(bmhdr->w == hidbm.BytesPerRow*8))
	{
	if (bmhdr->nPlanes< hidbm.Depth) ClearBitMap(&hidbm);
	return(SUCCESS);
	}
    else return(FAIL);
    }

NoOpen(name) char *name; { InfoMessage(" Couldn't Open file:",name); }
    
NoSave(name) char *name; { InfoMessage(" Couldn't Save to file:",name); }
    
LoadPicNamed(name) char *name; {
    LONG file;
    MaskBM mbm;
    WORD colors[32];
    file = Open(name, MODE_OLDFILE);
    if (file==0) { NoOpen(name); picname[0]=0; return(FAIL);}
    ZZZCursor();
    GetColors(colors); /* default values if fewer planes loaded */
    mbm.flags = MBM_HAS_CMAP;
    mbm.bitmap = &hidbm;
    mbm.ranges = cycles;
    mbm.nRange = MAXNCYCS;
    badFit = NO;
    if (SUCCESS == GetMaskBM(file, &mbm, colors,
			 &ResizePic,tmpRas.RasPtr, tmpRas.Size)) {
	KillCCyc();
	GetColors(prevColors);
	LoadRGB4(vport,colors,nColors);
	}
    else {
	if (IffErr()==BAD_FIT) InfoMessage(" Wrong format:", name);
	else InfoMessage(" Couldn't load file",name);
	ZapPicName();
	}
    Close(file);
    UnZZZCursor();
    }

local BYTE xAspRat[3] = {x320x200Aspect,x640x200Aspect,x640x400Aspect};
local BYTE yAspRat[3] = {y320x200Aspect,y640x200Aspect,y640x400Aspect};

BOOL SavePicNamed(name) char *name; {
    LONG file;
    WORD colors[32];
    BOOL res;
    MaskBM mbm;
    GetColors(colors);
    ZZZCursor();
    file = Open(name, MODE_NEWFILE);
    if (file==0) { NoSave(name); return(FAIL); }
    mbm.flags = MBM_HAS_CMAP|MBM_HAS_RANGES;
    mbm.nRange = MAXNCYCS;
    mbm.ranges = cycles;
    mbm.bitmap = &hidbm;
    mbm.w = hidbm.BytesPerRow*8;
    mbm.mask = NULL;
    mbm.masking = mskHasTransparentColor;
    mbm.xpcolor = curxpc;
    mbm.pos.x = mbm.pos.y = 0;
    res  = PutMaskBM(file, &mbm, colors, (UBYTE *)tmpRas.RasPtr, tmpRas.Size);
    if (res) {
	InfoMessage(" Couldn't save file",name);
	ZapPicName();
	}
    else WriteProjectIcon(picpath,picname);
    Close(file);
    UnZZZCursor();
    return(res);
    }

ChkFNRes(res) BOOL res; {
    if (res== FN_NoSuchDrawer) {
	InfoMessage(" Non existent drawer","");
	}
    }

#ifdef DOWB
LoadPic() {
    UBYTE nm[60];
    OffRefrAfterOv();
    if (MakePicName(nm)) {
	LoadPicNamed(nm);
	Str(picname);
	}
    OnRefrAfterOv();
    PaneRefrAll();
    }
#endif
    
OpenPic() {  UBYTE nm[60];
    BOOL res;
    InitRead(); /* bring in the reader overlay*/
    res  = DoPicFnReq("Load Picture",loadStr);
    if (res == FN_OK) {
	if (MakePicName(nm)) {
	    LoadPicNamed(nm);
	    StuffSaveStr(picname);
	    }
	}
    else ChkFNRes(res);
    PaneRefrAll();
    }

local UBYTE bkdotpic[] = "backup.pic";

extern BOOL largeMemory;
    

void SaveCurPic() {
    UBYTE nm[60],bkupname[60];  
    LONG lock;
    LONG err;
    if (MakePicName(nm)) {
	lock = Lock(nm, ACCESS_WRITE);
	if (lock) {
	    CombPathName(bkupname,picpath,bkdotpic);
	    UnLock(lock);
	    if (DeleteFile(bkupname) == 0) {
		err = IoErr();
		if (err != ERROR_OBJECT_NOT_FOUND)    return;
		}
	    if (Rename(nm,bkupname) == 0) return;
	    }
	SavePicNamed(nm);   
	StuffSaveStr(picname);
	}
    else InfoMessage("Invalid File name", "");
    }

SavePicAs() {
    BOOL res;
    InitWrite(); /* bring in the writer overlay*/
    res = DoPicFnReq("Save Picture", saveStr);
    if (res == FN_OK)  SaveCurPic();
    else ChkFNRes(res);
    PaneRefrAll();
    }

SavePic() {  
    InitWrite(); /* bring in the writer overlay*/
    UndoSave();
	if (strlen(picname) == 0) SavePicAs();
    else {
	if (!largeMemory) InfoMessage(" Insert data disk","in disk drive");
	SaveCurPic();
	}
    PaneRefrAll();
    }

/*----------------------------------------------------------------------*/
/*  Brush IO								*/
/*----------------------------------------------------------------------*/
extern UBYTE brspath[31];
extern UBYTE brsname[31];

ZapBrushName() { brsname[0] = 0; }
BOOL MakeBrsName(nm) UBYTE nm[]; {return(CombPathName(nm,brspath, brsname));}
BOOL DoBrsFnReq(title,doname) UBYTE *title,*doname;{return(DoFnReq(title,doname,brspath,
	brsname));}


SaveBrsNamed(name) char *name; {
    LONG file; 
    BOOL res;
    WORD colors[32];
    MaskBM mbm;
    GetColors(colors);
    ZZZCursor();
    file = Open(name, MODE_NEWFILE);
    if (file==0) { NoSave(name); return(FAIL); }
    mbm.flags = MBM_HAS_CMAP|MBM_HAS_GRAB;
    mbm.bitmap = curbr.pict.bm;
    mbm.w = curbr.pict.box.w;
    mbm.masking = mskHasTransparentColor;
    mbm.mask = NULL;
    mbm.xpcolor = curbr.xpcolor;
    mbm.pos.x = mbm.pos.y = 0;
    mbm.grab.x = curbr.xoffs;
    mbm.grab.y = curbr.yoffs;
    res = PutMaskBM(file, &mbm, colors,(UBYTE *)tmpRas.RasPtr,tmpRas.Size);
    if (res) { brsname[0] = 0; InfoMessage(" Couldn't save file",name);}
    else WriteBrushIcon(brspath,brsname);
    Close(file);
    UnZZZCursor();
    }

/* try to resize the brush to fit the one being read in */

local BOOL cantFit;

#ifdef SIMPLEWAY
BOOL ResizeBrush(mbm,bmhdr) MaskBM *mbm; BitMapHeader *bmhdr; {
    if (BMOBNewSize(&curbr, bmhdr->nPlanes, bmhdr->w,bmhdr->h)) {
	cantFit = YES;
	return(FAIL);
	}
    ClearBitMap(curbr.pict.bm);
    mbm->mask = curbr.mask;
    return(SUCCESS);
    }

#else
BOOL ResizeBrush(mbm,bmhdr) MaskBM *mbm; BitMapHeader *bmhdr; {
    SHORT depth = bmhdr->nPlanes;
    do  {if (!BMOBNewSize(&curbr, depth, bmhdr->w, bmhdr->h)) 
	   goto alldone;
	} while ((--depth)>=curDepth);
    cantFit = YES;
    return(FAIL);
    alldone:
	ClearBitMap(curbr.pict.bm);
	mbm->mask = curbr.mask;
	return(SUCCESS);
    }
#endif

LoadBrsNamed(name) char *name; {
    LONG file;
    MaskBM mbm;
    file = Open(name, MODE_OLDFILE);
    if (file==0) { NoOpen(name); brsname[0] = 0; return(FAIL); }
    ZZZCursor();
    mbm.flags = 0;
    mbm.bitmap = curbr.pict.bm;
    mbm.mask = curbr.mask;
    GetColors(LoadBrColors); /* default values if fewer planes loaded */
    RestoreBrush();
    cantFit = NO;
    if (SUCCESS == GetMaskBM(file, &mbm, LoadBrColors,
			 &ResizeBrush,tmpRas.RasPtr, tmpRas.Size))
	{
	if (mbm.flags&MBM_HAS_GRAB) {
	    curbr.xoffs = mbm.grab.x;
	    curbr.yoffs = mbm.grab.y; 
	    }
	else {
	    curbr.xoffs = curbr.pict.bm->BytesPerRow*4;
	    curbr.yoffs = curbr.pict.bm->Rows/2;
	    }
	curbr.xpcolor = mbm.xpcolor;
	if (mbm.masking == (UBYTE)mskHasTransparentColor) BMOBMask(&curbr);
	UserBr();
	}
    else { 
	InfoMessage(" Couldn't load brush",name); 
	if (cantFit) {
	    SelPen(0); 
	    SetNextIMode(IM_draw); 
	    }
	else ZapBrushName(); 
	}
    Close(file);
    UnZZZCursor();
    }
    
GetBrs() { BOOL res;
    UBYTE nm[60];
    InitRead(); /* bring in the reader overlay*/
    res = DoBrsFnReq("Load Brush",loadStr);
    if (res==FN_OK) {if (MakeBrsName(nm)) LoadBrsNamed(nm);}
    else ChkFNRes(res);
    PaneRefrAll();
    }

void SaveBrsAs() {  UBYTE nm[60];
    BOOL res;
    InitWrite(); /* bring in the writer overlay*/
    if (curpen!=USERBRUSH) {
	InfoMessage(" Cant save Built-in", "   brushes ");
	}
    else {
	res = DoBrsFnReq("Save Brush", saveStr);
	if (res == FN_OK) {if (MakeBrsName(nm)) SaveBrsNamed(nm);}
	else ChkFNRes(res);
	}
     PaneRefrAll();
    }


