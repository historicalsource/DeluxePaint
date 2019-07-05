/*----------------------------------------------------------------------*/
/*  									*/
/*        palbms.c --  Bitmaps for palette  requestor arrows	*/
/*  									*/
/*----------------------------------------------------------------------*/
#include "system.h"

#define STICKY_HEIGHT 18
#define STICKY_HOT_X  1
#define STICKY_HOT_Y  1

/* Arrow plus letters "to".  Cursor during sticky modes.*/
UWORD stickyPointer[STICKY_HEIGHT*2+4] = {
  0x0000, 0x0000,	  /* position and control info*/
     0x0, 0x7C00,   0x7C00, 0xFE00,
  0x7C00, 0x8600,   0x7800, 0x8C00,
  0x7C00, 0x8600,   0x6E00, 0x9300,
   0x700, 0x6980,    0x380,  0x4C0,
   0x1C0,  0x260,     0x80,  0x140,
     0x0,   0x80,      0x0, 0x7E38,
  0x7E38, 0x8144,   0x186C, 0x6692,
  0x1844, 0x24AA,   0x186C, 0x2492,
  0x1838, 0x2444,      0x0, 0x1838, 
  0xFFFF, 0xFFFF  	  /* end markers */ 
    };

UWORD diamond[5] = {
    0x2000,		/* first plane of image */
    0x7000,
    0xf800,
    0x7000,
    0x2000 };	/* forms a diamond pointer */

UWORD diamond2[5] = {
		0x0800,		/* first plane of image */
		0x3e00,
		0xff80,
		0x3e00,
		0x0800 };	/* forms a wider diamond pointer */


