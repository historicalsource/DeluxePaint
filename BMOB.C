/*--------------------------------------------------------------*/
/*								*/
/*	bmob.c   Movable, Sizable Bitmap Object			*/
/*								*/
/*	8-14-85	 created	Dan Silva			*/
/*	8-15-85	 BoxBM version	Dan Silva			*/
/*--------------------------------------------------------------*/
#include <system.h>
#include <prism.h>

#define local static

extern UBYTE *BMAllocMask();

extern BOOL Painting;
extern BoxBM canvas;
extern Box clipBox;
extern struct RastPort *curRP, tempRP;
extern SHORT curFormat, curxpc, xShft, yShft, curDepth;
extern SHORT curpen;
extern Range *shadeRange;
extern struct TmpRas tmpRas;

extern SHORT dbgbmob;
extern struct BitMap tmpbm;
extern BoxBM temp;
extern struct BitMap tmpbm2;
extern BoxBM temp2;

/*--------------------------------------------------------------*/
/*	Debug printout stuff					*/
#ifdef DebUG  

prlf() { dprintf("\n"); }
prBox(b) Box *b; {
    dprintf("  Box :(%ld, %ld, %ld, %ld)\n", b->x, b->y, b->w, b->h);
    }
prBoxBM(bbm) BoxBM *bbm;
    { dprintf("BoxBM:"); prBox(&bbm->box); prbm(bbm->bm);    }

XBltBitMap(sbm,sx,sy,dbm,dx,dy,w,h,mint,mask,tmp)
    struct BitMap *sbm,*dbm; SHORT sx,sy,dx,dy,w,h;
    UBYTE mint,mask, *tmp; {
/*    if (dbgbmob) */
    dprintf("BltBitMap, src xy = %ld,%ld, dst xy = %ld, %ld, w,h=%ld,%ld\n",sx,sy,dx,dy,w,h);
    BltBitMap(sbm,sx,sy,dbm,dx,dy,w,h,mint,mask,tmp);
    }

#endif 

/**
dbgbm(bm) struct BitMap *bm; {
    if (dbgbmob) 
    BltBitMap(bm,0,0,canvas.bm,0,0,bm->BytesPerRow*8,bm->Rows,REPOP,0xff,NULL);
    }
**/

/*--------------------------------------------------------------*/
/* all the boxes are relative to the same coordinate system 	*/
CopyBoxBM(src,dst,mb,clip,mint)  
    BoxBM *src; /* source */
    BoxBM *dst;	/* destination */
    Box *mb;  	/* box to be moved */
    Box *clip; 	/* clipping box */
    SHORT mint;	/* minterm */
    {
    Box cb;
    BOOL res;
    if (clip) res = BoxAnd(&cb,mb,clip);
    else {res = YES; cb = *mb; }
    if (res) {  
	WaitBlit();
	BltBitMap(src->bm, cb.x - src->box.x, cb.y - src->box.y, dst->bm, 
	  cb.x - dst->box.x, cb.y - dst->box.y, cb.w, cb.h, mint, 0xff, NULL);
	WaitBlit();
	}
    }

/*----------------------------------------------------------------------*/
/* BoxNot creates an array of four boxes "n" which represent the	*/
/*  complement of the input box "b":  n = NOT(b)			*/
/*----------------------------------------------------------------------*/
#define BIG 25000
BoxNot(n, b) Box *n, *b; {	
    SHORT xm = b->x + b->w;
    SHORT ym = b->y + b->h;
    n->x = b->x;  n->y = -BIG;  n->w = BIG;        n->h = BIG + b->y;  n++;
    n->x = xm;    n->y = b->y;  n->w = BIG;        n->h = BIG;         n++;
    n->x = -BIG;  n->y = ym;    n->w = BIG + xm;   n->h = BIG;         n++;
    n->x = -BIG;  n->y = -BIG;  n->w = BIG + b->x; n->h = BIG + ym;
    }
    
/*----------- Synchronize with the vertical beam motion */

#ifdef awawawa 
void WaitBeam(n) SHORT n; { 
    SHORT vb,tol;
    /* WaitBeam causes problems with interlace so skip it*/
    n = MAX(0, MIN(256,n));
    if (curFormat == 2) return;
    tol = curFormat?100:50;
    for (;;) { 
	vb = VBeamPos() - n;
	if (vb<0) vb += 256;
	if (( 0 <= vb) && ( vb < tol))  return; 
	}
    }
