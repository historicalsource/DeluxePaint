/*----------------------------------------------------------------------*
 * IFFW.C  Support routines for writing IFF-85 files.          12/02/85
 * (IFF is Interchange Format File.)
 *
 * By Jerry Morrison and Steve Shaw, Electronic Arts.
 * This software is in the public domain.
 *
 * This version for the Commodore-Amiga computer.
 *----------------------------------------------------------------------*/
#include "iff/iff.h"

/* ---------- IFF Writer -----------------------------------------------*/

/* A macro to test if a chunk size is definite, i.e. not szNotYetKnown.*/
#define Known(size)   ( (size) != szNotYetKnown )

/* Yet another weird macro to make the source code simpler...*/
#define IfIffp(expr)  {if (iffp == IFF_OKAY)  iffp = (expr);}


/* ---------- Generic I/O Speed Up Package -----------------------------*/

#define local /*TBD static*/

local BPTR wFile = NULL;
local BYTE *wBuffer = NULL;
local LONG wNBytes=0;	 /* buffer size in bytes.*/
local LONG wIndex=0;	 /* index of next available byte.*/
local LONG wWaterline=0; /* Count of # bytes to be written.
			  * Different than wIndex because of GSeek.*/

/* ---------- GWriteFlush ----------------------------------------------*/
/* Writes out any data in write buffer above.
 * NOTE WHEN have Seeked into middle of buffer:
 * GWriteFlush causes current position to be the end of the data written.
 * -1 on error from Write.*/
int GWriteFlush() {
    int gWrite = 0;
    if (wFile != NULL  &&  wBuffer != NULL  &&  wIndex > 0)
	gWrite = Write(wFile, wBuffer, wWaterline);
    wWaterline = wIndex = 0;	/* No matter what, make sure this happens.*/
    return( gWrite );
    }

/* ---------- GWriteDeclare --------------------------------------------*/
/* Sets up variables above to describe a write buffer.*/
/* -1 on error from Write.*/
int GWriteDeclare(file, buffer, nBytes)
    BPTR file;  BYTE *buffer;  LONG nBytes; {
    int gWrite = GWriteFlush();
    if ( file==NULL  ||  buffer==NULL  ||  nBytes<=3) {
	wFile = NULL;   wBuffer = NULL;     wNBytes = 0; }
    else {
	wFile = file;   wBuffer = buffer;   wNBytes = nBytes; }
    return( gWrite );
    }

/* ---------- GWrite ---------------------------------------------------*/
/* ANY PROGRAM WHICH USES "GWrite" MUST USE "GSeek" rather than "Seek"
 * TO SEEK ON A FILE BEING WRITTEN WITH "GWrite".
 * "Write" with Generic speed-up.
 * -1 on error from Write.  else returns # bytes written to disk.*/
int GWrite(file, buffer, nBytes)   BPTR file;  BYTE *buffer;  int nBytes; {
    int gWrite = 0;

    if (file == wFile  &&  wBuffer != NULL) {
	if (wNBytes >= wIndex + nBytes) {
	    /* Append to wBuffer.*/
	    movmem(buffer, wBuffer+wIndex, nBytes);
	    wIndex += nBytes;
	    if (wIndex > wWaterline)
		wWaterline = wIndex;
	    nBytes = 0;		/* Indicate data has been swallowed.*/
	    }
	else {
	    wWaterline = wIndex;     /* We are about to overwrite any
		* data above wIndex, up to at least the buffer end.*/
	    gWrite = GWriteFlush();  /* Write data out in proper order.*/
	    }
	}
    if (nBytes > 0  &&  gWrite >= 0)
	gWrite += Write(file, buffer, nBytes);
    return( gWrite );
    }

/* ---------- GSeek ----------------------------------------------------*/
/* "Seek" with Generic speed-up, for a file being written with GWrite.*/
/* Returns what Seek returns, which appears to be the position BEFORE
 * seeking, though the documentation says it returns the NEW position.*/
