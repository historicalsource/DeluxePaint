/** fnreq.c ******************************************************************/
/*                                                                           */
/* A file name requester                                                     */
/*                                                                           */
/* Date      Who Changes                                                     */
/* --------- --- ----------------------------------------------------------- */
/* 28-Aug-85 mrp created from imagedit's dos requester                       */
/* 15-Sep-85 DDS modified to do own event processing, take x,y coord.        */
/*                                                                           */
/*****************************************************************************/
/*  1) define a file and a drawer                                            */
/*                                                                           */
/*     UBYTE mydrawer[31] = {""};                                            */
/*     UBYTE myfile[31] = {"test.vcs"};                                      */
/*                                                                           */
/*  2) Issue a request, when the user picks your Load menu item              */
/*                                                                           */
/*  res  = FN_Request(mywindow,x,y,mydrawer,myfile,NULL,NULL,NULL,"Load");   */
/*      where res = TRUE if user did action, FALSE if he cancelled           */
/*                                                                           */
/*****************************************************************************/

#include <system.h>
#include <librarie\dos.h>
#include <fnreq.h>
#include <clib\ctype.h>

#define local

#define YES 1
#define NO 0

#define MAX(a,b)   	((a)>(b)?(a):(b))
#define MIN(a,b)   	((a)<(b)?(a):(b))

#define FN_GADGMIN 10  /* minimum gadget number used by this requester */
#define FN_GADGMAX 29  /* maximum gadget number used by this requester */

#define FN_CANCEL  10  /* user canceled the requester */
#define FN_GOTNAME 11  /* user pressed return on name or picked OK gadget */
#define FN_FILE    12  /* user selected filename gadget */
#define FN_DRAWER  13  /* user selected drawer name gadget */
#define FN_UP      14  /* user selected up one name gadget */
#define FN_DOWN    15  /* user selected down one name gadget */
#define FN_SLIDE   16  /* user did something to proportional gadget */

#define HIWIDTH 320
#define LOWIDTH 264
#define REQW  LOWIDTH                 /* width of requester */ 
#define REQH  147                      /* height of requester */

#define MAXFNAMES 50 	/* max number of file names in a dir */
#define MAXSHOW 8 	/* max number of file names visible in req*/

local struct Window *reqwindow;       /* window requester is in */
local struct Requester fn_req = {0};        /* the requester */
local LONG forbug = 0;		/* fix 1.06 Intuition bug */
local struct IntuiText fn_text[3];    /* requester text */
local struct IntuiText gadg_text[2];  /* gadget text */

local UBYTE deftitle[] = "FR";
local UBYTE defcancel[] = "Cancel";
local UBYTE defdoverb[] = "Ok";

local LONG firstname=0;                 /* index of first name in the list */

BOOL fndbg = NO;
                                                        
struct TextAttr reqfont = {            /* font to use for requester text */
   "topaz.font",
   TOPAZ_SIXTY,                        /* will be set to user's preference */
   0,
   0};

local UBYTE undofile[26];
local UBYTE undodrawer[26];

local SHORT reqX = 0;                      /* leftedge of requester */
local SHORT reqY = 0;                      /* topedge of requester */
local SHORT reqWidth = LOWIDTH;

local BOOL dosready = FALSE;          /* TRUE after requester is initialized */

local UWORD reqpts[] = {              /* xy table of border and reqion lines */
   2,1,
   2,REQH-2,
   REQW-3,REQH-2,
   REQW-3,12,
   2,12,
   2,1,
   REQW-3,1,
   REQW-3,93,
    2,93
    };
        
local struct Border reqborder = {     /* border for the requester */
   0,0,
   0,1,
   JAM1,
   9,
   (SHORT *)&reqpts,
   NULL};

/*=======================*/
/*== CANCEL GADGET ======*/
/*=======================*/

local UWORD dt0box1[] = {0,0,0,0,0,0,0,0,0,0};
local UWORD dt0box2[] = {1,2,0,2,0,0,1,0,1,2};
   
local struct Border dt0bor2 = {    /* Blue box */
   0,0,
   0,1,
   JAM1,
   5,
   (SHORT *)&dt0box2,
   NULL};
local struct Border dt0bor1 = {    /* Orange box */
   0,0,
   3,0,
   JAM1,
   5,
   (SHORT *)&dt0box1,
   &dt0bor2};