#endif

extern PaintMode paintMode;
extern BOOL erasing;
extern BOOL firstDown;
extern SHORT lastSmX, lastSmY;

/* make bm same size as pict bm of object, curdepth in depth */
SaveSizeBM(bm,ob) struct BitMap *bm;  BMOB *ob; {
    return(NewSizeBitMap(bm, curDepth, ob->pict.box.w, ob->pict.box.h));
    }

int BMBlitSize(bm) struct BitMap *bm; {
    return((int) BlitSize(bm->BytesPerRow, bm->Rows));
    }


/*----------------------------------------------------------------------*/
/*  SMEAR mode:								*/
/*----------------------------------------------------------------------*/

/* First some utilities .... */

/* maskblit the save.bm at the CURRENT position */
MBSaveAtPict(ob,canv,clip,mask) 
    BMOB *ob; BoxBM *canv; Box *clip; UBYTE *mask;  {		    
    Box b;
    BoxAnd(&b,&ob->save.box,clip);
    b.x += ob->pict.box.x - ob->save.box.x;
    b.y += ob->pict.box.y - ob->save.box.y;
    MaskBlit( ob->save.bm, &ob->pict.box, mask, canv->bm,
	    &canv->box, &b, clip, COOKIEOP, tmpRas.RasPtr, 0xff,0);
    }
    
RelocBoxBM(fr,to) BoxBM *fr,*to; {
    DupBitMap(fr->bm,to->bm);
    BlankBitMap(fr->bm);
    to->box = fr->box;
    }

void DoSmear(ob,canv,clip)  BMOB *ob; BoxBM *canv; Box *clip;  {		    
    if (SaveSizeBM(temp.bm, ob)) return;
    temp.box = ob->pict.box;
    CopyBoxBM(canv, &temp, &temp.box, clip, REPOP);
    if (!firstDown) MBSaveAtPict(ob,canv,clip,ob->mask);
    else firstDown = NO;
    RelocBoxBM(&temp,&ob->save); /* save = temp */
    }

#ifdef DOADD
void DoAdd(ob,canv,clip)  BMOB *ob; BoxBM *canv; Box *clip;  {		    
    if (SaveSizeBM(temp.bm, ob)) return;
    temp.box = ob->pict.box;
    CopyBoxBM(canv, &temp, &temp.box, clip, REPOP);
    AddBM(ob->pict.bm, temp.bm, tmpRas.RasPtr);
    CopyBoxBM(&temp,camv, &temp.box,clip,REPOP);
    }
#endif
    

#define USEMSK  0x100
#define USEPCT  0x200
#define SPECIAL 0x400

USHORT paintProps[NPaintModes] = {
    USEPCT| USEMSK| COOKIEOP,  	/*  Mask */
	    USEMSK| COOKIEOP,	/*  Color  */
    USEPCT| REPOP,		/*  Replace */
    SPECIAL,   			/*  Smear */
    SPECIAL,			/*  Shade */
    SPECIAL,			/*  Blend */
	    USEMSK| COOKIEOP,	/*  Cycle Paint*/
    USEPCT| USEMSK| XORMASK	/*  Xor */
    };

/* pens don't have Picture: just mask so they do the best they can
   in Mask and Replace modes */
USHORT penModes[NPaintModes] = {
    Color,  	/*  Mask */
    Color,	/* Color  */
    Color,	/*  Replace */
    Smear,   	/*  Smear */
    Shade,	/*  Shade */
    Blend,	/*  Blend */
    Color,	/*  Cycle Paint*/
    Xor		/*  Xor */
    };

#define ERASPROPS   USEMSK|COOKIEOP

		
    
