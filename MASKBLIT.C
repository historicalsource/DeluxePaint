/*----------------------------------------------------------------------*/
/*									*/
/*	MaskBlit	(c) Electronic Arts	1985			*/
/*									*/
/* 	7/12/85	created    		Dan Silva 			*/
/*	7/14/85 added clipping 		Dan Silva			*/
/*	7/19/85 added planeUse,planeDef Dan Silva			*/
/*									*/
/*									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>

#undef sx
#undef sy

#define local  static

extern SHORT masks[];

extern BlitterRegs *ar;

#define yA  	0xf0
#define yB	0xcc
#define yC	0xaa
#define nA	0x0f
#define nB 	0x33
#define nC	0x55

#define USEA 0x0800
#define USEB 0x0400
#define USEC 0x0200
#define USED 0x0100
#define MASKWRITE  	0x0F00  /* USEA|USEB|USEC|USED */
#define CONSTWRITE	0x0B00	/* USEA|USEC|USED */
    
#define COPYA 0xf0	/* minterm = A */

#define WordOffs(x)     (((x)>>3)&0xfffe)
#define LeftMask(x) (masks[(x)&15])
#define RightMask(x) (~masks[((x)&15)+1]) 

/*----------------------------------------------------------------------*/
/* MaskBlit -- Takes control of the blitter to do a blit of a source    */
/*	bitmap through a mask into a destination bitmap, clipped to an	*/
/*	optional clipping rectangle.  Also lets you specify which 	*/
/*	planes of the destination to operate on and default bits for	*/
/*	the other planes.  						*/
/*----------------------------------------------------------------------*/

void MaskBlit(sbm,spos,mask, dbm, dpos, copyBox, clip, minTerm,tmpmsk,planeUse,planeDef)
    struct BitMap *sbm;	/* source bitmap 			*/
    Point *spos;	/* where in space is the source bitmap located */
    UBYTE *mask;	/* mask plane (same dimensions as sbm) 	*/
    struct BitMap *dbm;	/* destination bitmap			*/
    Point *dpos;	/* where is dest bitmap located */
    Box *copyBox;	/* area to be copied   */
    Box *clip;		/* optional clip box   */
    BYTE minTerm;	/* normally = 0xCA for cookie cut	*/
    UBYTE *tmpmsk;	/* should be as big as the mask plane    */
    UBYTE planeUse,planeDef; /* which planes to do in the destination and
    			the default bits for the other planes */
    {
    LONG ash,shift,slWrd,srWrd,dlWrd,drWrd;
    LONG blsize,bytWidth,soffset,doffset,sdepth,ddepth,ns,nd,sBytes,dBytes,srx;
    UBYTE *clmask = mask,bit;
    UWORD con0_use,con0_def;
    Box cBox;
    BOOL hclip = NO;
    int dx,dy,sx,sy,w,h;
    if (clip) {	if (!BoxAnd(&cBox,clip,copyBox)) return;}
	else cBox = *copyBox;
    
    sx = cBox.x - spos->x;    sy = cBox.y - spos->y;
    dx = cBox.x - dpos->x;    dy = cBox.y - dpos->y;
    w = cBox.w;
    h = cBox.h;
    slWrd = WordOffs(sx);
    srx = sx+w-1;
    srWrd = WordOffs(srx);
    sBytes = srWrd-slWrd+2;
    soffset = sy*sbm->BytesPerRow + slWrd;
    ddepth = dbm->Depth;
    sdepth = sbm->Depth;
    dlWrd = WordOffs(dx);
    drWrd = WordOffs(dx+w-1);
    shift = (dx&15) - (sx&15);
    if (shift<0) { shift = 16+shift; dlWrd -= 2;}
    dBytes = drWrd-dlWrd+2;
    if (dBytes>dbm->BytesPerRow) {
	/* patch the "Destination Wrap-Around Bug" by breaking
	    into two halves -----*/
	cBox = *copyBox;
	cBox.w = cBox.w/2;
	MaskBlit(sbm,spos,mask, dbm, dpos, &cBox, clip, minTerm,tmpmsk,planeUse,planeDef);
	cBox.x += cBox.w;
	cBox.w = copyBox->w - cBox.w;
	MaskBlit(sbm,spos,mask, dbm, dpos, &cBox, clip, minTerm,tmpmsk,planeUse,planeDef);
	return;
	}
    if (((sx&15)!=0)||((srx&15)!=15))
    	{    /*  Apply clip rectangle to the mask plane */
	hclip = YES;
	WaitBlit();
	OwnBlitter(); 
	ar->fwmask = LeftMask(sx);
	ar->lwmask = RightMask(srx);
	ar->bltcon0 = USEA|USED|COPYA;  
	ar->bltcon1 = 0; 	
	ar->bdata = 0;
	ar->bltmda = ar->bltmdd = sbm->BytesPerRow - sBytes;
	ar->bltpta = mask+soffset;
	ar->bltptd = tmpmsk+soffset;
	ar->bltsize = (h<<6)|(sBytes>>1);  /* start the blit */
	clmask = tmpmsk;	
	}
    ash = shift<<12;
    bytWidth = MAX(sBytes,dBytes);
    doffset = dy*dbm->BytesPerRow + dlWrd;
    blsize = (h<<6)|(bytWidth>>1);  
    
    con0_use = ash|MASKWRITE|(minTerm&0xff);  
    con0_def = ash|CONSTWRITE|(minTerm&0xff);  
    
    WaitBlit();
    if (!hclip) OwnBlitter(); 

    ar->fwmask =  LeftMask(sx);
    ar->lwmask = (dBytes>sBytes)? 0 : RightMask(srx);
    ar->bltmda = ar->bltmdb = sbm->BytesPerRow - bytWidth;
    ar->bltmdc = ar->bltmdd = dbm->BytesPerRow - bytWidth;
    bit = 1;

    for (ns = nd = 0; nd<ddepth; nd++) {
	WaitBlit();
	ar->bltpta = clmask + soffset; 		 	/* a-data address */
	ar->bltptd = ar->bltptc = dbm->Planes[nd] + doffset;	/* c-data address */
	if ((bit&planeUse)&&(ns<sdepth)) {  /* use b source */
	    ar->bltcon1 = ash; 	
	    ar->bltcon0 = con0_use;
	    ar->bltptb = sbm->Planes[ns++] + soffset;		/* b-data  address */
	    }
	else {   /* default */
	    ar->bltcon1 = 0; 	
	    ar->bltcon0 = con0_def;
	    ar->bdata = (bit & planeDef ) ? 0xffff : 0;
	    }
	ar->bltsize = blsize;				/* trigger blitter */
	bit <<= 1;
	}
    WaitBlit();
    DisownBlitter(); 
    }

