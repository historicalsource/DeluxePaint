/*----------------------------------------------------------------------*/
/*  									*/
/*     magbits.c  -- Integer Bitmap Magnification  (c)Electronic Arts	*/
/*  									*/
/*  									*/
/*   	6/26/85   Created	Dan Silva  -- Electronic Arts		*/
/*   	7/13/85   v27 Mods	Dan Silva  -- Electronic Arts		*/
/*  									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>

#define method1

#ifdef method1
/* CLIPPED stretch by integer multiple in X and Y direction --
    
    This version does the final y-stretch by "tricking" BltBitMap into 
    outputting each line N times in one call. It requires H calls to 
    BltBitMap, wher H is the number of "fat bit" scan lines visible in 
    the clipping window.   It is thus slow for small magnifications, 
    but quite fast for high magnifications.  Even at small 
    magnifications the updates have a much smoother appearance than the 
    other algorithm. */
    
short MagBits(sbm,sbox,dbm,dx,dy,NX,NY,clip)
    struct BitMap *sbm, *dbm;
    Box *sbox;
    int dx,dy;	/* upper left corner in dbm */
    int NX,NY;		/* magnify factor */
    Box *clip;		/* clip box in destination bm */
    {
    SHORT xs,xd,yd,ys,sxmax,i,nout,lx,clymax, depth,bpr,nrep,ydcl;
    static SHORT dw;
    static Box c,fr;
    struct BitMap tmpBM,tmpBM2;
    depth = dbm->Depth;
    
    /* clip in source bitmap to avoid unnecessary work */
    
    fr.x = (clip->x - dx)/NX  + sbox->x;
    fr.y = (clip->y - dy)/NY  + sbox->y;
    fr.w = (clip->w + NX - 1)/NX;
    fr.h = (clip->h + NY - 1)/NY;
    
    if (!BoxAnd(&c,&fr,sbox)) return(0);

    dx += NX*(c.x - sbox->x);
    dy += NY*(c.y - sbox->y);
    dw = c.w*NX;

    if ( (NX==1)&&(NY==1)) 
      { BltBitMap(sbm,c.x,c.y,dbm,dx,dy,c.w,c.h,REPOP,0xff,NULL); return(0); }
    
    tmpBM.Planes[0] = tmpBM2.Planes[0] = 0;

    InitBitMap(&tmpBM,depth,dw,c.h);
    InitBitMap(&tmpBM2,depth,dw,c.h);

    if (TmpAllocBitMap(&tmpBM)) {
	GunTheBrush();
	if (TmpAllocBitMap(&tmpBM));
	return(FAIL); 
	}
    if (TmpAllocBitMap(&tmpBM2)) { 
	GunTheBrush();
	if (TmpAllocBitMap(&tmpBM2)) { 
	    FreeBitMap(&tmpBM); 
	    return(FAIL);
	    }
	}
    ClearBitMap(&tmpBM); ClearBitMap(&tmpBM2);

    /*--------- stretch horizontally first--------------*/
    sxmax = c.x + c.w; 
    xd = 0;
    for (xs = c.x; xs < sxmax; xs++ ) {
	BltBitMap(sbm,xs,c.y,&tmpBM,xd,0,1,c.h,REPOP,0xff,NULL);
	xd += NX;
	}
    for (i=0; i<NX; i++) 
	BltBitMap(&tmpBM,0,0,&tmpBM2,i,0,dw-NX+1,c.h,OROP,0xff,NULL);
    
    /*--------- prepare to stretch Vertically--------------*/
    lx = 0;
    if ( (nout = clip->x - dx) >0) { lx += nout; dx+=nout; dw -= nout; }
    if ( (nout = dx+dw - (clip->x+clip->w)) > 0) { dw -= nout; } 

    yd = dy;
    clymax = clip->y + clip->h;

    /**--------- Stretch vertically into destination Raster Port -----***/
    /* Mess around with width of tmpBM2 to trick BltBitMap   ----- 	*/
    /* into outputting the same line many times                     	*/

    bpr = tmpBM2.BytesPerRow;
    tmpBM2.BytesPerRow = 0;
    
    for (ys = 0; ys < c.h; ys++) {
	nrep = NY;
	ydcl = yd;
	if (ys==0) if (( nout=(clip->y-ydcl))>0) { nrep-= nout; ydcl += nout;}
	if (ys==c.h-1) if (( nout = ( yd+NY - clymax)) > 0) nrep -= nout;
	BltBitMap(&tmpBM2,lx,0,dbm,dx,ydcl,dw,nrep, REPOP,0xff,NULL);
	for (i = 0; i<depth; i++) tmpBM2.Planes[i] += bpr;
	yd += NY;
	}
    
    /* --------- restore bitmap pointers to original state -----*/
    for (i=0; i<depth; i++) tmpBM2.Planes[i] -= c.h*bpr;
    tmpBM2.BytesPerRow = bpr;
    FreeBitMap(&tmpBM);
    FreeBitMap(&tmpBM2);
    return(SUCCESS);
    }