/*--------------------------------------------------------------*/
/* Paint the object onto the canvas through the clip box 	*/
/* using "paintMode", the current global painting mode      	*/
/*--------------------------------------------------------------*/
void PaintOb(ob, canv, clip)  BMOB *ob; BoxBM *canv; Box *clip;  {
    UBYTE *tmpMask = NULL;
    SHORT pluse,pldef,props;
    PaintMode pntMode;
    tmpMask = tmpRas.RasPtr;
    pntMode = paintMode;
    if (curpen!=USERBRUSH) pntMode = penModes[pntMode];
    if ((pntMode==Smear)||(pntMode==Blend)) { 
	if (erasing||!Painting) pntMode = Color;
	}
    props = paintProps[pntMode]; 
    pluse = 0;
    if (erasing) {
	if (pntMode==Replace) { PFillBox(&ob->pict.box); return; }
	  else if (pntMode!=Shade) 
		{ props = ERASPROPS; pldef = curxpc; }
	}
    else if  (props&USEPCT) pluse = ob->planeUse; 
	    else  { pldef = curRP->FgPen;   }
    if (props&SPECIAL)  switch(pntMode) {
	case Shade: DoShade(ob,canv,clip); break;
	case Smear: DoSmear(ob,canv,clip); break;
	case Blend: DoBlend(ob,canv,clip); break;
	}
    else  if (props&USEMSK) {
	    if ((pntMode==Xor)&!erasing)  pldef = 0xff;
	    MaskBlit(ob->pict.bm, &ob->pict.box,ob->mask,canv->bm,&canv->box,
		 &ob->pict.box,clip, props&0xff, tmpMask,pluse,pldef);
	    }
	else  CopyBoxBM(&ob->pict, canv,&ob->pict.box, clip,props&0xff);
    }

/*---Create a BoxBM type object, Copying a rectangle of bits from curRP*/    
/* assumes pict.bm and save.bm already point at BitMap structs		*/

SHORT ExtractBMOB(ob,bx,xoff,yoff) 
    BMOB *ob;		/* object to be created  */
    Box *bx;		/* bounds of object in curRP */
    int xoff,yoff; 	/* offset of handle from u.l. of source */
    {
    if (NewSizeBitMap(ob->pict.bm, curDepth, bx->w, bx->h)) return(FAIL);

/** put in AllocMem instead
    if (AvailMem(MEMF_PUBLIC|MEMF_CHIP) < 12000) {
	FreeBitMap(ob->pict.bm);
	return(FAIL);
	}
**/
    ob->pict.box = *bx;
    ob->save.box = *bx;
    DFree(ob->mask);
    ob->mask = BMAllocMask(ob->pict.bm);
    if (ob->mask == NULL) { FreeBitMap(ob->pict.bm); return(FAIL);}

    SetBitMap(ob->pict.bm, curxpc);
    CopyBoxBM(&canvas,&ob->pict, &ob->pict.box, &clipBox, REPOP);
    MakeMask(ob->pict.bm,ob->mask, curxpc, NEGATIVE);
    ob->minTerm = COOKIEOP;
    ob->planeUse = 0xff;
    ob->planeDef = 0;
    ob->flags = 0;
    ob->xoffs = xoff;    ob->yoffs = yoff;
    ob->xpcolor = curxpc;
    return(SUCCESS);
    }

/*--------------------------------------------------------------*/

FreeBMOB(ob) BMOB *ob; {
    FreeBitMap(ob->pict.bm);
    FreeBitMap(ob->save.bm);
    FreeBitMap(&tmpbm);	/* just being compulsive */
    DFree(ob->mask);
    }

/* this needs to get called on button up in smear mode */		
BMOBFreeTmp() { FreeBitMap(&tmpbm); }

BMOBMask(ob) BMOB *ob; {
    DFree(ob->mask);
    ob->mask = (UBYTE *)BMAllocMask(ob->pict.bm);
    if (ob->mask==NULL) return(FAIL);
    MakeMask(ob->pict.bm,ob->mask,ob->xpcolor,NEGATIVE);
    return(SUCCESS);
    }


BMOBNewSize(ob,d,w,h) BMOB *ob; SHORT d,w,h; {
    if (NewSizeBitMap(ob->pict.bm,d,w,h)) return(FAIL);
    ob->pict.box.w = w;
    ob->pict.box.h = h;
    DFree(ob->mask);
    ob->mask = (UBYTE *)BMAllocMask(ob->pict.bm);
    if (ob->mask==NULL){ FreeBitMap(ob->pict.bm); return(FAIL); }
    return(SUCCESS);
    }


local InvSaveBox(ob) BMOB *ob; {
#ifdef DBGSTR
    dprintf(" InvSaveBox ... \n");
#endif
    TempXOR();
    PThinFrame(&ob->save.box);
    RestoreMode();
    }
    
