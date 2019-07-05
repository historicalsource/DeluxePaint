/*----------------------------------------------------------------------*/
/*									*/
/*		fontinit.c						*/
/*									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <librarie\dosexten.h>
#include <prism.h>
#include <librarie\diskfont.h>

#define local static
extern LONG DiskfontBase;
extern struct TmpRas tmpRas;
extern struct AvailFonts *fontDir;
extern SHORT nFonts;
extern SHORT curFontNum;
extern struct TextFont *font;
extern struct Menu *FontMenu;
extern struct Window *mainW;
extern struct Menu *MainMenu;

#ifdef junkjunkjunkjunk

------- STUFF from <librarie\diskfont.h> for reference :

#define		AFB_MEMORY	0
#define		AFF_MEMORY	1
#define		AFB_DISK	1
#define		AFF_DISK	2
/****** TextAttr node, matches text attributes in RastPort **********/
struct TextAttr {
    STRPTR  ta_Name;		/* name of the font */
    UWORD   ta_YSize;		/* height of the font */
    UBYTE   ta_Style;		/* intrinsic font style */
    UBYTE   ta_Flags;		/* font preferences and flags */
};

struct AvailFonts {
	UWORD	af_Type;		/* MEMORY or DISK */
	struct	TextAttr af_Attr;	/* text attributes for font */
	};

struct AvailFontsHeader {
	UWORD	afh_NumEntries;		/* number of AvailFonts elements */
    /*	struct	AvailFonts afh_AF[]; */
    };
#endif
/*----------------------------------------------------------------------*/

#define BUFSIZE 800


local CopyPrefix(to,from) UBYTE *to,*from; {
    UBYTE c;
    while ( (c = *from++) != '.')  *to++ = c; 
    *to++ = '-';
    *to = '\0';
    }

/* -----------initialize font directory */
local void InitFonts() {
    int i,err,fontType;
    UBYTE *str, name[20], sHigh[5],*buffer;
    struct AvailFonts *avFonts;
    struct AvailFontsHeader *avh = (struct AvailFontsHeader *)tmpRas.RasPtr;
    buffer = (UBYTE *)tmpRas.RasPtr;
    
    fontType = AFF_MEMORY;
    DiskfontBase = OpenLibrary("diskfont.library",0);
    if (DiskfontBase== NULL){
	InfoMessage("Couldn't open","diskfont.library" );
	return;
	}
    else fontType |= AFF_DISK;

    ZZZCursor();
    err = AvailFonts(avh, tmpRas.Size, fontType);
    nFonts = avh->afh_NumEntries;

#if 0
    printf("err = %ld, #found = %ld\n", err, nFonts);
#endif

    avFonts = (struct AvailFonts *)&(buffer[2]); 
    fontDir = (struct AvailFonts *)DAlloc(nFonts*sizeof(struct AvailFonts), MEMF_PUBLIC);

    movmem(avFonts,fontDir,nFonts*sizeof(struct AvailFonts));

    /* copy the font names into strings allocated with AllocMem,
       adding the size to the name */
    for (i=0; i<nFonts; i++)  {

#if 0
	printf("Font '%s', type = %lx\n",avFonts[i].af_Attr.ta_Name,
	   avFonts[i].af_Type);
#endif

#define SAVINGSPACE

#ifdef SAVINGSPACE
	CopyPrefix(name, avFonts[i].af_Attr.ta_Name);
	stcu_d(sHigh,avFonts[i].af_Attr.ta_YSize,3);
	strcat(name,sHigh); 
	str =(UBYTE *)AllocMem(strlen(name)+1,MEMF_PUBLIC);
	strcpy(str,name);
#else
	str =(UBYTE *)AllocMem(strlen(avFonts[i].af_Attr.ta_Name)+1,MEMF_PUBLIC);
	strcpy(str,avFonts[i].af_Attr.ta_Name);
#endif
	fontDir[i].af_Attr.ta_Name = str;
	}
    InitFontMenu();
    UnZZZCursor();
    }

static struct IntuiText itInit = {0,1,JAM1,0,1,NULL,NULL,NULL};

#define AllocStruct(astr) (struct astr *)AllocMem( sizeof(struct astr), MEMF_PUBLIC);

    
/* Build a font menu, allocating a MenuItem and IntuiText (which points 
at the same name as the font Directory) */
local InitFontMenu() {
    int i;
    struct MenuItem *mi, *nextMI;
    struct IntuiText *it;
    LONG mutExSet = (1 << nFonts) -1;
    nextMI = NULL;
    for (i = nFonts-1; i>=0; i--) {
	mi = (struct MenuItem *)AllocStruct(MenuItem);
	it = (struct IntuiText *)AllocStruct(IntuiText);
	movmem(&itInit,it,sizeof(struct IntuiText));
	it->IText = fontDir[i].af_Attr.ta_Name,
	mi->TopEdge = 0;
	mi->SelectFill = (APTR)NULL;
	mi->ItemFill = (APTR)it;
	mi->Flags = (CHECKIT | ITEMTEXT | ITEMENABLED | HIGHCOMP);
	mi->MutualExclude = mutExSet & ( ~(1<<i) );
	mi->NextItem = nextMI;
	mi->SubItem = NULL;
	nextMI = mi;
	}
    FontMenu->FirstItem = nextMI; 
    FontMenu->FirstItem->NextItem->Flags |= CHECKED;
    }

#define FreeString(s)  FreeMem((s),strlen(s)+1)
    
void FreeFonts() {
    int i;
    struct MenuItem *mi, *nextMI=NULL;
    MyCloseFont(font);
    if (fontDir == NULL) return;
    /* free the font MenuItems and IntuiText's  */
    for (mi = FontMenu->FirstItem; mi != NULL; mi = nextMI) {
	nextMI = mi->NextItem;
	FreeMem( mi->ItemFill, sizeof(struct IntuiText));
	FreeMem( mi, sizeof(struct MenuItem));
	}
    FontMenu->FirstItem = NULL;
    for (i=0; i<nFonts; i++) FreeString(fontDir[i].af_Attr.ta_Name);
    DFree(fontDir);
    CloseLibrary(DiskfontBase);
    }

void LoadFontDir() {
    UndoSave();
    InitFonts();
    ClearMenuStrip(mainW);
    InitMenu(); 
    SetMenuStrip(mainW, MainMenu);
    PaneRefrAll(); /* in case it put up requestor to switch disks*/
    /* may have made storage tight by loading the font directory:
      if so deallocate the brush */
    if (AvailMem(MEMF_PUBLIC|MEMF_CHIP) < 17000) GunTheBrush();
    }