/* CLIPPED stretch by integer multiple in X and Y direction --
    
    This version does the final y-stretch by "tricking" BltRaster into 
    spacing scan lines out by N lines.  Thus it only takes N calls to 
    BltRaster, and is quite fast, especially for low magnifications. 
    The resulting updates to the screen have a ragged appearance 
    however, since the changes occur to every Nth scan line on each 
    blit.

*/

#else

SHORT dbgmgb = NO;

short MagBits(sbm,sbox,dbm,dx,dy,NX,NY,clip)
    struct BitMap *sbm, *dbm;
    Box *sbox;
    int dx,dy;	/* upper left corner in dbm */
    int NX,NY;		/* magnify factor */
    Box *clip;		/* clip box in destination bm */
    {
    
    SHORT xs,xd,yd,ys,sxmax,i,nout,lx,clymax, depth;
    SHORT svbpr,magbpr,magyd,pixper,magpixper,magydcl,shcl,ydbot;
    static SHORT dw;
    static Box c,fr;
    struct BitMap tmpBM,tmpBM2;


    depth = dbm->Depth;
    
    /* clip in source bitmap to avoid unnecessary work */
    
    fr.x = (clip->x - dx)/NX  + sbox->x;
    fr.y = (clip->y - dy)/NY  + sbox->y;
    fr.w = (clip->w + NX - 1)/NX;
    fr.h = (clip->h + NY - 1)/NY;
    
    if (!BoxAnd(&c,&fr,sbox)) return(0);

    dx += NX*(c.x - sbox->x);
    dy += NY*(c.y - sbox->y);
    dw = c.w*NX;

    if ( (NX==1)&&(NY==1)) { BltBitMap(sbm,c.x,c.y,dbm,dx,dy,c.w,c.h,REPOP,0xff,NULL); return(0); }
    
    tmpBM.Planes[0] = tmpBM2.Planes[0] = 0;

    InitBitMap(&tmpBM,depth,dw,c.h);
    InitBitMap(&tmpBM2,depth,dw,c.h);

    if (TmpAllocBitMap(&tmpBM)) return(-1);
    if (TmpAllocBitMap(&tmpBM2)) {FreeBitMap(&tmpBM); return(-1);}

    ClearBitMap(&tmpBM); ClearBitMap(&tmpBM2);

    /*--------- stretch horizontally first--------------*/
    sxmax = c.x + c.w; 
    xd = 0;
    for (xs = c.x; xs < sxmax; xs++ ) {
	BltBitMap(sbm,xs,c.y,&tmpBM,xd,0,1,c.h,REPOP,0xff,NULL);
	xd += NX;
	}
    for (i=0; i<NX; i++) 
	BltBitMap(&tmpBM,0,0,&tmpBM2,i,0,dw-NX+1,c.h,OROP,0xff,NULL);
    

    /*--------- prepare to stretch Vertically--------------*/
    lx = 0;
    if ( (nout = clip->x - dx) >0) { lx += nout; dx+=nout; dw -= nout; }
    if ( (nout = dx+dw - (clip->x+clip->w)) > 0) { dw -= nout; } 
    yd = dy;
    clymax = clip->y + clip->h;

    /*---- mess around with width of dbm to trick BltBitMap----- */

    svbpr = dbm->BytesPerRow;
    magbpr = svbpr*NY;
    dbm->BytesPerRow = magbpr;  /* Make it skip N lines at a time
	    by saying the destination bitmap is NY times as wide
	    as it really is */
    
    yd = dy;
    magyd = (yd>=0) ? yd/NY : yd/NY - 1;
    pixper = 8*svbpr;
    magpixper = 8*magbpr;
    dx +=  pixper*(yd - magyd*NY);
    ydbot = yd + NY*(c.h-1);

#ifdef asdas
    if (dbgmgb) printf("NY=%ld,yd=%ld,magyd=%ld,dx=%ld,svbpr=%ld\n",
    	NY,yd,magyd,dx,svbpr);
#endif
    
    /**--------- Stretch vertically into destination Raster Port ***/
    for (i = 0; i < NY; i++)     {
	magydcl = magyd;  shcl = c.h; ys = 0;
	if (yd < clip->y) { magydcl++; shcl--; ys++; }
	if (ydbot >= clymax) shcl--;  
	BltBitMap(&tmpBM2,lx,ys,dbm,dx,magydcl, dw,shcl,REPOP,0xff,NULL);
	dx += pixper;
	if (dx>magpixper) { dx -= magpixper; ++magyd; }
	++yd; 
	++ydbot;
	}
	    
    dbm->BytesPerRow = svbpr;
    FreeBitMap(&tmpBM);
    FreeBitMap(&tmpBM2);
    return(0);
    }

#endif

