/*----------------------------------------------------------------------*/
/*  									*/
/*     stretch.c  -- Brush Stretching Operations			*/
/*  									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>
#include <pnts.h>

extern SHORT curpen;
extern Dims newDim;
extern BMOB origBr,curbr;
extern PointRec pnts;
extern BOOL wasShift;
extern void (*killMode)(), (*nop)();
extern void obclr();
extern Box screenBox;
extern PaintMode paintMode;

/* #define DBGSTRCH */

#define local 

Dims newDim;
SHORT savePntMode=0;

LONG MulDiv(a,b,c) LONG a,b,c; {  return( (a*b)/c ); }

void FixOffs() { 
    SetPaintMode(savePntMode);
    curbr.xoffs = MulDiv(origBr.xoffs,newDim.w,origBr.pict.box.w); 
    curbr.yoffs = MulDiv(origBr.yoffs,newDim.h,origBr.pict.box.h);  
    }

SHORT StrBMOB(src,dst,newW,newH) BMOB *src,*dst; SHORT newW,newH; {
    Box sbox,dbox;
    if ( BMOBNewSize(dst,src->pict.bm->Depth,newW,newH)) {
#ifdef DBGSTRCH
	dprintf(" StrBMOB: couldnt alloc bm \n");
#endif
	return(FAIL);
	}
    sbox.x = sbox.y = dbox.x = dbox.y = 0;
    sbox.w = src->pict.box.w;
    sbox.h = src->pict.box.h;
    dbox.w = newW;
    dbox.h = newH;
    if( StrBits(src->pict.bm,&sbox,dst->pict.bm,&dbox,REPOP)){
#ifdef DBGSTRCH
	dprintf(" StrBMOB: couldnt do stretch \n");
#endif
	return(FAIL);
	}
    MakeMask(dst->pict.bm,dst->mask,dst->xpcolor,NEGATIVE);
    return(SUCCESS);
    }


void StrBrTo(w,h) int w,h; {
    if (curpen!=USERBRUSH) return;
    /* let it be stretched up to 1/2 screen size in area */
    if ((w<1)||(h<1)||(w*h >(screenBox.w*screenBox.h/2))) return;
    if (SaveBrush(STRETCHED)) return;
    ZZZCursor();
    newDim.w = w;
    newDim.h = h;
    if (StrBMOB(&origBr,&curbr,newDim.w,newDim.h)) RestoreBrush();
    else FixOffs();
    UnZZZCursor();
    }

void BrHalf()  { StrBrTo(curbr.pict.box.w/2, curbr.pict.box.h/2); }
void BrDubl()  { StrBrTo(curbr.pict.box.w*2, curbr.pict.box.h*2); }
void BrDublX() { StrBrTo(curbr.pict.box.w*2, curbr.pict.box.h); }
void BrDublY() { StrBrTo(curbr.pict.box.w, curbr.pict.box.h*2); }

/* ---------------Stretch brush----------------- */
    
Point bpl={0};
    
/* Begin stretching brush (button down) */

/* remember the upper left corner */
beginstr() {
    bpl.x = mx-curbr.xoffs;
    bpl.y = my-curbr.yoffs;
    }

#define oldW origBr.pict.box.w
#define oldH origBr.pict.box.h

LONG LongMult(a,b,c) LONG a,b,c; { return( a*b); }
    
local void doStretch() {
    newDim.w = mx-bpl.x + 1;
    newDim.h = my-bpl.y + 1;
    if  ( (newDim.w<=0) || (newDim.h<=0) ) return;
    if (wasShift) {
	if (LongMult(newDim.h,oldW) > LongMult(newDim.w,oldH)) 
	    newDim.w =   MulDiv(oldW,newDim.h,oldH);
	    else newDim.h = MulDiv(oldH,newDim.w,oldW);
	}
    if (StrBMOB(&origBr,&curbr,newDim.w,newDim.h))   {
#ifdef DBGSTRCH
	dprintf(" Can't make the brush that big! (w= %d, h = %d)\n",
			newDim.w, newDim.h); 
#endif
	ClearFB();
	RestoreBrush();
	killMode = nop;
	SetPaintMode(savePntMode);
	AbortIMode();
	return;
	} 
    curbr.xoffs = (mx - bpl.x);
    curbr.yoffs = (my - bpl.y);
    obmv();
    }

void IMStrBrush(restart) BOOL restart; {
    if (!restart) {
	if (curpen!=USERBRUSH) {RevertIMode(); return;}
	if (SaveBrush(STRETCHED)) {RevertIMode();  return;}
	/* hold at the lower right for stretching */
	curbr.xoffs = curbr.pict.box.w - 1;   
	curbr.yoffs = curbr.pict.box.h - 1;
	savePntMode = paintMode;
	SetPaintMode(Mask);
	}
    IModeProcs(&obmv,&obmv,&obclr,&beginstr,&obmv,
	&doStretch,&obclr, FixOffs);
    killMode = &FixOffs; 
    }

