/*----------------------------------------------------------------------*/
/*									*/
/*		chproc.c -- Main character dispatch proc		*/
/*									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>
#include <pnts.h>

extern PointRec pnts; /* contains mx,my,sx,sy, etc */
extern BOOL Ctrl;
extern BOOL modeHelp;
extern void ShadMode(),NullMode(),SelBrush(),CircMode();
extern void FCircMode(),DrawMode(),OvalMode(),FOvalMode();
extern void AFillMode(),BFillMode(),CurvMode(),RectMode();
extern void FRectMode(),VectMode(),BrStretch(),TextMode(),ReadMode();
extern void ToglMagW();
extern short curpen,curFormat;
extern struct Window *mainW;
extern SHORT getch();
extern BOOL kbhit();
extern BOOL titleShowing;
extern BOOL Painting,gridON,inside;
extern struct RastPort *curRP,tempRP;
extern struct ViewPort *vport;
extern struct BitMap hidbm;
extern struct ViewPort *vport;
extern SHORT curxpc,nColors,curIMode;
extern Box screenBox,bigBox;
extern struct TmpRas tmpRas;
extern SHORT curPixFmt, xShft, yShft, nColors;

SHORT chgToMode = IM_none;

#define local static

IncFG(inc) SHORT inc; { CPChgCol((nColors-1)&(curRP->FgPen + inc));  }

IncBG(inc) SHORT inc; { CPChgBg((nColors-1)&(curxpc + inc));  }

void (*tempChProc)() = NULL;

PlugChProc(proc) void (*proc)(); {    tempChProc = proc;    }

whoa() {}

DispAndSetMode(m) int m; {
    SetModeMenu(m);
    DispPntMode();
    }

IncPen() { CPChgPen(curpen?curpen+1:RoundB(1));   }

DecPen() { if ((curpen&0xfff) > 1) CPChgPen(curpen-1);
    else CPChgPen(0);  }


HelpMe() {
    BasicBandW();
    CPChgPen(0);
    NewIMode(IM_draw);
    DispAndSetMode(modeHelp? Color: Mask);
    SymState(NO);
    SetGridSt(NO);
    SetGridXY(8,8);
    SetGridOrg(0,0);
    ShowCtrPan(YES);
    if (!titleShowing) TogTitle();
    RemoveMag();
    KillCCyc();
    }

/* #define accelScroll */ /* BUGGY */

scrollKeys(wh,i,j) Pane *wh; int i,j; {
    SHORT c;
    int xs = PMapX(8);
    int ys = PMapY(8);
    MWScroll(wh,i*xs,j*ys);
    while (kbhit()) {
	i = j = 0;
	while (kbhit()) if (getch()==0) {
	    c = GetNextCh();
#ifdef accelScroll
	    switch(c) {
		case CLeft:  i++; break;
		case CRight: i--; break;
		case CUp:    j++; break;
		case CDown:  j--; break;
		}
#endif
	    }
#ifdef accelScroll
	if ((i!=0)||(j!=0)) MWScroll(wh,i*xs,j*ys);
#endif
	}
    }

Half() {  if (curpen==USERBRUSH) BrHalf();
    else CPChgPen((curpen&0xf000)|((curpen&0xfff)>>1));
    }

Dubl() {  if (curpen==USERBRUSH) BrDubl();
    else CPChgPen((curpen&0xf000)|MAX((curpen&0xfff)<<1,1));
    }	

    
/* toggle grid, but adust grid offset so brushes doesn't move */
extern SHORT gridx,gridy, gridOrgX, gridOrgY;
extern BOOL gridON;
extern BMOB *curob;
extern Point nogrid;
    
/* move grid is on the grid point so when
    toggle grid brush doesnt move*/ 
void TogGridFixed() { 
    SHORT tx,ty;
    TogGridSt();
    if (gridON) {
	tx = mx; ty = my;
	Gridify(&tx,&ty);
	gridOrgX += (mx - tx);
	gridOrgY += (my - ty);
	}
    }

/* --Process keyboard character(s) ----*/ 

