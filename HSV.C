/*--------------------------------------------------------------------*/
/*
	RGB - HSV Conversion 
	
	Author:		Gordon Knopes.

	Calling Seq.:	rgb2hsv (src, dst)
	Description:	Convert RGB color to HSV.
	Input:		src =	Pointer to source RGB color structure.
			dst =	Pointer to source HSV color structure.
	Returns:	Nothing.


	Calling Seq.:	hsv2rgb (src, dst)
	Description:	Convert HSV color to RGB.
	Input:		src =	Pointer to source HSV color structure.
			dst =	Pointer to source RGB color structure.
	Returns:	Nothing.

	Creation Date:	8/6/85						*/	
/*----------------------------------------------------------------------*/

#include <exec\types.h>
#define local static

#define SCLEXP 8
	/* normalize factor = 256			*/

#define MAXINT 0xefff
#define MAXRGB 15
#define MAXH 255
	/* normalized max h = 6 << SCLEXP		*/
#define	MAXSV 255
#define SCLH ((6 << SCLEXP) / MAXH)

#define LMAXRGB 15L
#define LMAXSV 255L

#define LROUND 0x80L
	/* (1 << (SCLEXP - 1))L				*/
#define UNDEFINED -1
#define F1 (1 << SCLEXP)
#define F2 (2 << SCLEXP)
#define F4 (4 << SCLEXP)

typedef struct { SHORT r, g, b; } rgb;
typedef struct { SHORT h, s, v; } hsv;

local long r,g,b;
local long tmp;

local int mul(a, b) int a, b;    {
    tmp = ((long) a * (long) b) >> SCLEXP;
    return ((int) tmp);
    }

local int div(a, b) int a, b;   {
    if (b == 0)	tmp = MAXINT;
    else tmp = ((long) a << SCLEXP) / (long) b;
    return ((int) tmp);
    }

/*	assign triplet to RGB.	*/
local void triplet(x, y, z) int x, y, z;    {
    r = x;    g = y;    b = z;
    }

local int normrgb(x) SHORT x;    {
    tmp = ((long) x << SCLEXP) / LMAXRGB;
    return ((int) tmp);
    }

local SHORT unnormrgb(x) int x;    {
    tmp = ((long) x * LMAXRGB + LROUND) >> SCLEXP;
    return ((SHORT) tmp);
    }

local int normsv(x) SHORT x;    {
    tmp = ((long) x << SCLEXP) / LMAXSV;
    return ((int) tmp);
    }

local SHORT unnormsv(x) int x;    {
    tmp = ((long) x * LMAXSV + LROUND) >> SCLEXP;
    if (tmp > LMAXSV)	tmp = LMAXSV;
    return ((SHORT) tmp);
    }

local int normh(h) SHORT h;    {
    if (h >= MAXH) h = 0;
    return ((int) (h * SCLH) ); 	/* h is now [0, 6] */
    }

local SHORT unnormh(h) int h;    {
    h /= SCLH;	/* convert to degrees */
    if (h < 0)		/* make nonnegative */
	h +=MAXH;
    if (h >= MAXH)    	h = 0;
    return ((SHORT) h);
    }

/*----------------------------------------------------------------------*/
/*
    RGB_TO_HSV.
	    
    Given: r, g, b, each in [0, 255]
    Desired: h, s and v in [0, 255], except if s = 0,
    then h = UNDEFINED which is a definrd constant not in [0, 255].
									*/
/*----------------------------------------------------------------------*/
    
rgb2hsv(src, dst) rgb *src; hsv *dst;    {
    int h, s, v, rc, gc, bc, max, min, diff;

    r = normrgb (src->r);
    g = normrgb (src->g);
    b = normrgb (src->b);
    max = (r > g) ? ((r > b) ? r : b) : (g > b) ? g : b;
    min = (r < g) ? ((r < b) ? r : b) : (g < b) ? g : b;
    v = max;			/* value */
    diff = div (F1, max-min);
    if (max != 0)
	s = div (F1, mul (diff, max));	/* saturation */
    else s = 0;
    s = (int) unnormsv(s);
    if (s == 0)
	h = (SHORT) UNDEFINED;
    else
	{			/* saturation not zero, so determine hue */
	rc = mul (max - r, diff); /* rc measures distance of color from red */
	gc = mul (max - g,  diff);
	bc = mul (max - b, diff);
	if (r == max)
	    h = bc - gc;	/* color between yellow & magenta */
	else
	    {
	    if (g == max)
		h = F2 + rc - bc; /* color between cyan & yellow */
	    else
		{
		if (b == max)
		    h = F4 + gc - rc; /* color between magenta & cyan */
		}
	    }
	h = unnormh(h);
	}	/* chromatic case */
    dst->h = h;
    dst->s = (SHORT) s;
    dst->v = unnormsv (v);
    
    if (dst->h < 3)	/* Rgb2hsv doesn't return 0. */
	dst->h = 0;
    if (dst->s < 3)
	dst->s = 0;
    if (dst->s < 3)
	dst->s = 0;
    }

/*----------------------------------------------------------------------*/
/*
	HSV_TO_RGB.
	    
	Given: h, s and v in [0, 255]
	Desired: r, g, b, each in [0, 255].				*/
/*----------------------------------------------------------------------*/
hsv2rgb(src, dst) hsv *src; rgb *dst;    {
    int h, s, v, f, p, q, t;
    int i;

    if (src->h >= MAXH)
	src->h = 0;
    h = (int) src->h;
    s = normsv (src->s);
    v = normsv (src->v);
    if (s == 0)
	triplet (v, v, v);	/* achromatic case */
/*
	{			* achromatic color: no hue *
	if (h < 0)
	    triplet (v, v, v);	* achromatic case *
	else
	    {
	    printf ("\n*** error (if s = 0 and h has value) ***\n");
	    exit (0);
	    }
	}
*/
    else
	{				/* chromatic color: hue */
	h = normh(h);
	i = (int) (h >> SCLEXP);	/* largest integer <= h */
	f = h - (i << SCLEXP);		/* fractional part of h */
	p = mul (v, F1 - s);
	q = mul (v, F1 - mul (s, f));
	t = mul (v, F1 - mul (s, F1 - f));
	switch (i)
	    {				/* triplet assignment */
	    case 0: triplet (v, t, p);
		    break;
	    case 1: triplet (q, v, p);
		    break;
	    case 2: triplet (p, v, t);
		    break;
	    case 3: triplet (p, q, v);
		    break;
	    case 4: triplet (t, p, v);
		    break;
	    case 5: triplet (v, p, q);
	    }	/* case */
	}	/* hue */
    dst->r = unnormrgb (r);
    dst->g = unnormrgb (g);
    dst->b = unnormrgb (b);

    }
