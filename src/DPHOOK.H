/*----------------------------------------------------------------------*/
/*  									*/
/*     dphook.h								*/
/*  									*/
/*----------------------------------------------------------------------*/

typedef struct {
    struct BitMap *bitmap;  /* the bitmap to be shared by DPaint */
    struct BitMap *dpScreenBitMap;  /* DPaint's screen bitmap */
    struct ViewPort *vport;	/* DPaint's ViewPort */
    SHORT format;	/* 0: 320x200,  1:640x200, 2: 640x400 */
    SHORT refCount;	/* initialize to zero: don't free bitmap if > 0 */ 
    SHORT version;	/* = 1 for DPaint version 1.0 */
    BMOB *brush;	/* pointer to DPaints brush: for possible
				future enhancements */
    } DPHook;
