/*----------------------------------------------------------------------*/
/* Palette.c: A window used for modifying a color palette.      	*/
/* Usage: error = PaletteTool(struct Window *, PaletteGlobals *);       */
/* Returns 0 if window moved; client should call again.			*/
/* Returns -1 if couldn't allocate enough RAM.				*/
/*									*/
/*  8/28/85 Gordon Knopes   Created.					*/
/* 11/02/85 Steve Shaw      New layout; many internal changes; less	*/
/*			    static RAM; added Spread,Copy,Range,Cancel. */
/*----------------------------------------------------------------------*/
#include "system.h"

extern struct	Window *OpenWindow();
extern struct	IntuiMessage *GetMsg();
extern SHORT	AddGadget();
extern void	RefreshGadgets(),
		ReplyMsg(),
		SetRGB4(),
		CloseWindow(),
		SetRast();

#define local /*TBD*/ static /**/

#define MIN(a,b)   	((a) < (b) ? (a) : (b))
#define MAX(a,b)   	((a) > (b) ? (a) : (b))
#define ABS(a)		((a) >= 0  ? (a) : -(a))

#define ALLOC(n)	AllocMem(n, MEMF_PUBLIC | MEMF_CLEAR)
#define FREE(addr, n)	if (addr)   FreeMem(addr, n)
#define DEPTH 5

#define LIM_SLIDERS 7	/* Includes SPEED */
#define MODIFYRED 0
#define MODIFYGREEN 1
#define MODIFYBLUE 2
#define MODIFYHUE 3
#define MODIFYSATURATION 4
#define MODIFYVALUE 5
#define MODIFYSPEED 6

#define LIM_COLORS 32

#define LIM_RANGES 4	/* 4 cycle-ranges */
#define SH_RANGE   0	/* Shade Paint */
#define C1_RANGE   1	/* Color Cycle 1 */
#define C2_RANGE   2	/* Color Cycle 2 */
#define C3_RANGE   3	/* Color Cycle 3 */

#define trans(val)	(val * (SH - 8))
#define inRange(a,low,width)   ( ((a) >= (low)) && ((a) < (low+width)) )

		/* starting gadget number for gadget id's */

#define COLORGADGETS	0x10	/* Leave a few lower ids available.  Why? */
#define SLIDERGADGETS	(COLORGADGETS + LIM_COLORS)
#define CANCEL_GADGET	(SLIDERGADGETS + LIM_SLIDERS)
#define OK_GADGET	(CANCEL_GADGET + 1)
#define SPREAD_GADGET	(OK_GADGET + 1)
#define COPY_GADGET	(OK_GADGET + 2)
#define RANGE_GADGET	(OK_GADGET + 3)
#define MIX_GADGET	(OK_GADGET + 4)
#define EXCHANGE_GADGET	(OK_GADGET + 5)
#define UNDO_GADGET	(OK_GADGET + 6)
#define RANGE_GADGETS	(UNDO_GADGET + 1)
#define LIM_GADGETS	(RANGE_GADGETS + LIM_RANGES)  /* End of gadget list.*/


#define TXHEIGHT 8

#define RSCALE (TXHEIGHT + 2)
#define TWSCALE (TXHEIGHT + 1)	/* Width per character */
#define THSCALE (TXHEIGHT + 4)	/* Height per boxed character */

#define SXRGB (15)
#define SXHSV (SXRGB + 2)
#define SY    (RSCALE + 9)
#define SH    (60+10)
#define SDX   12	/* Separation between sliders.*/

local int sdx = SDX;
local int sw = SDX;

#define IDCMPFLAGS GADGETUP | GADGETDOWN | MOUSEBUTTONS | RAWKEY
    /* "MOUSEBUTTONS" to do DrawWhich on window reentry when button in limbo.*/
    /* "RAWKEY" so can do "u" undo-key.*/
#define UCODE 0x16	/* RAWKEY for the letter "u".*/
#define DOWNFLAG 0x80	/* Down-transition of a key.*/

#define PX (SXRGB + 6 * SDX + 6)	/* Palette's X,Y,W,H */
#define PY (RSCALE + 13)
#define PW 10
#define PH 8
#define PHs 8	/* # color squares per column. (Numbered from the top.) */
#define PWs 4	/* # color squares per row. */

#define OKFLAGS        GADGHCOMP, RELVERIFY | ENDGADGET,    BOOLGADGET
#define DFFLAGS        GADGHCOMP, RELVERIFY,                BOOLGADGET
#define STICKY_FLAGS   GADGHCOMP, RELVERIFY, BOOLGADGET

#define COMX      5		/* Command Area's X,Y,H */
#define COMY      (PY + PHs * PH + 4)
#define COMH      (4 * THSCALE + 6)

#define RW        (PX + PWs * PW + 4)	/* Requestor Window's Width */
#define RH        (COMY + COMH)	/* Requestor Window's Height */

#define OKX       (RW - 3 * TWSCALE + 4)
#define OKY       (COMY + 3 * THSCALE + 5)
#define OKH       (RSCALE-1)

#define CANCELX   COMX
#define UNDOX     (CANCELX + 7 * TWSCALE - 2)

#define COPYX     (OKX - 2 * TWSCALE)
#define EXCHANGEX (COPYX - 2 * TWSCALE - 2)
#define SPREADX   (COMX + TWSCALE)
#define SPREADY   COMY
#define SPREADW   (6 * TWSCALE)

#define RANGEX	  COMX
#define RANGEY	  (COMY +  1 * THSCALE + 3)
#define SHADEX    (COMX +  6 * TWSCALE - 4)
#define CYCLE1X   (COMX +  8 * TWSCALE - 2)
#define CYCLE2X   (COMX + 10 * TWSCALE + 0)
#define CYCLE3X   (COMX + 12 * TWSCALE + 2)
#define RANGEW    (2 * TWSCALE)

#define SPEEDX    (COMX + 10)
#define SPEEDY	  (COMY + 2 * THSCALE + 3)

#define TXRGB     (SXRGB + 3)
#define TXHSV     (SXHSV + 3)
#define TY        (RSCALE + 2)


#define MAXINT 0x7fff
#define SCLEXP 8
/* normalize factor = 256			*/

#define MAXRGB 15
#define MAXHSV 255  /* From "rgb2hsv()".*/
#define MAXH 63 /*WANTED: 89*/
#define	MAXSV 15
#define MAXSP 63

#define RGBBODY (MAXBODY / MAXRGB)
#define SVBODY (MAXBODY / MAXSV)
#define HBODY (MAXBODY / MAXH)
#define SPEED_BODY (MAXBODY / MAXSP)

#define UNDEFINED -1

typedef struct {
    SHORT r, g, b;
    } RGB;

typedef struct {
    SHORT h, s, v;
    } HSV;

/* ---------- Color Ranges -------------------------------------------------*/
typedef struct {
    SHORT count;
    SHORT rate;
    BOOL active;
    UBYTE low,high;	/* upper and lower bounds of range */
    } Range;

