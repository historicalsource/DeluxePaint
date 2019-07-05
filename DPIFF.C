/** dpiff.c ******************************************************************/
/*                                                                           */
/* DPAINT IFF file I/O procedures                                            */
/*                                                                           */
/* Date      Who Changes                                                     */
/* --------- --- ----------------------------------------------------------- */
/* 01-Sep-85 mrp created                                                     */
/* 27-Sep-85 jhm & sss rewrote using ilbm.h, packer.h, & iff.h               */
/* 01-Oct-85 dds  modified for DPaint.                                       */
/* 07-Nov-85 sss  Dive into non-ILBM FORMs looking for an ILBM.              */
/*****************************************************************************/
#include "system.h"
#include "prism.h"
#include "dpiff.h" 

#define MaxDepth 5
static IFFP ifferror = 0;

extern Box screenBox;

#define CkErr(expression)  {if (ifferror == IFF_OKAY) ifferror = (expression);}
    
/*****************************************************************************/
/* IffErr                                                                    */
/*                                                                           */
/* Returns the iff error code and resets it to zero                          */
/*                                                                           */
/*****************************************************************************/
IFFP IffErr()
   {
   IFFP i;
   i = ifferror;
   ifferror = 0;
   return(i);
   }

/*------------ ILBM reader -----------------------------------------------*/
/* DVCS' number of planes in a bitmap. Could use MaxAmDepth. */

/* Here's our "client frame" for reading an IFF file, looking for ILBMs.
 * We use it to stack BMHD & CMAP properties. */
typedef struct {
   ClientFrame clientFrame;
   UBYTE foundBMHD;
   UBYTE nColorRegs;
   BitMapHeader bmHdr;
   Color4 colorMap[1<<MaxDepth];
   /* [TBD] Place to store any other shared PROPs encountered in a LIST.*/
   } ILBMFrame;


/* GetPicture communicates with GetFoILBM via these.*/
static struct BitMap *gpBitmap = NULL;
static WORD *gpColorMap = NULL;
static BYTE *gpBuffer = NULL;
static LONG gpBufsize = 0;
static BOOL picNotFit = FALSE;

#if 0
prbmHdr(bmHdr)  BitMapHeader *bmHdr; {
    printf("bmHdr->w %ld  ",bmHdr->w);  printf("bmHdr->h %ld\n",bmHdr->h);
    printf("bmHdr->x %ld  ",bmHdr->x);   printf("bmHdr->y %ld\n",bmHdr->y);
    printf("bmHdr->depth %ld  ",bmHdr->depth);  printf("bmHdr->nPlanes %ld\n",bmHdr->nPlanes);
    printf("bmHdr->masking %ld  ",bmHdr->masking);
    printf("bmHdr->pixelFormat %ld\n",bmHdr->pixelFormat);
    printf("bmHdr->compression %ld\n",bmHdr->compression);
    printf("bmHdr->pad1 %ld",bmHdr->pad1);
    printf("bmHdr->transparentColor %ld\n",bmHdr->transparentColor);
    }
#endif

/*----------------------------------------------------------------------*/
/*									*/
/* MBMGetFoILBM()							*/
/* Used by GetMaskBM to handle every FORM in an IFF file.		*/
/* Reads FORM ILBMs and skips all other FORMs.				*/
/* Inside a FORM ILBM,							*/
/* ILBMs with no BODY.							*/
/* 									*/
/* Once we find a BODY chunk, we'll start reading into client's BitMap. */
/* if it turns out to be malformed, we'll be sorry.  			*/
/*----------------------------------------------------------------------*/
static MaskBM *gpMaskBM;
static SHORT rngCnt;
ResizeProc gpReSize;