/* CURRENTLY RETURNS 0 WHEN SEEKING WITHIN THE BUFFER.*/
int GSeek(file, position, mode)
    BPTR file;   BYTE *position;   int mode; {
    int gSeek = -2;
    int newWIndex = wIndex + (int)position;

    if (file == wFile  &&  wBuffer != NULL) {
	if (mode == OFFSET_CURRENT  &&
	    newWIndex >= 0  &&  newWIndex <= wWaterline) {
	    wIndex = newWIndex;
	    gSeek = 0;		/* Okay */
	    }
	else {
	    /* Can't optimize it.*/
	    gSeek = GWriteFlush();
	    if (gSeek >= 0)   gSeek = -2;  /* OK so far */
	    }
	}
    if (gSeek == -2)
	gSeek = Seek(file, position, mode);
    return( gSeek );
    }

/* ---------- OpenWIFF -------------------------------------------------*/

IFFP OpenWIFF(file, new0, limit)  BPTR file; GroupContext *new0; LONG limit; {
    register GroupContext *new = new0;
    register IFFP iffp = IFF_OKAY;

    new->parent       = NULL;
    new->clientFrame  = NULL;
    new->file         = file;
    new->position     = 0;
    new->bound        = limit;
    new->ckHdr.ckID   = NULL_CHUNK;  /* indicates no current chunk */
    new->ckHdr.ckSize = new->bytesSoFar = 0;

    if (0 > Seek(file, 0, OFFSET_BEGINNING))	/* Go to start of the file.*/
	iffp = DOS_ERROR;
    else if ( Known(limit) && IS_ODD(limit) )
	iffp = CLIENT_ERROR;
    return(iffp);
    }

/* ---------- StartWGroup ----------------------------------------------*/
IFFP StartWGroup(parent, groupType, groupSize, subtype, new)
      GroupContext *parent, *new; ID groupType, subtype; LONG groupSize;  {
    register IFFP iffp;

    iffp = PutCkHdr(parent, groupType, groupSize);
    IfIffp( IFFWriteBytes(parent, (BYTE *)&subtype, sizeof(ID)) );
    IfIffp( OpenWGroup(parent, new) );
    return(iffp);
    }

/* ---------- OpenWGroup -----------------------------------------------*/
IFFP OpenWGroup(parent0, new0)  GroupContext *parent0, *new0; {
    register GroupContext *parent = parent0;
    register GroupContext *new    = new0;
    register LONG ckEnd;
    register IFFP iffp = IFF_OKAY;

    new->parent       = parent;
    new->clientFrame  = parent->clientFrame;
    new->file         = parent->file;
    new->position     = parent->position;
    new->bound        = parent->bound;
    new->ckHdr.ckID   = NULL_CHUNK;
    new->ckHdr.ckSize = new->bytesSoFar = 0;

    if ( Known(parent->ckHdr.ckSize) ) {
	ckEnd = new->position + ChunkMoreBytes(parent);
	if ( new->bound == szNotYetKnown || new->bound > ckEnd )
	    new->bound = ckEnd;
	};

    if ( parent->ckHdr.ckID == NULL_CHUNK || /* not currently writing a chunk*/
	 IS_ODD(new->position) ||
         (Known(new->bound) && IS_ODD(new->bound)) )
	iffp = CLIENT_ERROR;
    return(iffp);
    }

/* ---------- CloseWGroup ----------------------------------------------*/
IFFP CloseWGroup(old0)  GroupContext *old0; {
    register GroupContext *old = old0;

    if ( old->ckHdr.ckID != NULL_CHUNK )  /* didn't close the last chunk */
	return(CLIENT_ERROR);
    if ( old->parent == NULL ) {	  /* top level file context */
	/* [TBD] set logical EOF */
	/* [TBD] GWriteEOF(old->file); */
	GWriteFlush();		/* Good enough.*/
	}
    else {
	old->parent->bytesSoFar += old->position - old->parent->position;
	old->parent->position = old->position;
	};
    return(IFF_OKAY);
    }

/* ---------- EndWGroup ------------------------------------------------*/
IFFP EndWGroup(old)  GroupContext *old;  {
    register GroupContext *parent = old->parent;
    register IFFP iffp;

    iffp = CloseWGroup(old);
    IfIffp( PutCkEnd(parent) );
    return(iffp);
    }

/* ---------- PutCk ----------------------------------------------------*/
IFFP PutCk(context, ckID, ckSize, data)
      GroupContext *context; ID ckID; LONG ckSize; BYTE *data; {
    register IFFP iffp = IFF_OKAY;

    if ( ckSize == szNotYetKnown )
	iffp = CLIENT_ERROR;
    IfIffp( PutCkHdr(context, ckID, ckSize) );
    IfIffp( IFFWriteBytes(context, data, ckSize) );
    IfIffp( PutCkEnd(context) );
    return(iffp);
    }