local Range originalRanges[LIM_RANGES] = { 0 };
    /*  Local copy of pGlobals->ranges[0]..[LIM_RANGES-1] */
local Range *ranges = NULL;	/* pGlobals->ranges.*/
local Range *range = NULL;	/* points to current Range w/i "ranges".*/

/* --------- Globals preserved across invocations, provided by client ----- */
typedef struct {
    WORD paletteX, paletteY;
	/* LeftEdge,TopEdge for palette window.
    	 * Client can change these to force window to appear elsewhere.
     	* If user drags window, these are updated before return to client.*/
    WORD paletteRange;		/* Index of range currently examined.*/
    Range *ranges;		/* LIM_RANGES Range's */
    } PaletteGlobals;

local PaletteGlobals *pGlobals = NULL;

#define PALETTE_X	pGlobals->paletteX
#define PALETTE_Y	pGlobals->paletteY
#define PALETTE_RANGE	pGlobals->paletteRange

#define PANEL_WIDTH  25	  /* Width of control strip on the right side.*/
#define TITLE_HEIGHT 11	  /* Height of title bar at top of screen.*/

/* ---------- ------------ -------------------------------------------------*/
/* there are LIM_SLIDERS porportional gadgets, one for the red, green, blue,
 * hue, saturation, value, and speed.
 *
 * Each points to the same image data for rendering the gadget
 */

local struct Window *_wp = NULL;
local struct Window *userwp = NULL;
local struct RastPort *_rp = NULL;
local struct RastPort *userrp = NULL;
local SHORT depth = 0;
local no_of_colors = 0;
local maxColor = 0;

local cx = 0, cy = 0;	/* Coordinates of current color square in palette.*/

local int isWide = FALSE;	/* TRUE when 640 H.*/
local int isTall = FALSE;	/* TRUE when 400 V.*/
local int px = PX;
local int pw = PW;
local int phs = PHs;

typedef struct {
    SHORT m[LIM_COLORS];
    } ColorMap;
local ColorMap *colorMapOriginal = NULL;
local ColorMap *colorMapUndo = NULL;
local ColorMap *colorMapNow = NULL;

/* Hack to keep colors 0 and 1 from being too close together.*/
/* These will quickly get set to the correct colors.*/
local RGB c0  = {0x20,0x20,0x20}, c1  = {0x2f,0x2f,0x2f};
local RGB undoC0  = {0x20,0x20,0x20}, undoC1  = {0x2f,0x2f,0x2f};
#define MIN_MENU_INTENSITY 8  /* At least one of (r,g,b) must differ by this.*/

local struct Gadget *colors = NULL;
local struct Image *colorImages = NULL;


#define TxWrite(rp, str)   Text(rp, str, strlen(str))

local struct TextAttr _TestFont =   {  "topaz.font",  TXHEIGHT, 0, 0, };

/* Specifies place and text.*/
typedef struct {
    WORD x, y;
    char *body;
    } PlacedText;

#define LIM_DECOR_TEXT  15
/* decorText: "decorative" text; that is, not part of any gadget; painted
 * directly on the palette window.*/
local PlacedText decorText[LIM_DECOR_TEXT] = {
    { 2 * TWSCALE,  2, 	"Color Palette" },
    { TXRGB,       TY, 	"R"             },
    { TXRGB + SDX, TY, 	"G"             },    
    { TXRGB + 2 * SDX, TY, 	"B",    },
    { TXHSV + 3 * SDX, TY, 	"H",    },
    { TXHSV + 4 * SDX, TY, 	"S",    },
    { TXHSV + 5 * SDX, TY, 	"V",    },
    { 1,     SY,           	"1",    },
    { 7,     SY,           	"5",    },
    { 5,     SY + SH - 9,  	"0",    },
    { SPEEDX, SPEEDY, 	"SPEED:",       },
    { 5,      SY + 45, 	"4",            },
    { 5,      SY + 29, 	"8",            },
    { 1,      SY + 12, 	"1",            },
    { 7,      SY + 12, 	"2",            } };

#define LIM_COMMANDS  11	/* # Command Gadgets.*/
#define COM_SHADE      3
#define COM_SPREAD     7	/* offset for Command Spread.*/
#define COM_EXCHANGE   8	/* must match mode's enumeration.*/
#define COM_COPY       9
#define COM_RANGE     10

/* Some buttons put palette into a mode.*/
#define MO_NORMAL	0
#define MO_SPREAD	1
#define MO_EXCHANGE	2
#define MO_COPY		3
#define MO_RANGE	4
local int mode = MO_NORMAL;

local char *gStrings[LIM_COMMANDS] = {
    "OK",  "CANCEL",  "UNDO",
    "SH",  "C1",      "C2",      "C3",
    "SPREAD",  "Ex",  "COPY",  "RANGE:"  };

local struct IntuiText InitGText = {
    0,1,JAM1, 2,1, &_TestFont, NULL,NULL };

/* gText stands for gadget text */
local struct IntuiText *gText = NULL;


#define STICKY_HEIGHT 18
#define STICKY_HOT_X  1
#define STICKY_HOT_Y  1

/*------------ icon bitmaps are now in palbms.c */
extern USHORT stickyPointer[],diamond[],diamond2[];
 
local struct Image InitSliderImage = {
	0,  0,        5,    5,      1,     &diamond[0], 0x1,  0 };
/* left edge,top edge, width,height, depth, predrawn image, ppick,pponoff */

local struct Image *sliderImages = NULL;

local WORD sliderPIBody[LIM_SLIDERS] = {
	RGBBODY, RGBBODY, RGBBODY, HBODY, SVBODY, SVBODY, SPEED_BODY };

local struct PropInfo *sliderPI = NULL;

/* Here is the gadget set for changing red, green, and blue components
 * (can also vary hue, saturation, and value) of a currently selected color.
 * Also speed. */
#define SPEED_PROPX  (SPEEDX + 6*TWSCALE - 10)
local int speed_tickx = SPEED_PROPX;

local struct Gadget *sliderGadgets = NULL;

/* Command Gadget Initialize */
#define CGadInit(x,y,w, flags, id) { NULL, x,y, w,OKH, \
    flags, NULL,NULL, NULL, 0, \
    NULL, id, NULL }
/* next gadget, left,top, width,height of hitbox, */
/* flags, BORDER,SELECT descriptor, text(later), mutual exclude, */
/* special info, gadget's identifier for client, client data */

