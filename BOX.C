/*--------------------------------------------------------------*/
/*								*/
/*        box.c  -- operations on boxes 			*/
/*								*/
/*--------------------------------------------------------------*/

#include <system.h>
#include <prism.h>

Box *MakeBox(bx,x,y,w,h) Box *bx; int x,y,w,h; { 
    bx->x = x; bx->y = y; bx->w = w; bx->h = h; 
    return(bx);
    }

BOOL BoxContains(b,x,y) Box *b; int x,y; {
    return((BOOL)((x>=b->x)&&(x<(b->x+b->w))&&(y>=b->y) && (y<(b->y+b->h))));
    }

/* intersection of two boxes:  compute c = intersect(a,b)  */
    
BOOL BoxAnd(c,a,b) Box *c,*a,*b; {
    BOOL res = YES;
    SHORT amx,amy,bmx,bmy;
    amx = a->x + a->w;    amy = a->y + a->h;
    bmx = b->x + b->w;    bmy = b->y + b->h;
    c->w = MIN(amx, bmx) - (c->x = MAX(a->x, b->x));
    c->h = MIN(amy, bmy) - (c->y = MAX(a->y, b->y));
    if ( (c->w<=0) || (c->h<=0) ) { c->x = c->y = c->w = c->h = res = 0; }
    return(res);
    }
    
Box *BoxTwoPts(b,x1,y1,x2,y2) Box *b; int x1,y1,x2,y2; {
    ORDER(x1,x2);    ORDER(y1,y2);
    b->x = x1; b->y = y1; b->w = x2-x1+1;  b->h = y2-y1+1;
    return(b);
    }

Box *BoxThreePts(b,x1,y1,x2,y2,x3,y3) Box *b; int x1,y1,x2,y2,x3,y3; {
    ORDER(x1,x2); ORDER(x1,x3); ORDER(x2,x3);
    ORDER(y1,y2); ORDER(y1,y3); ORDER(y2,y3);
    b->x = x1; b->y = y1; b->w = x3-x1+1;  b->h = y3-y1+1;
    return(b);
    }

/* constrain box a to be inside b  */
BoxBeInside(a,b) Box *a,*b; {
    SHORT nout;
    a->x = MAX(b->x,a->x);
    a->y = MAX(b->y,a->y);
    nout = (a->x + a->w) - (b->x + b->w);
    if (nout>0) a->x -= nout;
    nout = (a->y + a->h) - (b->y + b->h);
    if (nout>0) a->y -= nout;
    }

/* make box c bigger so it encloses b */
EncloseBox(c,b) Box *c,*b; {
    SHORT xm,ym;
    if (c->w==0) *c = *b; 	/* w = 0 is null box by convention */
    else {
	xm = MAX(c->x+c->w,b->x+b->w);
	ym = MAX(c->y+c->h,b->y+b->h);
	c->x = MIN(c->x,b->x);
	c->y = MIN(c->y,b->y);
	c->w = xm-c->x;
	c->h = ym-c->y;
	}
    }