local void ShowBMOB(ob) BMOB *ob; {
    if (ob->flags&SHOWING) return;
    ob->save.box = ob->pict.box;
    if (NewSizeBitMap(ob->save.bm, curDepth, ob->pict.box.w,ob->pict.box.h))
	{ 
	ob->flags |= TOOBIGTOPAINT;
	InvSaveBox(ob);
	}
    else {
	CopyBoxBM(&canvas, &ob->save, &ob->save.box, &clipBox, REPOP);
	PaintOb(ob,&canvas,&clipBox);
	}
    ob->flags |= SHOWING;
    }


void ClearBMOB(ob) BMOB *ob; {
    if (!(ob->flags&SHOWING)) return;
    if (ob->flags&TOOBIGTOPAINT) {
	InvSaveBox(ob);
	ob->flags &= (~TOOBIGTOPAINT); 
	}
    else CopyBoxBM(&ob->save, &canvas, &ob->save.box, &clipBox, REPOP);
    FreeBitMap(ob->save.bm); 
    FreeBitMap(&tmpbm);
    ob->flags &= (~SHOWING);
    }

CheapMove(ob) BMOB *ob; {
#ifdef DBGSTR
    dprintf("Cheap move... \n");
#endif
    ClearBMOB(ob); /* so do economy version */
    ShowBMOB(ob);
    }

/* updates the screen for a BMOB that has changed position or size 
   pict is it's new position, and saves it's old one */

void ChangeBMOB(ob) BMOB *ob;{
    SHORT i;
    Box  c, *d, notb[4];
    Box *new = &ob->pict.box, *old = &ob->save.box;
    if (!(ob->flags&SHOWING)) ShowBMOB(ob);
    else {
	if ((ob->flags&TOOBIGTOPAINT)||
	 ( NewSizeBitMap(temp.bm, curDepth, new->w, new->h))) 
	    { CheapMove(ob); return; }
	temp.box = *new; 
	
	/*----- copy intersection of old and new to new */
	CopyBoxBM(&ob->save, &temp, old, new, REPOP);
    
	/* -----copy the newly covered areas (new-old) from scrn to new */ 
	BoxNot(&notb, old);
	for (i=0, d = &notb[0]; i<4; i++,d++) if (BoxAnd(&c,d,new))
	    CopyBoxBM(&canvas, &temp, &c, &clipBox, REPOP);
    
	/*-----fix up the uncovered areas (old-new) on the screen */
	BoxNot(&notb, new);
	for (i=0, d = &notb[0]; i<4; i++,d++)  if (BoxAnd(&c,d,old))
	    CopyBoxBM(&ob->save, &canvas, &c, &clipBox, REPOP);
    
	/*-----save an unpolluted copy of the area the object will cover */
	if (CopyBitMap(temp.bm,ob->save.bm)) { 
	    /* stretching: couldn't alloc save.bm-- must be stretching */
#ifdef DBGSTR
	    dprintf(" ChangeBM: Couldnt alloc save.bm \n");
#endif
	    DupBitMap(temp.bm, ob->save.bm);
	    ob->save.box = ob->pict.box;
	    BlankBitMap(temp.bm);
	    CheapMove(ob);
	    return;
	    }

	/*-----paint object onto background in temp */
	PaintOb(ob,&temp,NULL);

	/*-----copy updated area to screen, synced with vertical retrace */
/*	WaitBeam( new->y + new->h ); */

#ifdef DOLAYERS
	tempRP.BitMap = &tmpbm;
	/* ----play the layers game:--- */
	ClipBlit(&tempRP,0,0,curRP,new->x,new->y,new->w,new->h,REPOP);
#else
	/* use my own clipping */
	CopyBoxBM(&temp, &canvas, new, &clipBox, REPOP); 
#endif
	}
    ob->save.box = ob->pict.box;
    }

local PosBMOB(ob,nx,ny) BMOB *ob; SHORT nx,ny; {
    ob->pict.box.x = nx - ob->xoffs;
    ob->pict.box.y = ny - ob->yoffs;
    }
    
MoveBMOB(ob,nx,ny) BMOB *ob; SHORT nx,ny; { PosBMOB(ob,nx,ny); ChangeBMOB(ob); }

DispBMOB(ob,nx,ny) BMOB *ob; SHORT nx,ny; {
    PosBMOB(ob,nx,ny); 
    PaintOb(ob,&canvas, &clipBox);  
    }