local struct Gadget comGadgets[LIM_COMMANDS] = {
    CGadInit(OKX,      OKY,    2*TWSCALE, OKFLAGS, OK_GADGET),
    CGadInit(CANCELX,  OKY,    6*TWSCALE, OKFLAGS, CANCEL_GADGET),
    CGadInit(UNDOX,    OKY,    4*TWSCALE, DFFLAGS, UNDO_GADGET),
    CGadInit(SHADEX,   RANGEY, RANGEW,    DFFLAGS, RANGE_GADGETS+0),
    CGadInit(CYCLE1X,  RANGEY, RANGEW,    DFFLAGS, RANGE_GADGETS+1),
    CGadInit(CYCLE2X,  RANGEY, RANGEW,    DFFLAGS, RANGE_GADGETS+2),
    CGadInit(CYCLE3X,  RANGEY, RANGEW,    DFFLAGS, RANGE_GADGETS+3),
    CGadInit(SPREADX,  SPREADY,SPREADW,   STICKY_FLAGS, SPREAD_GADGET),
    CGadInit(EXCHANGEX,SPREADY,2*TWSCALE, STICKY_FLAGS, EXCHANGE_GADGET),
    CGadInit(COPYX,    SPREADY,4*TWSCALE, STICKY_FLAGS, COPY_GADGET),
    CGadInit(RANGEX,   RANGEY, 5*TWSCALE, STICKY_FLAGS, RANGE_GADGET) };

local struct NewWindow newcolorwindow = {
    0, 0,		/* start position	(later)	*/
    RW, RH, 		/* width, height 		*/
    0, 1,		/* detail pen, block pen	*/
    /* IDCMP Flags:-----*/
    IDCMPFLAGS,
    /* Window Flags:----*/
    BORDERLESS | ACTIVATE | WINDOWDRAG | RMBTRAP | NOCAREREFRESH,
    NULL,	/* pointer to first user gadget */
    NULL,		/* pointer to user checkmark    */ 
    "  Color Palette ",	/* window title 	*/ 
    NULL,		/* pointer to screen	(later)	*/
    NULL,		/* pointer to superbitmap 	*/
    0,0,0,0, 		/* ignored/not a sized window so
			 * dont have to specify min/max
			 * size to allow 		*/	
    CUSTOMSCREEN	/* type of screen in which to open */	
    };
	
local struct ViewPort *_vp = NULL;

local SHORT color_being_modified = 2;	/* Palette color #.*/
local SHORT currentColor = 0;	/* 4bits each r-g-b in ColorRegister format.*/
local RGB rgb = { 0 };	/* for currentColor */
local HSV hsv = { 0 };

local USHORT _class = 0;
local USHORT _code = 0;
local USHORT _qualifier = 0;
local USHORT _mode = 0;

local SHORT _gadgetid = 0;

local APTR _address = NULL;
local APTR oldaddress = NULL;

local testcode = 0;

local struct IntuiMessage *_message = NULL;

local void NoEvent() { _code = -1; _class = -1; _qualifier = -1; }

local int waiting_for_gadget_up() {

    _message = GetMsg(_wp->UserPort);
    if (_message == NULL)
	return(TRUE);      /* Yes, still waiting for a mouse-up event.*/

    _class = _message->Class;
    ReplyMsg(_message);
    return( _class != GADGETUP );    
    }

/* ---------- LookColor --------------------------------------------------- */
/* Set (cx,cy) to coordinates of color # id's square in the palette.*/
local void LookColor(id)   int id; {
    cx = px + pw * (id / phs);
    cy = PY + PH * (id % phs);
    }

/* ---------- Dash, HLine, Vline, DrawBox, DrawBoxAround, BoxText ---------*/
/* Some routines to help DrawWhich, and other drawers.*/

local int _y = 0;
local void Dash(x, w) int x, w; {	/* Draw a dash on the current line.*/
    Move(_rp, x, _y);
    Draw(_rp, x+w-1, _y);
    }

/* Draw a horizontal line.*/
#define HLine(x,y, w)  { _y = y;  Dash(x, w); }

local void VLine(x,y, h) int x,y, h; {	/* Draw a vertical line.*/
    Move(_rp, x, y);
    Draw(_rp, x, y+h-1);
    }

local void DrawBox(x,y, w,h) int x,y, w,h; {  /* Draw hollow box.*/
    register int x2 = x+w-1;
    register int y2 = y+h-1;

    Move(_rp, x,  y);
    Draw(_rp, x2, y);
    Draw(_rp, x2, y2);
    Draw(_rp, x,  y2);
    Draw(_rp, x,  y);
    }

local void DrawBoxAround(x,y, w,h) int x,y, w,h; {  /* Draw box around item.*/
    DrawBox(x-1,y-1, w+2,h+2);
    }

local void BoxText(x,y, w) int x,y, w; {  /* Draw box around text.*/
    DrawBoxAround(x,y, w,RSCALE-1);
    }

/* ---------- DrawRange --------------------------------------------------- */
/* Four routines for drawing range-marker on palette.*/

/* Bar to left of intermediate color in range.*/
local void DrawInRange(id)  int id;  { LookColor(id);  VLine(cx-1,cy,PH); }

/* Angle to upper-left of low color in range.*/
#define DrawLow(id)  { DrawInRange(id);  HLine(cx-1,cy-1,5); }

/* Angle to lower-left of high color in range.*/
#define DrawHigh(id)  { DrawInRange(id); HLine(cx-1,cy+PH,5); }

/* Draw current range. Caller must set pen color to 1 to draw or 0 to erase.*/
local void DrawRange() {
    register int i;

    if (range == NULL)   return;	/* No range yet.*/

    if (range->low > maxColor)
	range->low = maxColor;
    if (range->high > maxColor)
	range->high = maxColor;
    DrawLow(range->low);
    DrawHigh(range->high);
    for (i = range->low + 1;  i < range->high;  i++)
	DrawInRange(i);
    }

/* Draw a major mark around the Speed Proportional Gadget.*/
Stroke3(i)  int i; {
    VLine(speed_tickx+i,SPEEDY-1, 1);
    VLine(speed_tickx+i,SPEEDY+7, 3);
    }

/* ---------- DrawWhich --------------------------------------------------- */

#define DW_NONE    0
#define DW_TICKS   1
#define DW_HSV	   2
#define DW_RGB     3
#define DW_SPEED   4
#define DW_STICKY  5
local int drawWhich = DW_SPEED;
 /* Determines which set of gadgets (& their ticks) needs updating.*/

/* Accumulate needed gadget drawing.  The gadgets are in a list,
 * when RefreshGadgets is called, gadgets from the selected one through
 * the end of the list get redrawn.*/
NewDrawWhich(newDrawWhich)  int newDrawWhich; {
    if (newDrawWhich > drawWhich)
	drawWhich = newDrawWhich;
    }

local BOOL reentry = FALSE;	/* So know when re-entering the window.*/

/* Draw all the marks on the window that get damaged by RefreshGadgets(),
 * or by ResetColor().*/