IFFP MBMGetFoILBM(parent)  GroupContext *parent; {
    /*compilerBug register*/ IFFP iffp;
    GroupContext formContext;
    ILBMFrame ilbmFrame;
    register int i;
    BOOL resizeFail;

    /* Dive into any non-ILBM FORM looking for an ILBM.*/
    if (parent->subtype != ID_ILBM) {
	/* Open a non-ILBM FORM.*/
	iffp = OpenRGroup(parent, &formContext);
	CheckIFFP();
	do {
	    iffp = GetF1ChunkHdr(&formContext);
	    } while (iffp >= IFF_OKAY);
	if (iffp == END_MARK)
	    iffp = IFF_OKAY;	/* then continue scanning the file */
	CloseRGroup(&formContext);
	return(iffp);
	}

    /* Open an ILBM FORM.*/
    iffp = OpenRGroup(parent, &formContext);
    CheckIFFP();

    /* Place to store any ILBM properties that are found, starting from
     * any values found in ancestors.*/
    ilbmFrame = *(ILBMFrame *)parent->clientFrame;

    do switch (iffp = GetFChunkHdr(&formContext)) {
	case ID_BMHD: {
	    ilbmFrame.foundBMHD = TRUE;
            iffp = GetBMHD(&formContext, &ilbmFrame.bmHdr);
	    break; }
	case ID_CMAP: {
	    ilbmFrame.nColorRegs = 1 << MaxDepth;  /* we have room for this many */
	    iffp = GetCMAP(
		&formContext, (WORD *)&ilbmFrame.colorMap, &ilbmFrame.nColorRegs);
	    gpMaskBM->flags |= MBM_HAS_CMAP;
	    break; 
	    }
	case ID_GRAB: {
	    iffp = IFFReadBytes(&formContext, (BYTE *)&gpMaskBM->grab, 
		    sizeof(Point2D));
	    gpMaskBM->flags |= MBM_HAS_GRAB;
	    break; }

    	case ID_CRNG:
	    if (( gpMaskBM->ranges != NULL) && (rngCnt<gpMaskBM->nRange)) 
		iffp = IFFReadBytes(&formContext, 
		    (BYTE *)&gpMaskBM->ranges[rngCnt++], sizeof(Range));
	    break; 
	
	case ID_BODY: {
	    if (!ilbmFrame.foundBMHD)  return(BAD_FORM);

	    gpMaskBM->pos.x = ilbmFrame.bmHdr.x;
	    gpMaskBM->pos.y = ilbmFrame.bmHdr.y;
	    gpMaskBM->xAspect = ilbmFrame.bmHdr.xAspect;
	    gpMaskBM->yAspect = ilbmFrame.bmHdr.yAspect;
	    gpMaskBM->masking = ilbmFrame.bmHdr.masking;
	    gpMaskBM->xpcolor = ilbmFrame.bmHdr.transparentColor;

	    if (gpMaskBM->grab.x > ilbmFrame.bmHdr.w)
		gpMaskBM->grab.x = ilbmFrame.bmHdr.w;
	    if (gpMaskBM->grab.y > ilbmFrame.bmHdr.h)
		gpMaskBM->grab.y = ilbmFrame.bmHdr.h;

         /*  Compare bmHdr dimensions with gpBitmap dimensions! */
	    if( (RowBytes(ilbmFrame.bmHdr.w) != gpBitmap->BytesPerRow)||
	    (ilbmFrame.bmHdr.h != gpBitmap->Rows) ||
	    (ilbmFrame.bmHdr.nPlanes != gpBitmap->Depth) ) 		{
		/* didn't fit: must resize the destination */
		resizeFail = (*gpReSize)(gpMaskBM,&ilbmFrame.bmHdr); 
		if (resizeFail ) return(BAD_FIT);
		}
	    
         iffp = GetBODY(
	    &formContext, gpMaskBM->bitmap, gpMaskBM->mask, 
	    &ilbmFrame.bmHdr, gpBuffer, gpBufsize);
         if (iffp == IFF_OKAY) iffp = IFF_DONE;	/* Eureka */
         break; }
      case END_MARK: { iffp = BAD_FORM; break; } /* No BODY chunk! */
      } while (iffp >= IFF_OKAY);  /* loop if valid ID of ignored chunk or a
			  * subroutine returned IFF_OKAY, i.e., no errors.*/

   if (iffp != IFF_DONE)  return(iffp);
   CloseRGroup(&formContext);

   /* It's a good ILBM, return the properties to the client.*/

   /* Copy CMAP into client's storage.*/
   /* nColorRegs is 0 if no CMAP;  no data will be copied to gpColorMap.
    * That's ok; client is left with whatever colors she had before.*/
    if (gpColorMap!=NULL)   for (i = 0;  i < ilbmFrame.nColorRegs;  i++)
      gpColorMap[i] = ilbmFrame.colorMap[i];
   return(iffp);
   }

/*****************************************************************************/
/* MBMGetLiILBM()                                                            */
/* Used by GetMaskBM to handle every LIST in an IFF file.                    */
/*                                                                           */
/*****************************************************************************/
IFFP MBMGetLiILBM(parent)  GroupContext *parent; {
    ILBMFrame newFrame;	/* allocate a new Frame */

    newFrame = *(ILBMFrame *)parent->clientFrame;  /* copy parent frame */

    return( ReadIList(parent, (ClientFrame *)&newFrame) );
    }

/*****************************************************************************/
/* MBMGetPrILBM()                                                            */
/* Used by GetMaskBM to handle every PROP in an IFF file.                    */
/* Reads PROP ILBMs and skips all others.                                    */
/*                                                                           */
/*****************************************************************************/
IFFP MBMGetPrILBM(parent)  GroupContext *parent; {
   /*compilerBug register*/ IFFP iffp;
   GroupContext propContext;
   ILBMFrame *ilbmFrame = (ILBMFrame *)parent->clientFrame;

   iffp = OpenRGroup(parent, &propContext);
   CheckIFFP();

   switch (parent->subtype)  {

      case ID_ILBM: {
         do switch (iffp = GetPChunkHdr(&propContext)) {
            case ID_BMHD: {
              ilbmFrame->foundBMHD = TRUE;
              iffp = GetBMHD(&propContext, &ilbmFrame->bmHdr);
              break; }
            case ID_CMAP: {
              ilbmFrame->nColorRegs = 1 << MaxDepth; /* room for this many */
              iffp = GetCMAP(&propContext, (WORD *)&ilbmFrame->colorMap,
			     &ilbmFrame->nColorRegs);
	      gpMaskBM->flags |= MBM_HAS_CMAP;
              break; }
            } while (iffp >= IFF_OKAY);  /* loop if valid ID of ignored chunk
                     * or a subroutine returned IFF_OKAY, i.e., no errors.*/
         break;
         }

      default: iffp = IFF_OKAY;        /* just continue scanning the file */
      }

   CloseRGroup(&propContext);
   return(iffp == END_MARK ? IFF_OKAY : iffp);
   }

