/*----------------------------------------------------------------------*/
/*  									*/
/*        shade.c --							*/
/*  									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>

#define local
extern BoxBM temp;
extern BOOL erasing;
extern Range *shadeRange;
extern struct TmpRas tmpRas;
extern BoxBM canvas;

void MaskRange();

/*----------------------------------------------------------------------*/
/*  SHADE mode:								*/
/*----------------------------------------------------------------------*/
void DoShade(ob,canv,clip)   BMOB *ob; BoxBM *canv; Box *clip; {		    
    Box b;  
    BOOL res;
    int lo,hi;
    if (BMPlaneSize(ob->pict.bm)> (tmpRas.Size/2)) return;
    if (SaveSizeBM(temp.bm, ob)) return;
    temp.box = ob->pict.box;
    CopyBoxBM(canv, &temp, &temp.box, clip, REPOP);
    if (erasing) { lo = shadeRange->low+1; hi = shadeRange->high; }
	else {lo = shadeRange->low; hi = shadeRange->high - 1; }
    if (shadeRange==NULL) setmem(tmpRas.RasPtr,BMPlaneSize(ob->pict.bm),0xff);
    else MaskRange(temp.bm, tmpRas.RasPtr,tmpRas.RasPtr +tmpRas.Size/2,lo,hi);
    if (clip) res = BoxAnd(&b,&ob->pict.box,clip); 
	else { res = YES; b = ob->pict.box; }
    if (res) ShadePaint(ob->pict.bm,ob->mask, b.x - ob->pict.box.x,b.y - ob->pict.box.y, 
	canv->bm, b.x - canv->box.x, b.y - canv->box.y,
	b.w, b.h, erasing? MINUSOP: PLUSOP, tmpRas.RasPtr);
    }


/*----------------------------------------------------------------------*/
/* MaskRange: Build a mask for a bitmap which has 1's for every pixel	*/
/* in [lower..upper] inclusive.						*/
/*									*/
/*	Thanks to David Maynard for suggesting this  		 	*/
/*	algorithm.							*/
/*----------------------------------------------------------------------*/

#define yA  	0xf0
#define yB	0xcc
#define yC	0xaa
#define nA	0x0f
#define nB 	0x33
#define nC	0x55

UBYTE mints[4] = 
    { nA&nB&yC, nA&yB&yC, yA&nB&yC,  yA&yB&yC };

UBYTE mints4P[2] = {nB&yC, yB&yC};

#ifdef DBUG
dbgmask(msk,bpr,h) UBYTE  *msk; int bpr,h; {
    struct RastPort rp;
    InitRastPort(&rp);
    rp.BitMap = canvas.bm;
    SetAPen(&rp,1);
    SetBPen(&rp,0);
    SetDrMd(&rp,JAM2);
    BltTemplate(msk,0,bpr,&rp,0,30,bpr*8,h);
    }
    
dbgbm(bm) struct BitMap *bm; {
BltBitMap(bm,0,0,canvas.bm,0,80,bm->BytesPerRow*8,bm->Rows,REPOP,0xff,NULL);
    }
#endif