local void DrawWhich()
    {
    int i, iPower, tickx, tickw;
    
    drawWhich = DW_NONE;

    /* ----- draw HSV ticks ----- */

    SetAPen(_rp, 0);
    /* Dashes at sixths of HSV.*/
    _y = SY + SH - 6;
    for (i = 0;  i < 7;  i++) {
	Dash(SXHSV + 4 * sdx - 2, 5+1);
	Dash(SXHSV + 5 * sdx - 2, 5+1);
	if (i == 0  ||  i == 3  ||  i == 6) {  /* Extra dashes */
	    Dash(SXHSV + 3 * sdx - 1 + isWide, 3+1);
	    Dash(SXHSV + 6 * sdx - 3,          3+1);
	    }
	_y -= 10;
	}

    if (drawWhich == DW_HSV)
	return;

    /* ----- draw RGB ticks ----- */

    /* Narrow sliders. */
    if (!isWide) {
	SetAPen(_rp, 1);
	VLine(SXRGB + 0 * SDX + 2, SY + 1,  SH - 1);
	VLine(SXRGB + 1 * SDX + 2, SY + 1,  SH - 1);
	VLine(SXRGB + 2 * SDX + 2, SY + 1,  SH - 1);
	}

    SetAPen(_rp, 0);
    for (i = 0;  i < 16;  i++) {
	iPower = (i % 4 ? (i % 2 ? 0 : 1) : 2);  /* 1<<iPower == i */
	_y = SY + 4 + ((15 - i) * (SH - 10)) / 15;	/* 0..15 marks for RGB.*/
	tickx = SXRGB - iPower;
	tickw = iPower * 2 + 1;
	if (isWide)
	    tickw += 1;
	Dash(tickx,  tickw);		/* by RGB sliders */
	Dash(tickx += sdx, tickw);
	Dash(tickx += sdx, tickw);
	}

    if (drawWhich == DW_RGB)
	return;

    /* ----- Draw marks around Speed Proportional Gadget ----- */
    SetAPen(_rp, 0);
    Stroke3(0);
    for (i = 1;  i <= MAXSP;  i += 2) {	 /* Marks at 1,3,5,7...*/
	switch ((i+1) % 8) {
	    case 0:
		Stroke3(i);   break;
	    case 4:
		VLine(speed_tickx+i,SPEEDY+7, 2);   break;
	    default:
		VLine(speed_tickx+i,SPEEDY+7, 1);
	    }
	}

    }


local void ForceDrawWhich() {
    register struct Gadget *gadgetToDraw;

    switch (drawWhich) {
	case DW_RGB:  gadgetToDraw = &sliderGadgets[0];  goto YesRefresh;
	case DW_HSV:  gadgetToDraw = &sliderGadgets[3];  goto YesRefresh;
	case DW_SPEED: gadgetToDraw = &sliderGadgets[6];  goto YesRefresh;
	case DW_STICKY: gadgetToDraw = &comGadgets[COM_SPREAD];
YesRefresh:
	    RefreshGadgets(gadgetToDraw, _wp, NULL);
	case DW_TICKS:
	    DrawWhich();
	    /* FALL THRU */
	case DW_NONE:
	}
    }

InCaseOfReentry() {
    if (reentry) {
	reentry = FALSE;
	NewDrawWhich(DW_TICKS);
	ForceDrawWhich();
	}
    }

/* ---------- SetCurrentColor ----------------------------------------------*/

/* convert a 12bit rgb value to separate (r,g,b) values.*/
SplitRGB4(colorRegister, rgb)  WORD colorRegister;  RGB *rgb; {
    rgb->r = (colorRegister >> 8) & 0xf;
    rgb->g = (colorRegister >> 4) & 0xf;
    rgb->b = colorRegister        & 0xf;
    }

/* Check delta of each color component to see if that contrast allows readable
 * menus */
local int minReadable = 0;
local int Readable(delta)   int delta; {
    return ( ABS(delta) >= minReadable );
    }

/* Set CurrentColor on the screen.*/
local void _SetCC() {
    RGB other;

    /* Hack to keep colors 0 and 1 from being too close together.*/
    switch (color_being_modified) {
        case 0:	 other = c1;	goto menuColorHack;
        case 1:	 other = c0;
menuColorHack:
	    minReadable = MIN_MENU_INTENSITY;
	    if ( !Readable(other.r - rgb.r)  &&  !Readable(other.g - rgb.g) &&
	         !Readable(other.b - rgb.b) ) {
	        /* Don't make the color change; restore data to what it was.*/
	        currentColor = colorMapNow->m[color_being_modified];
	        SplitRGB4(currentColor, &rgb);
	        rgb2hsv(&rgb, &hsv);
		NewDrawWhich(DW_RGB);	/* All 6 sliders.*/
		return;
	        }
        }

    /* Remember the accepted color.*/
    switch (color_being_modified) {
        case 0:	 c0  = rgb;	break;
        case 1:	 c1  = rgb;	break;
	}

    /* Affect the screen.*/
    colorMapNow->m[color_being_modified] = currentColor;
    SetRGB4(_vp, color_being_modified, rgb.r, rgb.g, rgb.b);
    }

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Set current_color_no to 12b r-g-b "value".*/
/* Sets HSV to match.*/
SetCurrentColor(value)  int value; {
    currentColor = value;
    SplitRGB4(currentColor, &rgb);
    rgb2hsv(&rgb, &hsv);
    NewDrawWhich(DW_RGB /*DW_HSV*/);  /* Also force RGB to line up with ticks.*/
    if (hsv.h == UNDEFINED)   hsv.h = 0;  /* [TBD] move this to rgb2hsv? */
    _SetCC();
    }

RGBSetsCurrentColor() {
    SetCurrentColor( (rgb.r << 8) | (rgb.g << 4) | rgb.b );
    }

/* Preserves HSV.*/
HSVSetsCurrentColor() {
    hsv2rgb(&hsv, &rgb);
    NewDrawWhich(DW_RGB);
    currentColor = (rgb.r << 8) | (rgb.g << 4) | rgb.b;
    _SetCC();
    }


#define ToPot15(n)     ( 0xffff - 0x1111 * ((n) & 0xf) )
#define ToPot255(n)    ( 0xffff - 0x0101 * (n) )
#define FromVertPotXXXX(n) ( 0xffff - (n) )
#define FromHorizPotXXXX(n) (n)

local void ModifyRGBHSVSliders() {
    /* convert 4 bits into 16 bits by replicating it in 4 positions.*/
    sliderPI[MODIFYRED].VertPot = ToPot15(rgb.r);
    sliderPI[MODIFYGREEN].VertPot = ToPot15(rgb.g);
    sliderPI[MODIFYBLUE].VertPot = ToPot15(rgb.b);
    /* convert 8 bits to 16 bits by replicating it.*/
    sliderPI[MODIFYHUE].VertPot = ToPot255(hsv.h);
    sliderPI[MODIFYSATURATION].VertPot = ToPot255(hsv.s);
    sliderPI[MODIFYVALUE].VertPot = ToPot255(hsv.v);
    if (range != NULL)
	sliderPI[MODIFYSPEED].HorizPot = range->rate * SPEED_BODY;
    }

/*---------- WInvertGadget ------------------------------------------------*/
#define XORMASK 0x6a
#define NOOP 0xaa

local void SetXorFgPen(rp) struct RastPort *rp; {
    SHORT pen = rp->FgPen;
    SHORT bit = 1;
    SHORT i;
    for (i=0; i<rp->BitMap->Depth; i++) {
    	rp->minterms[i] = (bit&pen) ? XORMASK : NOOP;
	bit <<=1;
	}
    }