local struct Gadget cancelgadget = {
   NULL,          
   -58,-20,
   48,17,         
   GADGHCOMP | GRELBOTTOM | GRELRIGHT,     
   RELVERIFY | GADGIMMEDIATE | ENDGADGET, 
   REQGADGET | BOOLGADGET, 
   (APTR)&dt0bor1,         
   NULL,         
   &gadg_text[0],
   0,            
   NULL,         
   FN_CANCEL,    
   NULL };       

/*=========================*/
/*== GOTNAME, GADGET ======*/
/*=========================*/

local UWORD dt1box1[] = {0,0,0,0,0,0,0,0,0,0};
local UWORD dt1box2[] = {1,2,0,2,0,0,1,0,1,2};
   
local struct Border dt1bor2 = {     /* Blue box */
   0,0,
   0,1,
   JAM1,
   5,
   (SHORT *)&dt1box2,
   NULL};
local struct Border dt1bor1 = {     /* Orange box */
   0,0,
   3,0,
   JAM1,
   5,
   (SHORT *)&dt1box1,
   &dt1bor2};

local struct Gadget gotnamegadget = {
   &cancelgadget,
   10,-20,
   64,17,          
   GADGHCOMP | GRELBOTTOM,        
   RELVERIFY | GADGIMMEDIATE | ENDGADGET, 
   REQGADGET | BOOLGADGET, 
   (APTR)&dt1bor1,         
   NULL,          
   &gadg_text[1], 
   0,          
   NULL,       
   FN_GOTNAME, 
   NULL };


/*======================*/
/*== FILE, GADGET ======*/
/*======================*/

#define FG_WIDTH 184
#define FG_HEIGHT 13       


local UWORD dt2box1[] = {
   -2,-2,
   FG_WIDTH+1,-2,
   FG_WIDTH+1,FG_HEIGHT-3,
   -2,FG_HEIGHT-3,
   -2,-2};

local struct Border dt2bor1 = {
   0,0,
   0,1,
   JAM1,
   5,
   (SHORT *)&dt2box1,
   NULL};

local struct StringInfo filestr = {
   NULL,          /* default and final string */
   &undofile[0],  /* optional undo buffer (later) */
   0,             /* character position in buffer */
   25,            /* max characters in buffer */
   0,             /* buffer position of first displayed character */
   0,0,0,0,0,NULL,0,NULL };     /* intuition local variables */

local struct Gadget filegadget = {
   &gotnamegadget,
   -FG_WIDTH-12,-33,
   FG_WIDTH,FG_HEIGHT,
   GADGHCOMP | GRELBOTTOM | GRELRIGHT,
   RELVERIFY | GADGIMMEDIATE /* | ENDGADGET */ , 
   REQGADGET | STRGADGET,
   (APTR) &dt2bor1,      
   NULL,                 
   NULL,                 
   0,                    
   (APTR)&filestr,    
   FN_FILE,           
   NULL };            

/*========================*/
/*== DRAWER, GADGET ======*/
/*========================*/

#define DG_WIDTH 184
#define DG_HEIGHT 13       


local UWORD dt3box1[] = {
   -2,-2,
   DG_WIDTH+1,-2,
   DG_WIDTH+1,DG_HEIGHT-3,
   -2,DG_HEIGHT-3,
   -2,-2};

local struct Border dt3bor1 = {
   0,0,
   0,1,
   JAM1,
   5,
   (SHORT *)&dt3box1,
   NULL};

local struct StringInfo drawerstr = {
   NULL,          
   &undodrawer[0],
   0,             
   25,            
   0,             
   0,0,0,0,0,NULL,0,NULL };

local struct Gadget drawergadget = {
   &filegadget,         
   -DG_WIDTH-12,-48,
   DG_WIDTH,DG_HEIGHT,  
   GADGHCOMP | GRELBOTTOM | GRELRIGHT,
   RELVERIFY | GADGIMMEDIATE, 
   REQGADGET | STRGADGET,
   (APTR) &dt3bor1,      
   NULL,                 
   NULL,                 
   0,                    
   (APTR)&drawerstr,     
   FN_DRAWER,            
   NULL };               


/*========================*/
/*== SLIDER, GADGET ======*/
/*========================*/

