/*----------------------------------------------------------------------*/
/* ILBMR.C  Support routines for reading ILBM files.           11/07/85 */
/* (IFF is Interchange Format File.)     				*/
/*									*/
/* By Jerry Morrison and Steve Shaw, Electronic Arts.			*/
/* This software is in the public domain.				*/
/*----------------------------------------------------------------------*/
#include "iff\packer.h"
#include "iff\ilbm.h"


/* ---------- GetCMAP ------------------------------------------------*/
/* pNColorRegs is passed in as a pointer to the number of ColorRegisters
 * caller has space to hold.  GetCMAP sets to the number actually read.*/
IFFP GetCMAP(ilbmContext, colorMap, pNColorRegs)   
      GroupContext *ilbmContext;  WORD *colorMap;  UBYTE *pNColorRegs;
   {
   register int nColorRegs;   
   register IFFP iffp;
   ColorRegister colorReg;

   nColorRegs = ilbmContext->ckHdr.ckSize / sizeofColorRegister;
   if (*pNColorRegs < nColorRegs)   nColorRegs = *pNColorRegs;
   *pNColorRegs = nColorRegs;	/* Set to the number actually there.*/

   for ( ;  nColorRegs > 0;  --nColorRegs)  {
      iffp = IFFReadBytes(ilbmContext, (BYTE *)&colorReg,sizeofColorRegister);
      CheckIFFP();
      *colorMap++ = ( ( colorReg.red   >> 4 ) << 8 ) |
		    ( ( colorReg.green >> 4 ) << 4 ) |
		    ( ( colorReg.blue  >> 4 )      );
      }
   return(IFF_OKAY);
   }

/*---------- GetBODY ---------------------------------------------------*/
/* NOTE: This implementation could be a LOT faster if it used more of the
 * supplied buffer. It would make far fewer calls to IFFReadBytes (and
 * therefore to DOS Read) and to movemem. */
IFFP GetBODY(context, bitmap, mask, bmHdr, buffer, bufsize)
      GroupContext *context;  struct BitMap *bitmap;  BYTE *mask;
      BitMapHeader *bmHdr;  BYTE *buffer;  LONG bufsize;
   {
   register IFFP iffp;
   UBYTE srcPlaneCnt = bmHdr->nPlanes;   /* Haven't counted for mask plane yet*/
   LONG srcRowBytes = RowBytes(bmHdr->w);
   LONG bufRowBytes = MaxPackedSize(srcRowBytes);
   int nRows = bmHdr->h;
   Compression compression = bmHdr->compression;
   register int iPlane, iRow, nEmpty, nFilled;
   BYTE *buf, *nullDest, *nullBuf, **pDest;
   BYTE *planes[MaxSrcPlanes]; /* array of ptrs to planes & mask */

   if (compression > cmpByteRun1)
      return(CLIENT_ERROR);

   /* Complain if client asked for a conversion GetBODY doesn't handle.*/
   if ( srcRowBytes  !=  bitmap->BytesPerRow  ||
         bufsize < bufRowBytes * 2  ||
         srcPlaneCnt > MaxSrcPlanes )
      return(CLIENT_ERROR);

   if (nRows > bitmap->Rows)
      nRows = bitmap->Rows;
   
   /* Initialize array "planes" with bitmap ptrs; NULL in empty slots.*/
   for (iPlane = 0; iPlane < bitmap->Depth; iPlane++)
      planes[iPlane] = (BYTE *)bitmap->Planes[iPlane];
   for ( ;  iPlane < MaxSrcPlanes;  iPlane++)
      planes[iPlane] = NULL;

   /* Copy any mask plane ptr into corresponding "planes" slot.*/
   if (bmHdr->masking == mskHasMask) {
      if (mask != NULL)
         planes[srcPlaneCnt] = mask;  /* If there are more srcPlanes than
               * dstPlanes, there will be NULL plane-pointers before this.*/
      else
         planes[srcPlaneCnt] = NULL;  /* In case more dstPlanes than src.*/
      srcPlaneCnt += 1;  /* Include mask plane in count.*/
      }

   /* Setup a sink for dummy destination of rows from unwanted planes.*/
   nullDest = buffer;
   buffer  += srcRowBytes;
   bufsize -= srcRowBytes;

   /* Read the BODY contents into client's bitmap.
    * De-interleave planes and decompress rows.
    * MODIFIES: Last iteration modifies bufsize.*/
   buf = buffer + bufsize;  /* Buffer is currently empty.*/
   for (iRow = nRows; iRow > 0; iRow--)  {
      for (iPlane = 0; iPlane < srcPlaneCnt; iPlane++)  {

         pDest = &planes[iPlane];

         /* Establish a sink for any unwanted plane.*/
         if (*pDest == NULL) {
	    nullBuf = nullDest;
            pDest   = &nullBuf;
            }

         /* Read in at least enough bytes to uncompress next row.*/
         nEmpty  = buf - buffer;	  /* size of empty part of buffer.*/
         nFilled = bufsize - nEmpty;	  /* this part has data.*/
	 if (nFilled < bufRowBytes) {
	    /* Need to read more.*/

	    /* Move the existing data to the front of the buffer.*/
	    /* Now covers range buffer[0]..buffer[nFilled-1].*/
            movmem(buf, buffer, nFilled);  /* Could be moving 0 bytes.*/

            if (nEmpty > ChunkMoreBytes(context)) {
               /* There aren't enough bytes left to fill the buffer.*/
               nEmpty = ChunkMoreBytes(context);
               bufsize = nFilled + nEmpty;  /* heh-heh */
               }

	    /* Append new data to the existing data.*/
            iffp = IFFReadBytes(context, &buffer[nFilled], nEmpty);
            CheckIFFP();

            buf     = buffer;
	    nFilled = bufsize;
	    nEmpty  = 0;
	    }

         /* Copy uncompressed row to destination plane.*/
         if (compression == cmpNone) {
            if (nFilled < srcRowBytes)  return(BAD_FORM);
	    movmem(buf, *pDest, srcRowBytes);
	    buf    += srcRowBytes;
            *pDest += srcRowBytes;
            }
	 else
         /* Decompress row to destination plane.*/
            if ( UnPackRow(&buf, pDest, nFilled,  srcRowBytes) )
                    /*  pSource, pDest, srcBytes, dstBytes  */
               return(BAD_FORM);
         }
      }

   return(IFF_OKAY);
   }

