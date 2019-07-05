/*----------------------------------------------------------------------*/
/* ILBMW.C  Support routines for writing ILBM files.           11/06/85 */
/* (IFF is Interchange Format File.)     				*/
/*									*/
/* By Jerry Morrison and Steve Shaw, Electronic Arts.			*/
/* This software is in the public domain.				*/
/*----------------------------------------------------------------------*/
#include "iff\packer.h"
#include "iff\ilbm.h"

/*---------- InitBMHdr -------------------------------------------------*/
IFFP InitBMHdr(bmHdr0, bitmap, masking, compression, transparentColor,
	    pageWidth, pageHeight)
        BitMapHeader *bmHdr0;  struct BitMap *bitmap;
        int masking, compression, transparentColor, pageWidth, pageHeight;
        /*  Masking, Compression, UWORD,            WORD,      WORD  */
    {
    register BitMapHeader *bmHdr = bmHdr0;
    register WORD rowBytes = bitmap->BytesPerRow;

    bmHdr->w = rowBytes << 3;
    bmHdr->h = bitmap->Rows;
    bmHdr->x = bmHdr->y = 0;	/* Default position is (0,0).*/
    bmHdr->nPlanes = bitmap->Depth;
    bmHdr->masking = masking;
    bmHdr->compression = compression;
    bmHdr->pad1 = 0;
    bmHdr->transparentColor = transparentColor;
    bmHdr->xAspect = bmHdr->yAspect = 1;
    bmHdr->pageWidth = pageWidth;
    bmHdr->pageHeight = pageHeight;

    if (pageWidth = 320)
	switch (pageHeight) {
	    case 200: {bmHdr->xAspect = x320x200Aspect;
		       bmHdr->yAspect = y320x200Aspect; break;}
	    case 400: {bmHdr->xAspect = x320x400Aspect;
		       bmHdr->yAspect = y320x400Aspect; break;}
	    }
    else if (pageWidth = 640)
	switch (pageHeight) {
	    case 200: {bmHdr->xAspect = x640x200Aspect;
		       bmHdr->yAspect = y640x200Aspect; break;}
	    case 400: {bmHdr->xAspect = x640x400Aspect;
		       bmHdr->yAspect = y640x400Aspect; break;}
	    }

    return( IS_ODD(rowBytes) ? CLIENT_ERROR : IFF_OKAY );
    }

/*---------- PutCMAP ---------------------------------------------------*/
IFFP PutCMAP(context, colorMap, depth)   
      GroupContext *context;  WORD *colorMap;  UBYTE depth;
   {
   register LONG nColorRegs;   
   IFFP iffp;
   ColorRegister colorReg;

   if (depth > MaxAmDepth)   depth = MaxAmDepth;
   nColorRegs = 1 << depth;

   iffp = PutCkHdr(context, ID_CMAP, nColorRegs * sizeofColorRegister);
   CheckIFFP();

   for ( ;  nColorRegs;  --nColorRegs)  {
      colorReg.red   = ( *colorMap >> 4 ) & 0xf0;
      colorReg.green = ( *colorMap      ) & 0xf0;
      colorReg.blue  = ( *colorMap << 4 ) & 0xf0;
      iffp = IFFWriteBytes(context, (BYTE *)&colorReg, sizeofColorRegister);
      CheckIFFP();
      ++colorMap;
      }

   iffp = PutCkEnd(context);
   return(iffp);
   }

/*---------- PutBODY ---------------------------------------------------*/
/* NOTE: This implementation could be a LOT faster if it used more of the
 * supplied buffer. It would make far fewer calls to IFFWriteBytes (and
 * therefore to DOS Write). */
IFFP PutBODY(context, bitmap, mask, bmHdr, buffer, bufsize)
      GroupContext *context;  struct BitMap *bitmap;  BYTE *mask;
      BitMapHeader *bmHdr;  BYTE *buffer;  LONG bufsize;
   {         
   IFFP iffp;
   LONG rowBytes = bitmap->BytesPerRow;
   int dstDepth = bmHdr->nPlanes;
   Compression compression = bmHdr->compression;
   int planeCnt;		/* number of bit planes including mask */
   register int iPlane, iRow;
   register LONG packedRowBytes;
   BYTE *buf;
   BYTE *planes[MaxAmDepth + 1]; /* array of ptrs to planes & mask */

   if ( bufsize < MaxPackedSize(rowBytes)  ||	/* Must buffer a comprsd row*/
        compression > cmpByteRun1  ||		/* bad arg */
	bitmap->Rows != bmHdr->h   ||		/* inconsistent */
	rowBytes != RowBytes(bmHdr->w)  ||	/* inconsistent*/
	bitmap->Depth < dstDepth   ||		/* inconsistent */
	dstDepth > MaxAmDepth )			/* too many for this routine*/
      return(CLIENT_ERROR);

   planeCnt = dstDepth + (mask == NULL ? 0 : 1);

   /* Copy the ptrs to bit & mask planes into local array "planes" */
   for (iPlane = 0; iPlane < dstDepth; iPlane++)
      planes[iPlane] = (BYTE *)bitmap->Planes[iPlane];
   if (mask != NULL)
      planes[dstDepth] = mask;

   /* Write out a BODY chunk header */
   iffp = PutCkHdr(context, ID_BODY, szNotYetKnown);
   CheckIFFP();

   /* Write out the BODY contents */
   for (iRow = bmHdr->h; iRow > 0; iRow--)  {
      for (iPlane = 0; iPlane < planeCnt; iPlane++)  {

         /* Write next row.*/
         if (compression == cmpNone) {
            iffp = IFFWriteBytes(context, planes[iPlane], rowBytes);
            planes[iPlane] += rowBytes;
            }

         /* Compress and write next row.*/
         else {
            buf = buffer;
            packedRowBytes = PackRow(&planes[iPlane], &buf, rowBytes);
            iffp = IFFWriteBytes(context, buffer, packedRowBytes);
            }

         CheckIFFP();
         }
      }

   /* Finish the chunk */
   iffp = PutCkEnd(context);
   return(iffp);
   }