local struct Image slideknob = {
   0,0,
   0,
   0,0,
   0,
   0,
   0,
   NULL};

local struct PropInfo slideprop =
   {
   AUTOKNOB | FREEVERT,
   0,0,
   0xffff,0x5555,
   12,52,
   0,0,
   2,0
   };
         
struct Gadget slidegadget =
   {
   &drawergadget,
   -19,27,
   20,52,
   GRELRIGHT | GADGIMAGE | GADGHCOMP | GADGHBOX,
   RELVERIFY | GADGIMMEDIATE,
   PROPGADGET| REQGADGET,
   (APTR) &slideknob,
   NULL,
   NULL,
   NULL,
   (APTR) &slideprop,
   FN_SLIDE,
   NULL
   };

/*====================*/
/*== UP, GADGET ======*/
/*====================*/
extern SHORT up0[];

local struct Image uparrow = {
   0,0,
   16,
   16,1,
   (USHORT *)&up0,
   0x01,
   0x00,
   NULL};

local struct Gadget upgadget = {
   &slidegadget,
   -17,12,
   16,16,          
   GADGHCOMP | GRELRIGHT | GADGIMAGE, 
   RELVERIFY | GADGIMMEDIATE, 
   REQGADGET | BOOLGADGET, 
   (APTR)&uparrow,         
   NULL,          
   NULL,      
   0,         
   NULL,      
   FN_UP,     
   NULL };

/*======================*/
/*== DOWN, GADGET ======*/
/*======================*/
extern SHORT down0[];

local struct Image downarrow = {
   0,0,
   16,
   16,1,
   (USHORT *)&down0,
   0x01,
   0x00,
   NULL};

local struct Gadget downgadget = {
   &upgadget,
   -17,78,
   16,16,          
   GADGHCOMP | GRELRIGHT | GADGIMAGE,
   RELVERIFY | GADGIMMEDIATE, 
   REQGADGET | BOOLGADGET, 
   (APTR)&downarrow,          
   NULL,     
   NULL,     
   0,        
   NULL,     
   FN_DOWN,  
   NULL };

              
/*========================*/
/*== NAMES, GADGETS ======*/
/*========================*/

SHORT NFNames = 0; 	/* The number of file names in currren dir */
UBYTE *names[MAXFNAMES] = {NULL};
UBYTE namedir[MAXFNAMES];

local struct IntuiText nametext[MAXSHOW];

#define MAXCHARS 30
typedef UBYTE NameStr[MAXCHARS];
NameStr  *showNames;

local struct Gadget namegadget[MAXSHOW] = {0};

AllocShowNames() {
    showNames = (NameStr *)DAlloc(MAXSHOW*sizeof(NameStr), MEMF_PUBLIC);
    }
    
FreeShowNames() {  DFree(showNames);  }
    
FN_InitReq()
   {
   LONG n;
   InitRequester(&fn_req);
   fn_req.LeftEdge = reqX;
   fn_req.TopEdge = reqY;
   fn_req.Width = reqWidth;
   fn_req.Height = REQH;
   fn_req.ReqGadget = &namegadget[MAXSHOW-1];
   fn_req.ReqBorder = &reqborder;
   fn_req.ReqText = &fn_text[2];   
   fn_req.BackFill = 1;

   fn_text[0].FrontPen = 3;
   fn_text[0].BackPen = 1;
   fn_text[0].DrawMode = JAM2;
   fn_text[0].ITextFont = &reqfont;
   fn_text[0].NextText = NULL;
   fn_text[0].IText = NULL;
   fn_text[0].LeftEdge = 0;
   fn_text[0].TopEdge = 3;

   fn_text[1].FrontPen = 0;
   fn_text[1].BackPen = 1;
   fn_text[1].DrawMode = JAM2;
   fn_text[1].ITextFont = &reqfont;
   fn_text[1].NextText = &fn_text[0];
   fn_text[1].LeftEdge = 5;
   fn_text[1].TopEdge = REQH-49;
   fn_text[1].IText = (UBYTE *)"Drawer:";

   fn_text[2].FrontPen = 0;
   fn_text[2].BackPen = 1;
   fn_text[2].DrawMode = JAM2;
   fn_text[2].ITextFont = &reqfont;
   fn_text[2].NextText = &fn_text[1];
   fn_text[2].LeftEdge = 21;
   fn_text[2].TopEdge = REQH-34;
   fn_text[2].IText = (UBYTE *)"File:";
   
   for(n=0;n<MAXSHOW;n++)     {
	nametext[n].FrontPen = 0;
	nametext[n].BackPen = 1;
	nametext[n].DrawMode = JAM2;
	nametext[n].ITextFont = &reqfont;
	nametext[n].NextText = NULL;
	nametext[n].LeftEdge = 2;
	nametext[n].TopEdge = 1;   
	}
    reqpts[4] = reqpts[6] = reqpts[12] = reqpts[14] = reqWidth-3;
    }