void MaskRange(bm, mask, tmp, lower, upper)
    struct BitMap *bm;	/* bitmap for which mask is being created */
    UBYTE *mask;	/* to receive mask */
    UBYTE *tmp;		/* temp storage */
    int lower,upper;	/* defines range for mask */
    {
    int minterm, i, octstart, octend, lastInOct,oct,depth;
    int bpr = bm->BytesPerRow;
    int h = bm->Rows;
    int bltsize = BlitSize(bpr,h);
    
    octstart = lower>>3;
    octend = upper>>3;
    lastInOct = (octstart<<3)+7;
    depth = bm->Depth;
    
    for (oct = octstart; oct<=octend; oct++) {
	minterm = 0;
	/* find subset of ALL pixels whose lower order 3 bits 
		are in this octaves subrange */
	for (i = lower; i<= MIN(upper, lastInOct); ++i)
    		minterm |= ( 1 << (i&7) );

	BltABCD(bm->Planes[2],bm->Planes[1],bm->Planes[0],tmp, bltsize, minterm);

	/* filter out the pixels not in this 8 color "octave" */
	
	if (depth==5) 
	  BltABCD(bm->Planes[4],bm->Planes[3], tmp, tmp, bltsize, mints[oct]);
	else if (depth==4)
	  BltABCD(NULL,bm->Planes[3], tmp, tmp, bltsize, mints4P[oct]);

	/* OR in to mask being built up */
	BltABCD(tmp,mask,NULL,mask, bltsize, (oct==octstart)?yA : yA|yB);

	lower = lastInOct+1;
	lastInOct += 8;
	}	    
    }


#ifdef newway
    
ShadePaint(sbm, spos, mask, dbm, dpos, copyBox, incop,tmpbuf) 
    struct BitMap *sbm;
    Point *spos;
    UBYTE *mask;
    struct BitMap *dbm;
    Point *dpos;
    Box *copyBox;
    UBYTE incop;		
    UBYTE *tmpbuf; 
    {
    struct BitMap tmp,carry;
    int i, depth;
    int sx,sy,dx,dy,w,h;
    PLANEPTR svp0;
    sx = copyBox->x - spos->x;    sy = copyBox->y - spos->y;
    dx = copyBox->x - dpos->x;    dy = copyBox->y - dpos->y;
    w = copyBox->w;    h = copyBox->h;
    depth = dbm->Depth;
    dbm->Depth = 1;
    svp0 = dbm->Planes[0];
    InitBitMap(&carry,1,sbm->BytesPerRow*8, sbm->Rows);
    carry.Planes[0] = tmpbuf;
    tmp = carry;
    tmp.Planes[0] = mask;
    BltBitMap(&tmp,sx,sy,&carry,sx,sy,w,h,ANDOP,0xff,NULL);
    WaitBlit();
    for (i = 0; i< depth; i++) {
	dbm->Planes[0] = dbm->Planes[i];
	BltBitMap(&carry,sx,sy,dbm,dx,dy,w,h, XOROP, 0xff, NULL);
	WaitBlit();
	BltBitMap(dbm,dx,dy,&carry,sx,sy,w,h, incop, 0xff, NULL);
	WaitBlit();
	}
    dbm->Planes[0] = svp0;
    dbm->Depth = depth;
    }

#else

ShadePaint(sbm, mask, sx, sy, dbm, dx, dy, w,h, incop,tmpbuf) 
    struct BitMap *sbm;
    UBYTE *mask;
    SHORT sx,sy;
    struct BitMap *dbm;
    SHORT dx,dy,w,h;
    UBYTE incop;		
    UBYTE *tmpbuf; 
    {
    struct BitMap tmp,carry;
    int i, depth;
    PLANEPTR svp0;
    
    depth = dbm->Depth;
    dbm->Depth = 1;
    svp0 = dbm->Planes[0];
    InitBitMap(&carry,1,sbm->BytesPerRow*8, sbm->Rows);
    carry.Planes[0] = tmpbuf;
    tmp = carry;
    tmp.Planes[0] = mask;
    BltBitMap(&tmp,sx,sy,&carry,sx,sy,w,h,ANDOP,0xff,NULL);
    WaitBlit();
    for (i = 0; i< depth; i++) {
	dbm->Planes[0] = dbm->Planes[i];
	BltBitMap(&carry,sx,sy,dbm,dx,dy,w,h, XOROP, 0xff, NULL);
	WaitBlit();
	BltBitMap(dbm,dx,dy,&carry,sx,sy,w,h, incop, 0xff, NULL);
	WaitBlit();
	}
    dbm->Planes[0] = svp0;
    dbm->Depth = depth;
    }
#endif


