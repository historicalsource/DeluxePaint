/*--------------------------------------------------------------*/
/*								*/
/*        text.c  -- text manager	 			*/
/*								*/
/*--------------------------------------------------------------*/
#include <system.h>
#include <prism.h>
#include <pnts.h>
#include <librarie\diskfont.h>

extern struct Window *mainW;
extern struct Menu *MainMenu;
extern struct TextFont *OpenDiskFont();
extern struct TextFont *OpenFont();
extern struct Menu *FontMenu;
extern PointRec pnts;
extern struct RastPort *mainRP;
extern BOOL everyTime;
extern SHORT curxpc;
extern struct RastPort *curRP;
extern void (*nop)();
extern SHORT lastPermMode, chgToMode;
extern void (*killMode)();
extern BOOL fbOn;

#define local static

/* Text data (in root ) */
extern SHORT cx,cy,cwidth;
extern char textBuf[80];
extern SHORT textX, textY;
extern SHORT textX0,cw,ch,cyoffs;
extern SHORT ptext;
extern SHORT svmd;
extern BOOL tblinkOn;
extern SHORT tbCount;
    
InvTxtBox() {  InvTxtXY(mx,my); }

local InvTxtXY(x,y) SHORT x,y; {
    Box bx;
    TempXOR(); 
    mFrame(MakeBox(&bx,x,y-cyoffs,cw,ch)); 
    RestoreMode(); 
    }

/* TextBlinker  */
local InvBlink() { InvTxtXY(textX,textY); }
local TBlink() { if (++tbCount == 300) { 
	tbCount = 0; tblinkOn = !tblinkOn; InvBlink();}
	}


local TBlinkClear() { if (tblinkOn)  InvBlink(); tblinkOn = NO; fbOn = NO; }
local TBlinkShow() { if (!tblinkOn) InvBlink(); tblinkOn = YES; }

local int  CLength(c) char c; { return(TextLength(curRP, &c, 1 )); }
    
local void killText() {
    TBlinkClear();
    PlugChProc(NULL);  
    SetPeriodicCall(NULL); 
    }

local void DoTextChar(c) char c;	{
    Box bx;
    TBlinkClear(); UpdtON();
    switch(c) {
	case ESC: UpdtOFF();  killText(); chgToMode = svmd;
		return; break;
	case BS: if (ptext>0) {
	    c = textBuf[--ptext];
	    bx.w = CLength(c);
	    textX -= bx.w;
	    bx.x = textX;   bx.y = textY - cyoffs;
	    bx.h = ch;
	    EraseBox(&bx);
	    } break;
	case CR: textX = textX0; textY += ch; ptext = 0; break;
	case 0: switch(GetNextCh()) {
	case CLeft: textX -= cw; break;
	    case CRight: textX += cw; break;
	    case CDown: textY += ch; break;
	    case CUp: textY -= ch; break;
	    } break;
	default:mDispChar(c,textX,textY); textX += CLength(c); textBuf[ptext++]=c; break;
	}
    textX0 = MIN(textX0,textX);
    UpdtOFF(); 
    TBlinkShow();
    }

local void dummy(){}

local void stopBlink() {    SetPeriodicCall(NULL);     PlugChProc(nop);   }

local void StartText() {
    ptext = 0;
    textX0 = textX = mx;
    textY = my;
    }

void IMText2() {
    killMode = &killText; 
    SetPeriodicCall(&TBlink); 
    PlugChProc(&DoTextChar);
    IModeProcs(TBlinkShow,dummy,TBlinkClear,stopBlink,
	InvTxtBox,nop,InvTxtBox,StartText);
    }

local GetTextSizeParams() {
    cw = curRP->TxWidth; ch = curRP->TxHeight;
    cyoffs = curRP->TxBaseline;
    }
    
IMText1() {
    svmd = lastPermMode;
    GetTextSizeParams();
    killMode = &killText; 
    PlugChProc(nop);
    IModeProcs(&InvTxtBox,nop,&InvTxtBox,nop,
			&InvTxtBox,nop,&InvTxtBox,&StartText);
    }


extern struct AvailFonts *fontDir;
extern SHORT nFonts;
extern SHORT curFontNum;
extern struct TextFont *font;

LoadAFont(item) SHORT item; {
    struct TextAttr *fa;
    UBYTE fontName[20], *dispName, *nm, *fn;
    SHORT type;
    struct TextFont *oldFont = font;

    fa = &fontDir[item].af_Attr;

/* restore name to its original 'foo.font' form */
    dispName = fa->ta_Name;		/* save the name */
    for (nm = dispName, fn = fontName; *nm  != '-';  nm++) *fn++ = *nm;
    *fn = 0;
    strcat(fontName,".font");
    fa->ta_Name = fontName;

    type = fontDir[item].af_Type;
    ZZZCursor();

    if (type == AFF_DISK) {
	UndoSave(); /* it may put up requestor */
	font = OpenDiskFont(fa);
	PaneRefrAll(); /* in case put up requestor */
	}

    else font = OpenFont(fa);

#ifdef DBGFONT
    printf(" Opened font named %s, access = %ld \n",fontName, 
	font->tf_Accessors);

    if (oldFont!=NULL)
     printf(" Closing font named %s, access = %ld. \n",
	oldFont->tf_Message.mn_Node.ln_Name, oldFont->tf_Accessors);
#endif

    MyCloseFont(oldFont);

    curFontNum = -1;
    
    if (font == NULL) InfoMessage(" Couldn't open font", "");
    else { SetFont(mainRP,font); curFontNum = item; }

    FreeUpMem();    
    if (AvailMem(MEMF_PUBLIC|MEMF_CHIP) < 12000) GunTheBrush(); 

    fa->ta_Name = dispName;

    GetTextSizeParams();
    UnZZZCursor();
    }
    
extern SHORT clipMaxX, xorg,yorg;

mDispChar(c,x,y) char c; SHORT x,y; {
    Box bx;
    SHORT svmd;
    char s[2];
    s[0] = c;    s[1] = 0;
    if ( (x+xorg+CLength(c)) <= clipMaxX) {
	Move(curRP,x +xorg,y + yorg);
	svmd = curRP->DrawMode;
	SetDrMd(curRP,JAM1);
	Text(curRP, s, 1);
	SetDrMd(curRP,svmd);
	UpdtSibs(MakeBox(&bx, x , y-curRP->TxBaseline, 
		curRP->TxWidth, curRP->TxHeight));
	}
    }
    