/*****************************************************************************/
/* FN_FixBox                                                                 */
/*                                                                           */
/* Adjust a box's corners to a specified width and height                    */
/*                                                                           */
/*****************************************************************************/

VOID FN_FixBox(box1,box2,width,height)
UWORD box1[],box2[];
UWORD width,height;
   {
   box1[2] = width-1;
   box1[4] = width-1;
   box1[5] = height-1;
   box1[7] = height-1;
   box2[2] = width-2;
   box2[4] = width-2;
   box2[5] = height-3;
   box2[7] = height-3;
   }               
                                                         
/*****************************************************************************/
/* FN_InitText                                                               */
/*                                                                           */
/* Initialize text used in this requester                                    */
/*                                                                           */
/*****************************************************************************/
VOID FN_InitText()
   {
   SHORT n;
   for(n=0; n<2; n++)
      {
      gadg_text[n].FrontPen = 0;
      gadg_text[n].BackPen = 1;
      gadg_text[n].DrawMode = JAM2;
      gadg_text[n].ITextFont = &reqfont;
      gadg_text[n].NextText = NULL;
      gadg_text[n].LeftEdge = 4;
      gadg_text[n].TopEdge = 4;
      }
   gadg_text[0].IText = NULL;
   gadg_text[1].IText = NULL;   

   for(n=0;n<MAXSHOW;n++)
      {
      if (n==0) namegadget[n].NextGadget = &downgadget;
      else namegadget[n].NextGadget = &namegadget[n-1];
      namegadget[n].LeftEdge = 3;
      namegadget[n].TopEdge = 13+(n*10);
      namegadget[n].Width = REQW-22;
      namegadget[n].Height = 9;
      namegadget[n].Flags = GADGHCOMP;
      namegadget[n].Activation = RELVERIFY | GADGIMMEDIATE;
      namegadget[n].GadgetType = REQGADGET | BOOLGADGET;
      namegadget[n].GadgetID = FN_SLIDE + 1 + n;
      namegadget[n].GadgetText = &nametext[n];
      }
   }               

UBYTE emptyName[] = "";
int nrange = 0;
#define MAXCHWIDE 20
StuffReqNames() {
    int i,n,j;
    int newpot,newbody;
    if (firstname<0) firstname = 0;
    if (NFNames<=MAXSHOW) firstname = 0;
    else if (firstname > (NFNames-MAXSHOW)) firstname = NFNames-MAXSHOW;
    n = firstname;
    for (i=0; i<MAXSHOW; i++)  {
	if (n<NFNames)  strncpy(showNames[i],names[n],MAXCHARS);
	else showNames[i][0] = 0;
	nametext[i].IText = showNames[i];
	for (j = strlen(showNames[i]); j<MAXCHARS-1; j++)
		showNames[i][j] = ' ';
	showNames[i][MAXCHARS-1] = 0;
	n++;
	}
    nrange = NFNames - MAXSHOW + 1;
    if (nrange<=0)   {  newpot = 0x0;  newbody = 0xffff;   }
    else  {
	newpot = (firstname<<16)/nrange;
	newbody = (MAXSHOW<<16)/NFNames;
	}
#ifdef foob
    printf("newpot, newbody = %4lx %4lx\n",newpot,newbody);
    printf("NFNames = %ld\n",NFNames);
    printf("firstname = %ld\n",firstname);                 
#endif
    slideprop.VertBody = newbody;
    slideprop.VertPot = newpot;
    }
    

FreeFileNames() {
    int n;
    for(n=0; n<MAXFNAMES; n++) 	{
	if (names[n] != NULL) DFree(names[n]);
	names[n] = NULL;
	namedir[n] = FALSE;
	}
    }