/*****************************************************************************/
/* GetMaskBM()                                                              */
/*                                                                           */
/* Get a picture from an IFF file                                            */
/*                                                                           */
/*****************************************************************************/
BOOL GetMaskBM(file, maskBM, colorMap, reSize, buffer, bufsize)
      LONG file;  MaskBM *maskBM;  WORD *colorMap;
	ResizeProc reSize; BYTE *buffer;  LONG bufsize;
    {
    ILBMFrame iFrame;
    iFrame.clientFrame.getList = &MBMGetLiILBM;
    iFrame.clientFrame.getProp = &MBMGetPrILBM;
    iFrame.clientFrame.getForm = &MBMGetFoILBM;
    iFrame.foundBMHD = FALSE;
    iFrame.nColorRegs = 0;
    
    gpReSize = reSize;
    gpMaskBM = maskBM;
    gpBitmap = maskBM->bitmap;
    gpColorMap = colorMap;
    gpBuffer = buffer;
    gpBufsize = bufsize;
    rngCnt = 0;
    picNotFit = FALSE;
    maskBM->grab.x = maskBM->grab.y = 0;

    ifferror = ReadIFF(file, (ClientFrame *)&iFrame);
    return( (BOOL)(ifferror != IFF_DONE) );
    }    

/*****************************************************************************/
/* PutMaskBM()                                                               */
/*                                                                           */
/* Put a picture into an IFF file                                            */
/* This procedure is specific to VCS. It writes an entire BitMap.            */
/* Pass in mask == NULL for no mask.                                         */
/*                                                                           */
/* Buffer should be big enough for one packed scan line                      */
/*                                                                           */
/*****************************************************************************/
BOOL dorng = YES;
    
BOOL PutMaskBM(file, maskBM, colorMap, buffer, bufsize)
      LONG file; MaskBM *maskBM;  WORD *colorMap;
      BYTE *buffer;  LONG bufsize;

    {
    BitMapHeader bmHdr;
    GroupContext fileContext, formContext;
    IFFP tmpifferror;
    int i;
	    
    ifferror = InitBMHdr(&bmHdr,
	maskBM->bitmap, 
	(int)maskBM->masking, 
	(maskBM->w <= 64)? (int)cmpNone : (int)cmpByteRun1, 
	(int)maskBM->xpcolor,
	(int)screenBox.w, 
	(int)screenBox.h );
	
    bmHdr.x = maskBM->pos.x;   
    bmHdr.y = maskBM->pos.y;
    bmHdr.w = maskBM->w;

#define BODY_BUFSIZE 512

    if (bufsize > 2*BODY_BUFSIZE) {
	GWriteDeclare(file, buffer+BODY_BUFSIZE, bufsize-BODY_BUFSIZE);
	bufsize = BODY_BUFSIZE;
	}
    
    CkErr(OpenWIFF(file, &fileContext, szNotYetKnown) );
    CkErr(StartWGroup(&fileContext, FORM, szNotYetKnown, ID_ILBM, &formContext) );

    CkErr(PutCk(&formContext, ID_BMHD, sizeof(BitMapHeader), (BYTE *)&bmHdr));

    if (colorMap!=NULL)
	CkErr( PutCMAP(&formContext, colorMap, (UBYTE)maskBM->bitmap->Depth) );
    if (maskBM->flags&MBM_HAS_GRAB) {
	tmpifferror = PutCk(&formContext, ID_GRAB, sizeof(Point2D), 
	    (BYTE *)&maskBM->grab); 
	CkErr(tmpifferror); 
	}
    if (maskBM->flags&MBM_HAS_RANGES) {
	for (i=0; i<maskBM->nRange; i++) {
	    tmpifferror = PutCk(&formContext, ID_CRNG, sizeof(Range),
		(BYTE *)&maskBM->ranges[i]);
	    CkErr(tmpifferror); 
	    }
	}
    tmpifferror =  PutBODY(&formContext, maskBM->bitmap, 
	(BYTE *)((maskBM->masking == mskHasMask)? maskBM->mask: NULL),
	&bmHdr, buffer, bufsize);
    CkErr(tmpifferror); 
    
    CkErr( EndWGroup(&formContext) );
    CkErr( CloseWGroup(&fileContext) );
    GWriteDeclare(NULL, NULL, 0);
    return( (BOOL)(ifferror != IFF_OKAY) );
    }    