/* Does highlighting of gadgets using only colors 0 and 1.
 * Caution: Leaves draw mode and pen modified.
 * After, do "SetDrMd(_rp,JAM1 or JAM2)" and "SetAPen(_rp,color)".*/
local void WInvertGadget(gad)  struct Gadget *gad; {
    int x,y,temp;
    struct IntuiText *text = gad->GadgetText;

    x = gad->LeftEdge,
    y = gad->TopEdge;
    SetAPen(_rp,1);
    SetXorFgPen(_rp);
    RectFill(_rp, x, y, x + gad->Width-1, y + gad->Height-1);
    /* Tell system to use inverted rendering for this gadget in case
     * window reentered.*/
    if (text) {
	temp = text->FrontPen;
	text->FrontPen = text->BackPen;
	text->BackPen = temp;
	}
    }

/* ---------- ResetRange ---------------------------------------------------*/
/* Caller must do "ModifyRGBHSVSliders" to have new range's speed show up.*/
local void ResetRange(index)   int index; {

    /* Force "index" to be valid.*/
    if (index < 0)
	index = 0;
    if (index >= LIM_RANGES)
	index = LIM_RANGES-1;

    if (range)	/* turn off previous highlight.*/
	WInvertGadget(&comGadgets[COM_SHADE+PALETTE_RANGE]);
    /* Highlight selected range.*/
    WInvertGadget(&comGadgets[COM_SHADE+index]);
    SetDrMd(_rp, JAM1);		/* WInvertGadget altered draw mode.*/

    SetAPen(_rp, 0);  DrawRange();	/* Erase old range.*/
    range = &ranges[PALETTE_RANGE = index];
    SetAPen(_rp, 1);  DrawRange();  /* Draw current range.*/
    DrawColorsBorder(color_being_modified);
	/* May have been smashed when range erased.*/
    }

/* ---------- ResetColor ---------------------------------------------------*/
/* Make the sliders jump out to the RGB values for the color that was
 * selected.
 * When mode==MO_COPY, it first copies the previous color's RGB values to
 * the new color.
 * When mode==MO_SPREAD, it first alters the colors inbetween to be
 * an HSV spread between the previous color and the newly selected color.
 */

/* Draw white border around newly selected palette square.*/
local DrawColorsBorder(id)   int id; {
    SetAPen(_rp, 1);
    LookColor(id);
    DrawBox(cx, cy,  pw - 1, PH);
    }

/* New state for "mode".*/
local void NewMode(newMode)  int newMode; {
    int sticky = newMode - 1 + COM_SPREAD;
    int oldSticky = mode - 1 + COM_SPREAD;
    int oldMode;

    oldMode = mode;
    mode = newMode;

    if (newMode == MO_NORMAL)
	ClearPointer(_wp);
    else
	SetPointer(_wp, &stickyPointer, STICKY_HEIGHT, 16,
		   -STICKY_HOT_X, -STICKY_HOT_Y);

    if (oldMode != MO_NORMAL)
	WInvertGadget(&comGadgets[oldSticky]);
    if (newMode != MO_NORMAL)
	WInvertGadget(&comGadgets[sticky]);
    SetDrMd(_rp, JAM1);		/* WInvertGadget alters draw mode.*/
    }

local void ResetColor(id)  SHORT id; {
    register int i, rounder, limitI, dI, newColor, oldColor;
    int rounder1, rounder2, rounder3;
    HSV firstHsv, dHsv;	  /* first color, total delta for HSV color-spread.*/
    RGB tempRgb;	  /* temp color for swapping with undo.*/

    *colorMapUndo = *colorMapNow;   /* So can undo any changes that follow.*/
    undoC0  = c0;    undoC1  = c1;

    /* Erase border around previous palette square by drawing in black.*/
    SetAPen(_rp, 0);
    LookColor(color_being_modified);
    DrawBox(cx, cy,  pw - 1, PH);

    oldColor = currentColor;
    currentColor = newColor = colorMapNow->m[id]; /*GetRGB4(_vp->ColorMap,id);*/

    switch(mode) {
	case MO_NORMAL: {
	    /*ALREADY DID ABOVE: currentColor = newColor; */
	    break;  }
	case MO_COPY: {
	    currentColor = oldColor;	/* Uses color from PREVIOUS  */
	    break;  }			/* color_being_modified.     */
	case MO_EXCHANGE: {
	    /* Hack to counter c0/c1 hack in _SetCC */
	    /* This avoids a moment when both c0 & c1 attempt to be newColor.*/
	    if ( (color_being_modified == 0  &&  id == 1)  ||
		 (color_being_modified == 1  &&  id == 0) ) {
		tempRgb = c0;   c0 = c1;   c1 = tempRgb;  /* Exchange c0,c1 */
		}

	    SetCurrentColor(newColor);	/* Set previous id's color to
					 * new id's color.*/
	    currentColor = oldColor;	/* Uses color from PREVIOUS  */
	    break;  }			/* color_being_modified.     */
	case MO_SPREAD: {
	    limitI = id - color_being_modified;
	    rounder = limitI / 2;	/* For rounding below.*/
	    dI = (limitI >= 0 ? 1 : -1);
	    SplitRGB4(newColor, &rgb);
	    /* dHSV = new color's HSV */
		rgb2hsv(&rgb, &dHsv);
		if (dHsv.s == 0)   	/* Set new H to match previous,   */
		    dHsv.h = hsv.h;	/* since H was really "UNDEFINED".*/
		if (hsv.s == 0)		/* Set previous H to match new.*/
		    hsv.h = dHsv.h;
		if (dHsv.v == 0)	/* Set new S to match previous,   */
		    dHsv.s = hsv.s;	/* since S was really "UNDEFINED".*/
		if (hsv.v == 0)		/* Set previous S to match new.*/
		    hsv.s = dHsv.s;
	        firstHsv = hsv;	        /* Previous "color_being_modified".*/

#define FAVOR_ARC (256/12)	/* Distance between Yellow and Orange.*/
#define MAIN_ARC  (128+FAVOR_ARC)

	    /* dHSV = newHSV - previousHSV */
		dHsv.h -= hsv.h;
		if (dHsv.h > MAIN_ARC)	/* Choose the smaller arc around hue.*/
		    dHsv.h -= 256;	/* Values related to MAXHSV.*/
		if (dHsv.h < -MAIN_ARC)	/* Favor Y..G..B over Y..R..B arc.*/
		    dHsv.h += 256;
		dHsv.s -= hsv.s;   dHsv.v -= hsv.v;
	    /* round with same sign as each component.*/
	    rounder1 = (dHsv.h > 0 ? rounder : -rounder);
	    rounder2 = (dHsv.s > 0 ? rounder : -rounder);
	    rounder3 = (dHsv.v > 0 ? rounder : -rounder);

	    /* Starts at one-end, so can detect case where ends are same id.*/
	    for (i = 0;  i != limitI;  i += dI) {
		/* Each color is fractionally farther from firstHsv.*/
		    hsv.h = MAXHSV &
			( firstHsv.h + (dHsv.h * i + rounder1) / limitI );
		    hsv.s = firstHsv.s + (dHsv.s * i + rounder2) / limitI;
		    hsv.v = firstHsv.v + (dHsv.v * i + rounder3) / limitI;
		HSVSetsCurrentColor();
		color_being_modified += dI;
		}
	    currentColor = newColor;
	    break;  }
	case MO_RANGE: {
	    if (range == NULL)
		break;
	    SetAPen(_rp, 0);  DrawRange();	/* Erase old range.*/

	    if (color_being_modified <= id) {
	    	range->low = color_being_modified;
	    	range->high = id;
		}
	    else {
	    	range->low = id;
	    	range->high = color_being_modified;
		}
	    break;  }
	}
    NewMode(MO_NORMAL);	/* Ends MO_COPY, etc. */

    /* Draw white border around newly selected palette square.*/
    /* Done after any range-change, since range-erase may hit border.*/
    DrawColorsBorder(id);

    color_being_modified = id;  /* An implicit parameter to many routines!*/
    SetCurrentColor(currentColor);
    NewDrawWhich(DW_RGB);		/* So get all 6 sliders.*/

    SetAPen(_rp, 1);  DrawRange();  /* Draw current range.*/

    /* Draw enlarged current-color rectangle above the palette array.*/
    SetAPen(_rp, color_being_modified);
    RectFill(_rp, px, TY+1,  px+2*pw -1, TY+1 + PH-1 -1);

    ModifyRGBHSVSliders();
    }