local BOOL okDrawer;

strToLower(d,s) char *d,*s; {
    char c;
    do { c = *s++;  *d++ = tolower(c); } while(c!=0);
    }
    
CmpStr(s,t) char *s,*t; {
    char a[40],b[40];
    strToLower(a,s);
    strToLower(b,t);
    return(stscmp(a,b));
    }

local UBYTE dotinfo[] = ".info";

DotInfo(nm) char *nm; {
    int l = strlen(nm);
    if (l<5) return(NO);
    return( strcmp(dotinfo,nm+l-5) == 0);
    }
/*----------------------------------------------------------------------*/
/* GetFileNames  							*/
/*         Read in all the file names, allocating space for them	*/
/*----------------------------------------------------------------------*/
VOID GetFileNames(drawername) UBYTE drawername[];   {                     
    BOOL success;
    LONG lock;                     
    LONG n, ln, t, bound;
    UBYTE *tmp, tmpb;
    struct FileInfoBlock *fib;                      
    
    ZZZCursor();
    FreeFileNames();
    okDrawer = NO;
    NFNames = 0;
    lock = Lock(drawername,SHARED_LOCK);
    success = (lock!=0);
    if (success) {                      
	fib = (struct FileInfoBlock *)DAlloc(sizeof(struct FileInfoBlock),MEMF_PUBLIC);
	success = Examine(lock,fib);
	if (success)   {
	    okDrawer = YES;
	    n = 0;
	    while (success) {
		success = ExNext(lock,fib);
		if (success&&(n<MAXFNAMES))    {
		    ln = strlen(fib->fib_FileName);
		    if (fib->fib_DirEntryType>0) { 
			namedir[n] = TRUE;
			ln += 6;
			}
		    else {
			if (DotInfo(fib->fib_FileName)) continue;
			namedir[n] = FALSE;
			}
		    names[n] = (UBYTE *)DAlloc(ln+1,MEMF_PUBLIC);
		    strcpy(names[n],fib->fib_FileName);
		    if (namedir[n]) strcat(names[n]," (Dir)");
		    n++;
		    }
		} /* end while */                         
	    NFNames = n;
	    }
	DFree(fib);
	UnLock(lock);  
	}
    /** bubble sort file names **/
    t = NFNames-1;
    while (t>0) {
	bound = t; 
	t = 0;
	for (n=0; n<bound; n++) {
	    if (CmpStr(names[n],names[n+1])>0) {
		tmp = names[n]; names[n] = names[n+1]; names[n+1] = tmp;
		tmpb = namedir[n]; namedir[n] = namedir[n+1];
		namedir[n+1] = tmpb;
		t = n;
		}
	    }
	}
    UnZZZCursor();
    } /* end GetFileNames() */

local BOOL doAction;

/*****************************************************************************/
/* FN_GadgUp                                                                 */
/*                                                                           */
/* Handle gadget up events                                                   */
/*                                                                           */
/*****************************************************************************/

RedoNames() {
    StuffReqNames();
    RefreshGadgets(&namegadget[MAXSHOW-1],reqwindow,&fn_req);
    }

VOID FN_GadgUp(gid,x,y) UWORD gid; int x,y;   {     
    LONG n;
    UBYTE tmp[31];
    /* ---- names in the directory display */
    if ((gid >= (FN_SLIDE+1)) && (gid <= (FN_SLIDE+MAXSHOW)))      {
	n = gid - FN_SLIDE - 1 + firstname;
	if (namedir[n]) {
	    strncpy(tmp,names[n],strlen(names[n])-6);
	    ConcatPath(drawerstr.Buffer,tmp);
	    GetFileNames(drawerstr.Buffer);
	    drawerstr.BufferPos = 0; /*strlen(drawerstr.Buffer);*/
	    filestr.DispPos = 0;
	    firstname = 0;
	    StuffReqNames();
	    }             
	else {
	    strcpy(filestr.Buffer,names[n]);
	    filestr.BufferPos = 0; /*strlen(filestr.Buffer);*/
	    filestr.DispPos = 0;
	    }
	/*RefreshGadgets(&namegadget[MAXSHOW-1],reqwindow,&fn_req); */
	/* try to keep it from redrawing whole thing-- didnt work */
	RefreshGadgets(&filegadget,reqwindow,&fn_req); 
	}
    if (gid==FN_SLIDE) {
	firstname = ((slideprop.VertPot*nrange)>>16);
	RedoNames();
	}
    if (gid==FN_DRAWER)  {
	GetFileNames(drawerstr.Buffer);
	firstname = 0;
	RedoNames();
	}
    if ((gid==FN_UP)||(gid==FN_DOWN) ) {
	if (gid==FN_UP) firstname--;	else  firstname++;
	RedoNames();
	}
   }

