/*----------------------------------------------------------------------*/
/*									*/
/*		keyjunk.c							*/
/*									*/
/*----------------------------------------------------------------------*/

#include "system.h"

#define local static

SHORT ShiftKey = 0;
SHORT AltKey = 0;
SHORT Lshift = 0, Rshift = 0;
SHORT Ctrl = 0;
SHORT Caps = 0;
SHORT LeftAlt = 0;
SHORT RightAlt = 0;
SHORT LeftCom = 0;
SHORT RightCom = 0;
    
local char unshChars[] = {
/*0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f */
0x60,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x30,0x2d,0x3d,0x5c,0x08,0x00,
0x71,0x77,0x65,0x72,0x74,0x79,0x75,0x69,0x6f,0x70,0x5b,0x5d,0x0d,0x31,0x32,0x33,
0x61,0x73,0x64,0x66,0x67,0x68,0x6a,0x6b,0x6c,0x3b,0x27,0x00,0x00,0x34,0x35,0x36,
0x00,0x7a,0x78,0x63,0x76,0x62,0x6e,0x6d,0x2c,0x2e,0x2f,0x00,0x2e,0x37,0x38,0x39,
0x20,0x08,0x09,0x0d,0x0d,0x1b,0x7f,0x00,0x00,0x00,0x2d,0x00,0x00,0x00,0x00,0x00
};
	
local char shftChars[] = {
0x7e,0x21,0x40,0x23,0x24,0x25,0x5e,0x26,0x2a,0x28,0x29,0x5f,0x2b,0x7c,0x08,0x00,
0x51,0x57,0x45,0x52,0x54,0x59,0x55,0x49,0x4f,0x50,0x7b,0x7d,0x0d,0x31,0x32,0x33,
0x41,0x53,0x44,0x46,0x47,0x48,0x4a,0x4b,0x4c,0x3a,0x22,0x00,0x00,0x34,0x35,0x36,
0x00,0x5a,0x58,0x43,0x56,0x42,0x4e,0x4d,0x3c,0x3e,0x3f,0x00,0x2e,0x37,0x38,0x39,
0x20,0x08,0x09,0x0d,0x0d,0x1b,0x7f,0x00,0x00,0x00,0x2d,0x00,0x00,0x00,0x00,0x00
};

#define NoChar 0xff


InitBoard() {}

/****************************************************************/
/* 								*/
/*  MGetCh: equivalent to the old MayGetCh();			*/
/* 	returns(0xff) if no keys have been hit, otherwise	*/
/*   returns the key.						*/
/* 								*/
/*   Special Keys (non-ascii) are returned as 2 chars:		*/
/*      a null (0) character, followed by a character specific	*/
/* 	to the key hit. The codes returned following the 	*/
/* 	null are:						*/
/* 	    	Function Key Fn: 0x3a + n			*/
/* 		Cursor Up: 	0x50				*/
/* 		Cursor Down: 	0x48				*/
/* 		Cursor Right: 	0x4d				*/
/* 		Cursor Left: 	0x4b				*/
/* 								*/
/****************************************************************/
SHORT dbgkbd = 0;

local SHORT specChar;
local SHORT specialKey = FALSE;
local BYTE cursChar[4] = {0x50,0x48,0x4d,0x4b};
local Special(c) BYTE c; { specialKey = TRUE; specChar = c; }

#define ESC 0x1b

/* use this for getting second char of 2 char sequence */
SHORT GetNextCh() {
    if (specialKey) { specialKey = FALSE; return(specChar); }
    else return(NoChar);
    }

SHORT CharFromKeyCode(rcode) UBYTE rcode; {
    UBYTE code,key;
    SHORT downStroke;
/*  if (rcode ==NULL) return(NoChar); */ 
    if (dbgkbd)  KPrintF(" rcode = %lx \n",rcode); 
    code = 0x7f&rcode;
    if ( (code&0x70) == 0x70) return(NoChar);
    downStroke = ((rcode&0x80)==0);
    if (code<0x4b) {
	if (!downStroke) return(NoChar);
	key = (Caps||(Lshift|Rshift))? shftChars[code]: unshChars[code];
	return((SHORT)key);
	}
    switch (code) {
	case 0x45: return(ESC); break;
    	case 0x60: Lshift = downStroke; ShiftKey = Lshift|Rshift; return(NoChar); break;
    	case 0x61: Rshift = downStroke; ShiftKey = Lshift|Rshift;return(NoChar); break;
    	case 0x62: Caps = downStroke; return(NoChar); break;
    	case 0x63: Ctrl = downStroke; return(NoChar); break;
    	case 0x64: LeftAlt = downStroke; AltKey = LeftAlt|RightAlt; return(NoChar); break;
    	case 0x65: RightAlt = downStroke; AltKey = LeftAlt|RightAlt;return(NoChar); break;
    	case 0x66: LeftCom = downStroke; return(NoChar); break;
    	case 0x67: RightCom = downStroke; return(NoChar); break;
	case 0x5f: Special(0x01); return(NULL); break; /*HELP key*/
	default:
	    if (!downStroke) return(NoChar);
	    if ((0x50<=code)&&(code <= 0x59))  /* function keys */
		{ Special(0x3b + code-0x50); return(NULL); }
	    if ((0x4c<=code)&&(code <= 0x4f))  /* cursor keys */
		{ Special(cursChar[code - 0x4c]); return(NULL); }
	    return(NoChar);
	    break;
	}
    }