/* ---------- getvalue -----------------------------------------------------*/
/* "oldaddress" points to the proportional gadget which is down.
 * RETURN the pot value in a range dependent on "sig".
 * sig is # significant bits;  4 yields 0..15, 8 yields 0..255.
 * 16 yields 0..0xFFFF, which is MAXBODY.
 * Pass in negated sig to get "HorizPot".
 * [TBD] Should ROUND rather than TRUNCATE?  Or is that problem internal
 * to PropGadget ROM code? Or maybe ROM code is including inaccessible
 * ends of slider region in the computation?
 */
local int getvalue(sig)  SHORT sig; {
    register struct PropInfo *p;
    register int colorpar;

    p = (struct PropInfo *)( ((struct Gadget *)oldaddress)->SpecialInfo);
    colorpar = FromVertPotXXXX(p->VertPot);
    if (sig < 0) {
	sig = -sig;
	colorpar = FromHorizPotXXXX(p->HorizPot);
	}
    colorpar = colorpar >> (16 - sig);
    
    if ((sig == 8) && (colorpar < 2))	/* VertPot doesn't return 0 for HSV. */
	    colorpar = 0;  /* [TBD] is this a ROM bug to report? Still true?*/
    return(colorpar);
    }

/* ---------- syscolorfix -------------------------------------------------*/
/* Handles sliders R,G,B, H,S,V, SPEED */
local void syscolorfix(whichslider)  SHORT whichslider; {
    do  {
	switch (whichslider) {
	    case MODIFYRED:
		rgb.r = getvalue(4);
		goto RGBSets;
	    case MODIFYGREEN:
		rgb.g = getvalue(4);
		goto RGBSets;
	    case MODIFYBLUE:
		rgb.b = getvalue(4);
RGBSets:	RGBSetsCurrentColor();
		break;
	    case MODIFYHUE:
		hsv.h = getvalue(8);
		goto HSVSets;
	    case MODIFYSATURATION:
		hsv.s = getvalue(8);
		goto HSVSets;
	    case MODIFYVALUE:
		hsv.v = getvalue(8);
HSVSets:	HSVSetsCurrentColor();
		break;
	    case MODIFYSPEED:
		if (range != NULL)
		    range->rate = getvalue(-6);
		break;
	    }
	} while (waiting_for_gadget_up());

    if (drawWhich != DW_NONE)
	ModifyRGBHSVSliders();
    }

/* ---------- _Undo --------------------------------------------------------*/
/* UNDO command-button activated, or "u" key pressed.*/
local void _Undo() {
    int i, colorid;

    NewMode(MO_NORMAL);	/* Ends MO_COPY, etc. */
    /* Restore colors to just before last palette selection.
     * This undoes any slider action on this color; it also undoes
     * any MIX,GLOW,Exchange,COPY */
    colorid = color_being_modified;  /* hold */
    c0  = undoC0;    c1 = undoC1;
    for (i = 0; i < no_of_colors; i++) {
	SetCurrentColor(colorMapUndo->m[color_being_modified = i]);
	}
    SetCurrentColor(colorMapNow->m[color_being_modified = colorid]);
    ModifyRGBHSVSliders();
    }

/* ---------- EventHandler ------------------------------------------------*/
local EventHandler() {
    int i, colorid, newMode;
    
    WaitTOF();   /* yield control to other processes */
    
    _message = GetMsg (userwp->UserPort);
    if (_message) {
	ReplyMsg(_message);

	if ((_message->Class == MOUSEBUTTONS) & (_message->Code == SELECTDOWN))
	    {
	    colorid = ReadPixel (userrp, _message->MouseX, _message->MouseY);
	    ResetColor(colorid);
	    reentry = TRUE;	/* Will be re-entering the window.*/
	    }
	}

    _message = GetMsg(_wp->UserPort);
    if (_message == NULL) {
	ForceDrawWhich();
	return(PALETTE_X == _wp->LeftEdge  &&  PALETTE_Y == _wp->TopEdge);
	    /* FALSE if window moved. EXIT.*/
	}

    _class = _message->Class;
    _code = _message->Code;
    _qualifier = _message->Qualifier;
    _address = _message->IAddress;
    oldaddress = _address;
    ReplyMsg(_message);
    testcode = TRUE;	/* closewindow test for neil ~~~~~~*/

    InCaseOfReentry();
    if (_class != GADGETDOWN  &&  _class != GADGETUP) {
	if (_class == MOUSEBUTTONS  &&  _code == SELECTDOWN)
	    InCaseOfReentry();
	else if (_class == RAWKEY  &&  _code == (UCODE|DOWNFLAG))
	    _Undo();
	return(testcode);
	}
    _gadgetid = ((struct Gadget *)_address)->GadgetID;

    if (inRange(_gadgetid, SLIDERGADGETS, LIM_SLIDERS)) {
	syscolorfix(_gadgetid - (SLIDERGADGETS));
	}

    else if (inRange(_gadgetid, COLORGADGETS, LIM_COLORS)) {
	ResetColor(_gadgetid - (COLORGADGETS));
	}
    else if (inRange(_gadgetid, RANGE_GADGETS, LIM_RANGES)) {
	ResetRange(_gadgetid - RANGE_GADGETS);
	ModifyRGBHSVSliders();
	NewDrawWhich(DW_SPEED);	/* So get all 7 sliders drawn.*/
	}
    else switch (_gadgetid) {
	case UNDO_GADGET:
	    _Undo();
	    break;
	case CANCEL_GADGET:
	    /* Restore original ranges.*/
	    for (i = 0;  i < LIM_RANGES;  i++)
		ranges[i] = originalRanges[i];
	    /* Restore original colors.*/
	    colorid = color_being_modified;  /* hold */
	    for (i = 0; i < no_of_colors; i++)
		SetCurrentColor(colorMapOriginal->m[color_being_modified = i]);
	    SetCurrentColor(colorMapNow->m[color_being_modified = colorid]);
	    /* FALL THRU to OK_GADGET */
	case OK_GADGET:
	    testcode = FALSE;  break;  /* Exit PaletteTool */
	case SPREAD_GADGET:
	    newMode = MO_SPREAD; /* Previous and next colors chosen span HSV */
	    goto NewSticky;   /* range within which colors are blended */
	case EXCHANGE_GADGET:
	    newMode = MO_EXCHANGE;  goto NewSticky;
	case COPY_GADGET:
	    newMode = MO_COPY;  goto NewSticky;
	case RANGE_GADGET:
	    newMode = MO_RANGE;    /* Previous and next colors chosen become */
	                        /* the new span for currently selected range.*/

NewSticky:  /* These are infix operators, indicated by making
	     * corresponding boolean be sticky.
	     * Next color chosen is destination, or end of range.*/
	    NewMode(newMode);
	    break;
	}

    NoEvent();
    return(testcode);
    }