/*----------------------------------------------------------------------*/
/* << StrBits >>   Stretching BltBitMap					*/
/*  Does not handle negative stretches (use FlipX, FlipY first)		*/
/*  since X stretching is slower, order the X- and Y- stretching to     */
/*	minimize the X stretching 					*/
/*  									*/
/*----------------------------------------------------------------------*/

#undef sx
#undef sy

local SHORT sx,sy,sw,sh,dx,dy,dw,dh;
    
StrX(sbm,sbox,dbm,dbox,minterm)
    struct BitMap *sbm,*dbm;
    Box *sbox,*dbox;
    SHORT minterm;
    {
    SHORT xs,xd,xs0,xd0,xslast,s,dxmax;
    sy = sbox->y; dy = dbox->y; 
    sw = sbox->w-1; dw = dbox->w-1;    sh = sbox->h;
    dxmax = dbox->x + dw;
    xs0 = xs = xslast = sbox->x; xd0 = dbox->x;   
    s = dw/2; 
    for (xd = dbox->x+1 ; xd <= dxmax; xd++ ) {
	s += sw;
	while ( s >= dw ) { s -= dw; xs++; } 
	if (xs != xslast + 1 ) { 
	    BltBitMap(sbm,xs0,sy,dbm,xd0,dy,xd-xd0,sh,minterm,0xff,NULL);
	    xs0 = xs;    xd0 = xd;
	    }
	xslast = xs;
	}
    BltBitMap(sbm,xs0,sy,dbm,xd0,dy,xd-xd0,sh,minterm,0xff,NULL);   
    }

StrY(sbm,sbox,dbm,dbox,minterm)
    struct BitMap *sbm,*dbm;
    Box *sbox,*dbox;
    SHORT minterm;
    {
    SHORT ys,yd,ys0,yd0,yslast,s,dymax;
    sx = sbox->x; dx = dbox->x; 
    sh = sbox->h-1; dh = dbox->h -1;    sw = sbox->w;
    dymax = dbox->y + dh;
    ys0 = ys = yslast = sbox->y; yd0 = dbox->y; 
    s = dh/2;
    for (yd = dbox->y+1; yd <= dymax; yd++ ) {
	s += sh;
	while ( s >= dh ) { s -= dh; ys++; }
	if (ys != yslast + 1)  {
	    BltBitMap(sbm,sx,ys0,dbm,dx,yd0,sw,yd-yd0,minterm,0xff,NULL);
	    ys0 = ys;    yd0 = yd;
	    }
	yslast = ys;
	}
    BltBitMap(sbm,sx,ys0,dbm,dx,yd0,sw,yd-yd0,minterm,0xff,NULL);
    }

StrBits(srcBM,sbox,dstBM,dbox,mint) 
    struct BitMap *srcBM;     /* source bimap */
    Box *sbox;			/* source rectangle */
    struct BitMap  *dstBM;	/* destination bitmap */
    Box *dbox;			/* destination rectangle */
    SHORT mint;			/* minterm for blitter */
    {
    SHORT depth;
    Box tbox;
    struct BitMap tmpBM;
    if ( (sbox->w == dbox->w) && (sbox->h == dbox->h)) {
	BltBitMap(srcBM,sbox->x,sbox->y,dstBM,dbox->x,dbox->y,
		sbox->w,sbox->h,mint,0xff,NULL);
	return(0);
	}
    if ((dbox->h == 0)||(dbox->w == 0)) return(0);
    if ( dbox->w == sbox->w) {StrY(srcBM,sbox,dstBM,dbox,mint);	return(0); }
    if ( dbox->h == sbox->h) {StrX(srcBM,sbox,dstBM,dbox,mint);	return(0); }
    
    depth = dstBM->Depth;
    
    if (dbox->h > sbox->h) {   /*---  Getting Taller ---*/
	InitBitMap(&tmpBM,depth,dbox->w,sbox->h);
	if (TmpAllocBitMap(&tmpBM)) return(FAIL);
	tbox.x = 0; tbox.y = 0; tbox.w = dbox->w; tbox.h = sbox->h;
	StrX(srcBM, sbox, &tmpBM, &tbox, REPOP);
	StrY(&tmpBM, &tbox, dstBM, dbox, mint);
	}
    else {         /*----  Getting Shorter ----*/ 
	InitBitMap(&tmpBM,depth,sbox->w,dbox->h);
	if (TmpAllocBitMap(&tmpBM)) return(FAIL);
	tbox.x = 0; tbox.y = 0; tbox.w = sbox->w; tbox.h = dbox->h;
	StrY(srcBM, sbox, &tmpBM, &tbox, REPOP);
	StrX(&tmpBM, &tbox, dstBM, dbox, mint);
	}
    FreeBitMap(&tmpBM);
    return(SUCCESS);
    }

static SHORT dum;
static SHORT dum1 = 2;