/* ---------- PutCkHdr -------------------------------------------------*/
IFFP PutCkHdr(context0, ckID, ckSize)
      GroupContext *context0;  ID ckID;  LONG ckSize; {
    register GroupContext *context = context0;
    LONG minPSize = sizeof(ChunkHeader); /* physical chunk >= minPSize bytes*/

    /* CLIENT_ERROR if we're already inside a chunk or asked to write
     * other than one FORM, LIST, or CAT at the top level of a file */
    /* Also, non-positive ID values are illegal and used for error codes.*/
    /* (We could check for other illegal IDs...)*/
    if ( context->ckHdr.ckID != NULL_CHUNK  ||  ckID <= 0 )
	return(CLIENT_ERROR);
    else if (context->parent == NULL)  {
	switch (ckID)  {
	    case FORM:  case LIST:  case CAT:  break;
	    default: return(CLIENT_ERROR);
	    }
	if (context->position != 0)
	    return(CLIENT_ERROR);
	}

    if ( Known(ckSize) ) {
	if ( ckSize < 0 )
	    return(CLIENT_ERROR);
	minPSize += ckSize;
	};
    if ( Known(context->bound)  &&
         context->position + minPSize > context->bound )
	return(CLIENT_ERROR);

    context->ckHdr.ckID   = ckID;
    context->ckHdr.ckSize = ckSize;
    context->bytesSoFar   = 0;
    if (0 > GWrite(context->file, &context->ckHdr, sizeof(ChunkHeader)))
	return(DOS_ERROR);
    context->position += sizeof(ChunkHeader);
    return(IFF_OKAY);
    }

/* ---------- IFFWriteBytes ---------------------------------------------*/
IFFP IFFWriteBytes(context0, data, nBytes)
      GroupContext *context0;  BYTE *data;  LONG nBytes; {
    register GroupContext *context = context0;

    if ( context->ckHdr.ckID == NULL_CHUNK  ||	/* not in a chunk */
	 nBytes < 0  ||				/* negative nBytes */
	 (Known(context->bound)  &&		/* overflow context */
	    context->position + nBytes > context->bound)  ||
	 (Known(context->ckHdr.ckSize)  &&   	/* overflow chunk */
	    context->bytesSoFar + nBytes > context->ckHdr.ckSize) )
	return(CLIENT_ERROR);

    if (0 > GWrite(context->file, data, nBytes))
	return(DOS_ERROR);

    context->bytesSoFar += nBytes;
    context->position   += nBytes;
    return(IFF_OKAY);
    }

/* ---------- PutCkEnd -------------------------------------------------*/
IFFP PutCkEnd(context0)  GroupContext *context0; {
    register GroupContext *context = context0;
    WORD zero = 0;	/* padding source */

    if ( context->ckHdr.ckID == NULL_CHUNK )  /* not in a chunk */
	return(CLIENT_ERROR);

    if ( context->ckHdr.ckSize == szNotYetKnown ) {
	/* go back and set the chunk size to bytesSoFar */
	if ( 0 > GSeek(context->file,
		      -(context->bytesSoFar + sizeof(LONG)),
		      OFFSET_CURRENT)  ||
	     0 > GWrite(context->file, &context->bytesSoFar, sizeof(LONG))  ||
	     0 > GSeek(context->file, context->bytesSoFar, OFFSET_CURRENT)  )
	    return(DOS_ERROR);
	}
    else {  /* make sure the client wrote as many bytes as planned */
	if ( context->ckHdr.ckSize != context->bytesSoFar )
	    return(CLIENT_ERROR);
	};

    /* Write a pad byte if needed to bring us up to an even boundary.
     * Since the context end must be even, and since we haven't
     * overwritten the context, if we're on an odd position there must
     * be room for a pad byte. */
    if ( IS_ODD(context->bytesSoFar) ) {
	if ( 0 > GWrite(context->file, &zero, 1) )
	    return(DOS_ERROR);
	context->position += 1;
	};

    context->ckHdr.ckID   = NULL_CHUNK;
    context->ckHdr.ckSize = context->bytesSoFar = 0;
    return(IFF_OKAY);
    }