/* ---------- InitPaletteGadgets -------------------------------------------*/
local void InitPaletteGadgets(depth)  SHORT depth; {
    SHORT n;

    for(n=0; n < no_of_colors; n++) {
	colors[n].LeftEdge = px + (pw) * (n / phs);
	colors[n].TopEdge = PY + (PH) * (n % phs);		
	colors[n].Width = pw-1;
	colors[n].Height = PH;
	colors[n].Flags = GADGHNONE | GADGIMAGE;
	colors[n].Activation = GADGIMMEDIATE;
	colors[n].GadgetType = BOOLGADGET;
	colors[n].GadgetRender = (APTR)&colorImages[n];
	colors[n].GadgetID = COLORGADGETS + n;
/*ALLOC filled with zeroes already.
	colors[n].NextGadget = NULL;
	colors[n].SelectRender = NULL;
	colors[n].GadgetText = NULL;	
	colors[n].MutualExclude = 0;
	colors[n].SpecialInfo = NULL;
	colors[n].UserData = NULL;
 */
					/* Make hit box smaller for outside. */
	colorImages[n].LeftEdge = 1;
	colorImages[n].TopEdge = 1;
	colorImages[n].Width = pw-3;
	colorImages[n].Height = PH-2;
	colorImages[n].PlaneOnOff = n;
/*ALLOC filled with zeroes already.
	colorImages[n].Depth = 0;
	colorImages[n].ImageData = NULL;
	colorImages[n].PlanePick = 0;
 */

	AddGadget (_wp, &colors[n], -1);
	}
    } 

/* ---------- InitSliderGadgets --------------------------------------------*/
local void InitSliderGadgets() {
    register int i, x;

    InitSliderImage.ImageData = (isWide ? &diamond2[0] : &diamond[0]);
    InitSliderImage.Width     = (isWide ? 9            : 5);

    x = SXRGB;
    for (i = 0;  i < LIM_SLIDERS;  i++) {

	sliderImages[i] = InitSliderImage;

	sliderPI[i].Flags = FREEVERT;
	sliderPI[i].VertBody = sliderPIBody[i];

	if (i == MODIFYHUE)
	    x = SXHSV;
	sliderGadgets[i].LeftEdge = x + i * sdx + 2*isWide;
	sliderGadgets[i].TopEdge  = SY;
	sliderGadgets[i].Width    = sw;
	sliderGadgets[i].Height   = SH;
	sliderGadgets[i].Flags    = GADGIMAGE | GADGHNONE;
	sliderGadgets[i].Activation = RELVERIFY | GADGIMMEDIATE;
	sliderGadgets[i].GadgetType = PROPGADGET;
	sliderGadgets[i].GadgetRender = (APTR)&sliderImages[i];
	sliderGadgets[i].SpecialInfo = (APTR)&sliderPI[i];
	sliderGadgets[i].GadgetID = SLIDERGADGETS+i;
	if (i == MODIFYSPEED) {
	    sliderPI[i].Flags = FREEHORIZ;
	    sliderPI[i].HorizBody = sliderPI[i].VertBody;

	    sliderGadgets[i].LeftEdge = SPEED_PROPX - 2;
	    sliderGadgets[i].TopEdge  = SPEEDY - 1;
	    sliderGadgets[i].Width    = (isWide ? MAXSP+17 : MAXSP+13);
	    sliderGadgets[i].Height   = 9;
	    }
	}

/* Speed gadget first, so can re-draw rgbhsv without redrawing speed.*/
    AddGadget(_wp, &sliderGadgets[MODIFYSPEED], -1);
    for (i = 0;  i < MODIFYSPEED;  i++)
	AddGadget(_wp, &sliderGadgets[i], -1);
    }

/* ---------- PaletteTool --------------------------------------------------*/

FreeAllRAM() {
    FREE( colors,        LIM_COLORS * sizeof(struct Gadget) );
    FREE( colorImages,   LIM_COLORS * sizeof(struct Image)  );
    FREE( colorMapOriginal, sizeof(ColorMap) );
    FREE( colorMapUndo,     sizeof(ColorMap) );
    FREE( colorMapNow,      sizeof(ColorMap) );
    FREE( sliderGadgets, LIM_SLIDERS * sizeof(struct Gadget)   );
    FREE( sliderImages,  LIM_SLIDERS * sizeof(struct Image)    );
    FREE( sliderPI,      LIM_SLIDERS * sizeof(struct PropInfo) );
    FREE( gText,       LIM_COMMANDS * sizeof(struct IntuiText) );
    }

/* windowptr is client's window, in which to bring up palette window.
 * paletteGlobals allows Palette.c to be an overlay, yet retain information
 * between calls.
 * Returns 0 if window moved.  Client should call again, will reopen in
 * new position.  Returns -1 if not enough RAM.
 */
