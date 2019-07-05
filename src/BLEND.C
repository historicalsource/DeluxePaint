/*----------------------------------------------------------------------*/
/*  BLEND mode:								*/
/*  									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>

#define local
extern BoxBM temp;
extern BOOL erasing,firstDown;
extern Range *shadeRange;
extern struct TmpRas tmpRas;

local UBYTE *blSmask, *blDmask=0;/* used by blend to point at the mask for the save.bm */

void DoBlend(ob,canv,clip)   BMOB *ob; BoxBM *canv; Box *clip; {		    
    UBYTE *wmask, *tmp;
    if ( BMPlaneSize(ob->pict.bm) > tmpRas.Size/3) return;
    if (SaveSizeBM(temp.bm, ob)) return;
    temp.box = ob->pict.box;
    CopyBoxBM(canv, &temp, &temp.box, clip, REPOP);
    if (firstDown) { 
	blDmask = tmpRas.RasPtr +tmpRas.Size/3;
	blSmask = tmpRas.RasPtr + 2*tmpRas.Size/3;
	}
    MaskRange(temp.bm, blDmask, tmpRas.RasPtr, shadeRange->low, shadeRange->high);
    if (!firstDown) {
	wmask = ob->mask;
	if (shadeRange!=NULL) {
	    wmask = blSmask;
	    BltABCD(blSmask,blDmask,ob->mask,wmask,BMBlitSize(ob->pict.bm),0x80);
	    }
	BlendBM(temp.bm, ob->save.bm, tmpRas.RasPtr);
	MBSaveAtPict(ob,canv,clip,wmask);
	}
    else firstDown = NO;
    tmp = blSmask; blSmask= blDmask; blDmask = tmp;
    RelocBoxBM(&temp,&ob->save); /* save  = temp */
    }

#define yA  	0xf0
#define yB	0xcc
#define yC	0xaa
#define nA	0x0f
#define nB 	0x33
#define nC	0x55
#define A_AND_B   yA&yB

/* sum bit is on if odd number of sources are 1 */
#define ADD_ABC	(yA&nB&nC)|(nA&yB&nC)|(nA&nB&yC)|(yA&yB&yC)

/* carry if at least 2 of three sources are 1's */
#define CARRY_ABC (yA&yB&nC)|(yA&nB&yC)|(nA&yB&yC)|(yA&yB&yC)
#define POST_CARRY_ABC  0xB2	/* black magic.. trust me */

/* b = (a + b)/2 */
BlendBM(a,b,car)  
    struct BitMap *a, *b;
    UBYTE *car;	/* one plane scratch area( carry bit) */
    {
    int i;
    int bpr = a->BytesPerRow;
    int h = a->Rows;
    int depth = a->Depth;
    int bsz = BlitSize(bpr,h);

    /* AND low order bits to get initial carry */
    BltABCD(a->Planes[0], b->Planes[0], NULL, car, bsz, A_AND_B);
    
    for (i=1; i<depth; i++) {
	/* add i'th bit and carry, store as new i-1'th bit */
	BltABCD(a->Planes[i], b->Planes[i], car, b->Planes[i-1],bsz, ADD_ABC );
	/* compute new carry bit */
	BltABCD(a->Planes[i], b->Planes[i], car, 
	    (i == (depth-1))? b->Planes[i] : car, bsz, CARRY_ABC);
	}
    }