/*****************************************************************************/
/* Handle events for the File Name requester                                */
/*****************************************************************************/
local BOOL FN_HandleEvent(event) struct IntuiMessage *event; {                                
    WORD gadgetid;
    BOOL keepGoing = YES;
    switch(event->Class)
	{
	case GADGETUP:
	    gadgetid = ((struct Gadget *)event->IAddress)->GadgetID;
	    if (gadgetid == FN_CANCEL) 	keepGoing = NO;
	    else if (gadgetid==FN_GOTNAME) { doAction = TRUE;
		keepGoing = NO;
		}
	    else if ((gadgetid>=FN_GADGMIN) && (gadgetid<=FN_GADGMAX))
		FN_GadgUp(gadgetid);
	    break;
	}
    return(keepGoing);
    }


/*****************************************************************************/
/* FN_Request                                                                */
/*                                                                           */
/* Initialize and display the requester                                      */
/* Returns TRUE if do, FALSE if cancel                                       */
/*****************************************************************************/
BOOL FN_Request(w,x,y,drawername,filename,extension,title,cancelverb,doverb)
struct Window *w; SHORT x,y;
    UBYTE drawername[]; UBYTE filename[]; UBYTE extension[]; UBYTE title[];
    UBYTE doverb[]; UBYTE cancelverb[];                       
    {
    UWORD width0,width1,width,height;
    ULONG saveIDCMP;
    
    
    FixMenCols();
    AllocShowNames();
    reqX = x;
    reqY = y;
    reqwindow = w;
    saveIDCMP = w->IDCMPFlags;
    
    reqWidth = ( (w->WScreen->ViewPort.Modes & (HIRES|LACE)) == 
     (HIRES|LACE)) ? HIWIDTH: LOWIDTH;
    
    /* --- What class of events do we want to know about? --- */

    ModifyIDCMP( reqwindow, RAWKEY | MOUSEBUTTONS |  GADGETUP | GADGETDOWN);

    if (!dosready) {                   
	/* Initialize stuff the first time through */
	GetPrefs(&reqfont.ta_YSize,2); 
	FN_InitReq();
	FN_InitText();
	dosready = TRUE;
	}
    drawerstr.Buffer = drawername;
    filestr.Buffer = filename;

    if (title==NULL) fn_text[0].IText = deftitle;
    else fn_text[0].IText = title;
    fn_text[0].LeftEdge = (reqWidth-10*strlen(fn_text[0].IText))/2;
    
    if (cancelverb==NULL) gadg_text[0].IText = defcancel;
    else gadg_text[0].IText = cancelverb;

    if (doverb==NULL) gadg_text[1].IText = defdoverb;
    else gadg_text[1].IText = doverb;

    width0 = strlen(gadg_text[0].IText)*10+10;
    width1 = strlen(gadg_text[1].IText)*10+10;
    width = MAX(width0,width1);
    height = 17;            

    cancelgadget.LeftEdge = -10-width;
    cancelgadget.Width = width;
    cancelgadget.Height = height;
    FN_FixBox(&dt0box1,&dt0box2,width,height);

    gotnamegadget.Width = width;
    gotnamegadget.Height = height;
    FN_FixBox(&dt1box1,&dt1box2,width,height);
    
    GetFileNames(drawerstr.Buffer);

    StuffReqNames();

    ZZZCursor();
    
    Request(&fn_req,w);
    
    UnZZZCursor();

    doAction = FALSE;

    ReqListen(w->UserPort,&FN_HandleEvent);
   
    FreeFileNames();

    /* restore the IDCMP to its original state */
    ModifyIDCMP(reqwindow, saveIDCMP);

    FreeShowNames();    
    UnFixMenCols();
    return( (BOOL)
	(doAction? ( okDrawer? FN_OK: FN_NoSuchDrawer): FN_Cancel ));
    }                    