int PaletteTool(windowptr, paletteGlobals)
    struct Window *windowptr;   PaletteGlobals *paletteGlobals;  {
    int i, minY, rw, returnCode, enoughRAM;

    pGlobals = paletteGlobals;  /* So can access globals from subroutines.*/

    _wp = windowptr;
    _rp = _wp->RPort;
    _vp = &_wp->WScreen->ViewPort;
    depth = _rp->BitMap->Depth;
    if (depth > DEPTH)
	{
/*	KPrintF ("\nDEPTH of Color Palette > %d\n", DEPTH); */
	exit (0);
	}

    colors      = (struct Gadget *)ALLOC(LIM_COLORS * sizeof(struct Gadget));
    colorImages  = (struct Image *)ALLOC(LIM_COLORS * sizeof(struct Image));
    colorMapOriginal = (ColorMap *)ALLOC(sizeof(ColorMap));
    colorMapUndo     = (ColorMap *)ALLOC(sizeof(ColorMap));
    colorMapNow      = (ColorMap *)ALLOC(sizeof(ColorMap));
    sliderGadgets =(struct Gadget *)ALLOC(LIM_SLIDERS * sizeof(struct Gadget));
    sliderImages  = (struct Image *)ALLOC(LIM_SLIDERS * sizeof(struct Image));
    sliderPI = (struct PropInfo *)ALLOC(LIM_SLIDERS * sizeof(struct PropInfo));
    gText = (struct IntuiText *)ALLOC(LIM_COMMANDS * sizeof(struct IntuiText));
    enoughRAM = (colors && colorImages && colorMapOriginal && colorMapUndo &&
	colorMapNow && sliderGadgets && sliderImages && sliderPI && gText);

    if (!enoughRAM) {
	FreeAllRAM();
	return(-1);
	}

    mode = MO_NORMAL;

    no_of_colors = 1 << depth;
    maxColor = no_of_colors - 1;
    if (color_being_modified > maxColor)
	color_being_modified = maxColor;

    for (i = 0; i < no_of_colors; i++)
	colorMapOriginal->m[i] = GetRGB4(_vp->ColorMap, i);
    *colorMapUndo = *colorMapNow = *colorMapOriginal;

    isWide = (_wp->Width > 320);

    px = (isWide ? PX+38 : PX);
    pw = (isWide ? 2*PW  : PW);

    /* With 16 or 32 colors, 8 color-squares per column.
     * With 8 colors, 4 per column.  With 2 or 4 colors, 2 per column.*/
    phs = ( depth > 3 ? 8 :  (depth == 3 ? 4 : 2) );

    speed_tickx = (isWide ? SPEED_PROPX+4+2 : SPEED_PROPX+4);

    /* Double palette window width when 640 H.*/
    newcolorwindow.Width = rw = (isWide ? px + 2*pw + 4 : RW);

    if ((_wp->Width < rw) | (_wp->Height < RH))
	{
/*	KPrintF ("\nWindow Too Small For NewWindow\n"); */
	exit (0);
	}

    sdx = (isWide ? SDX+6 : SDX);
    sw  = (isWide ? SDX+4 : SDX);

    /* Double PANEL_WIDTH when 640 H.*/
/*  maxX = _wp->Width - rw -
        (isWide ? 2*PANEL_WIDTH : PANEL_WIDTH);*/
    if ((PALETTE_X + rw) > _wp->Width)
	PALETTE_X = _wp->Width - rw - 1;

    /* Double TITLE_HEIGHT in INTERLACE mode.*/
    minY = (_wp->Height > 200 ? 2*TITLE_HEIGHT : TITLE_HEIGHT);
    if (PALETTE_Y < minY)
	PALETTE_Y = minY;
    if ((PALETTE_Y + RH) > _wp->Height)
	PALETTE_Y = _wp->Height - RH - 1;

    newcolorwindow.LeftEdge = PALETTE_X;
    newcolorwindow.TopEdge = PALETTE_Y;
    newcolorwindow.Screen = _wp->WScreen;

    userwp = _wp;
    userrp = _rp;
    _wp = OpenWindow(&newcolorwindow);		/* open a window */
    _rp = _wp->RPort;
    _vp = &_wp->WScreen->ViewPort;

    SetRast(_rp, 1);		/* White window background.*/

    SetAPen(_rp, 0);
    RectFill(_rp, px-2,PY-1,
	          px+((maxColor+phs)/phs)*pw,
		  PY+MIN(phs,no_of_colors)*PH); /* Black behind palette.*/
    BoxText(COPYX,   SPREADY, 4*TWSCALE);
    BoxText(EXCHANGEX,   SPREADY, 2*TWSCALE);
    BoxText(SPREADX, SPREADY, SPREADW);		/* Black border around text.*/
    BoxText(RANGEX,  RANGEY,  5*TWSCALE-2);
	BoxText(SHADEX,  RANGEY, RANGEW);
	BoxText(CYCLE1X, RANGEY, RANGEW);
	BoxText(CYCLE2X, RANGEY, RANGEW);
	BoxText(CYCLE3X, RANGEY, RANGEW);
    BoxText(CANCELX, OKY,     6*TWSCALE);
    BoxText(UNDOX,   OKY,     4*TWSCALE);
    BoxText(OKX,     OKY,     2*TWSCALE);
    DrawBoxAround(OKX-1,   OKY-1,   2*TWSCALE+2,RSCALE+1);	/* Double-thick box.*/

    /* Border around enlarged current-color rectangle above palette array.*/
    SetAPen(_rp, 0);	/* Black border.*/
    DrawBoxAround(px, TY+1,  2*pw, PH-1);

    InitPaletteGadgets();

    for (i = 0;  i < LIM_COMMANDS;  i++) {
	gText[i] = InitGText;
	gText[i].IText = gStrings[i];
	comGadgets[i].GadgetText = &gText[i];
	AddGadget(_wp, &comGadgets[i], -1);
	}

    RefreshGadgets(&colors[0], _wp, NULL);

/* After RefreshGadgets, so not displayed until have correct values, later.*/
    InitSliderGadgets();

    /* After InitSliderGadgets, so can pick up x for "R","G","B","H","S","V".*/
    for (i = 0;  i < 6;  i++)
	decorText[i+1].x = sliderGadgets[i].LeftEdge + 2 + 2 * isWide;
    SetAPen(_rp, 0);
    SetDrMd(_rp, JAM1);
    for (i = 0;  i < LIM_DECOR_TEXT;  i++) {
	Move(_rp, decorText[i].x, decorText[i].y+6);
	TxWrite(_rp, decorText[i].body);
	}

    ranges = pGlobals->ranges;
    /* Make local copy, so can CANCEL without having lost client's values.*/
    for (i = 0;  i < LIM_RANGES;  i++)
	originalRanges[i] = ranges[i];

    range = NULL;	/* No previous range to erase.*/
    ResetRange(PALETTE_RANGE);

    SetAPen(_rp, 0);
    DrawBox(0,0, rw,RH);	/* black border around window.*/
    DrawBox(0,0, rw,RSCALE+1);	/* line under title.*/

    oldaddress = (APTR) &sliderPI[0];

    /* Hack to keep colors 0 and 1 from being too close together.
     * This code forces colors 0 & 1 to be examined;
     * so c0, c1 get set to correct values.
     */
    i = color_being_modified;	/* hold */
    SetCurrentColor(colorMapNow->m[color_being_modified = 0]);
    SetCurrentColor(colorMapNow->m[color_being_modified = 1]);

    ResetColor(i);
    NewDrawWhich(DW_SPEED);	/* So get all 7 sliders drawn.*/
    while (EventHandler());


    returnCode = (PALETTE_X == _wp->LeftEdge  &&  PALETTE_Y == _wp->TopEdge);
	/* 0 if window moved.*/

    /* Remember where window ended up.*/
    PALETTE_X = _wp->LeftEdge;
    PALETTE_Y = _wp->TopEdge;

    CloseWindow(_wp);

    FreeAllRAM();

    return (returnCode);	/* 0 if window moved.*/
    }