void mainCproc(pn,c) Pane *pn; char c; { 
    BOOL svpnt = Painting;
    if (tempChProc!=NULL) {
	(*tempChProc)(c); 
	/* this lets text, when <esc>, change to mode in parallel overlay */
	if (chgToMode!=IM_none) {
	    NewIMode(chgToMode);
	    chgToMode = IM_none;
	    }
	return;
	}
    ClearFB();
    switch(c) { 
	/*---------Change Color Pallete( use Keypad ) */
	case '7': IncRGB(-REDC); break;
	case '8': IncRGB( REDC); break;
	case '4': IncRGB(-GREENC); break;
	case '5': IncRGB( GREENC); break;
	case '1': IncRGB(-BLUEC); break;
	case '2': IncRGB( BLUEC); break;
	
	case '.': CPChgPen(0); break;		/*  1 pixel Pen */
	case '-': if (curIMode == IM_airBrush) ChgABRadius(-1);
		    else DecPen(); break; /* cycle thru pens */
	case '=': if (curIMode == IM_airBrush) ChgABRadius(1);
		   else IncPen(); break;

	case '[': IncFG(-1); break;	/* Cycle thru foreground cols*/
	case ']': IncFG(1); break;	
	
	case '{': IncBG(-1); break;	/* cycle thru background cols*/
	case '}': IncBG(1); break;
	
	case '/': TogSymSt(); break;	/* Symmetry on and off */
	
	case ' ': AbortIMode(); break;	/* Kills any action */
	
	case ',': NewIMode(IM_readPix); break;  /* Read color from 
		screen-- Left Button for FG col, Right Button for BG */
	
	case TAB: TogColCyc();  break;
	
	case 'j': SwapSpare(); break;
#ifdef agagag
	/*----- Rudimentary File IO  */
	case 'i': LoadPicNamed("temp.pic"); break;
	case 'o': SavePicNamed("temp.pic"); break;
	case 'I': LoadPicNamed("temp1.pic"); break;
	case 'O': SavePicNamed("temp1.pic"); break;
#endif

	case 'a': if (Ctrl) DispAvailMem(); else NewIMode(IM_airBrush);  break;
	case 'A': SizeAirBrush();  break;
		
	case 'b': NewIMode(IM_selBrush); break; /* select a new brush*/
	case 'B': UserBr(); break;		/* use last brush selected */

/* The rest is like PC Prism */		
	case 'c': NewIMode(IM_circ); break; 	/*  draw circle */	
	case 'C': NewIMode(IM_fcirc); break; 	/* fill circle */
	case 'd': NewIMode(IM_draw); break;  	/* draw  */		
	case 'D': CPChgPen(0); NewIMode(IM_draw); break; /* draw with 1-pixel pen */
	case 'e': NewIMode(IM_oval); break;	/* ellipse */
	case 'E': NewIMode(IM_foval); break;	/* filled ellipse*/ 
	case 'f': NewIMode(IM_fill); break;	/* flood area*/
	case 'g': TogGridSt(); break;		/* grid on-off */
	case 'G': TogGridFixed();break;  	/* grid on-off, keep pos fixed */
	case 'h': Half(); break;		/* halve brush size */
	case 'H': Dubl(); break;		/* double brush size */
	case 'K': mClearToBg(); break;		/* clear screen to b.g. color */	
#ifdef OWell
	case 'm': ShowMag(mx,my); break; 	/* show magnify window*/
	case 'n': RemoveMag(); break; 		/* remove mag window */
#else
	case 'm': ToggleMag(); break; 	/* show magnify window*/
	case 'n': CenterPoint(mx,my); break;		/* remove mag window */
#endif
	case 'p': ShowPallet(); break;		/* Show the RGB-HSV pallet*/
	case 'q': NewIMode(IM_curve1); break; 	/* curve */	
	case 'r': NewIMode(IM_rect); break;	/* rectangle */
	case 'R': NewIMode(IM_frect); break;	/*filled rectangle */
	case 's': NewIMode(IM_shade); break;	/* shade */
	case 't': NewIMode(IM_text1); break; 	/* text */
	case 'u': Undo(); break;		/* Undo  */
	case 'v': NewIMode(IM_vect); break;	 /* straight lines */
	case 'x': BrFlipX(); break;		/* Flip brush horizontally */
	case 'y': BrFlipY(); break;		/* Flip brush vertically */
	case 'z': BrRot90(); break;		/* Rotate brush 90 degrees */
	case 'Z': NewIMode(IM_strBrush); break; 	/* stretch brush */
	case '>': MWZoom(1); break; 		/* Zoom mag window in*/
	case '<': MWZoom(-1); break;	 	/* Zoom mag window out */
	case 'w': whoa(); break;		/* for break points */
	case 0: switch(c = GetNextCh()) { 
	
	    /* Set painting mode with function keys F1..F7 */
	    case F1Key:  case F2Key:   case F3Key:
	    case F4Key:  case F5Key:   case F6Key:
	    case F7Key:  DispAndSetMode(c-F1Key); break;

	    case HELPKEY: HelpMe(); break;
		
	    /* Turn cursor On/Off  */
	    case F8Key: TogCursor(); break; 
	    
	    /* Show/Hide Title Bar */
	    case F9Key: TogTitle(); break;
	    
	    /* Show/Hide Graphics Menu (Control panel) */
	    case F10Key: TogBoth(); break;
	    
	    /* Scroll Window contents  with Arrow keys */
	    case CLeft: scrollKeys(pn,1,0); break;
	    case CRight: scrollKeys(pn,-1,0); break;
	    case CUp: scrollKeys(pn,0,1); break;
	    case CDown: scrollKeys(pn,0,-1); break;
	    }
	break;
	}
    LoadIMode(); /* incase overlay happened */
    if (svpnt) UpdtON(); else UpdtOFF();
    PaneFakeMove(); /* make sure the feedback is turned back on*/
    }

