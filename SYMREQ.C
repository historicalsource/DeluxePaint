/** symReq.c ******************************************************************/
/*                                                                           */
/*****************************************************************************/

#include <system.h>
#include <librarie\dos.h>

extern struct RastPort screenRP;

#define NO 0
#define YES 1

#define local static

#define SYM_CYCLIC	10
#define SYM_MIRROR	11
#define SYM_CANCEL	12
#define SYM_OK		13
#define SYM_ORDER	14

#define MAX(a,b) ((a)>(b)?(a):(b))

#define REQW  160                      /* width of requester */
#define REQH  84                      /* height of requester */

#define ORDX  20
#define ORDY  40

local struct Window *reqwindow;       /* window requester is in */
local struct Requester Sym_Req= {0};        /* the requester */
local LONG bugfix = 0;	/* temp fix to the V1.07 Requester struct bug */

local UBYTE title[] = "Symmetry";
local UBYTE cyclic[] = "Cyclic";
local UBYTE mirrorStr[] = "Mirror";
local UBYTE order[] = "Order:";
local UBYTE cancel[] = "Cancel";
local UBYTE okStr[] = "Ok";

#define NOFONT NULL
#define ITInit(name)  {3,4,JAM2,0,0,NOFONT,name,NULL}

local struct IntuiText cycText = { 0,1,JAM1,3,1,NOFONT,cyclic,NULL};
local struct IntuiText mirText =  { 0,1,JAM1,3,1,NOFONT,mirrorStr,NULL};
local struct IntuiText ordText =  { 0,1,JAM1,ORDX,ORDY,NOFONT,order,NULL};
local struct IntuiText cancText =  { 0,1,JAM1,4,4,NOFONT,cancel,NULL};
local struct IntuiText okText =  { 0, 1,JAM1,4,4,NOFONT,okStr,NULL};

local struct IntuiText titleText =  {0,1,JAM1,36,3,NOFONT,title,&ordText};

local SHORT reqX = 0;                      /* leftedge of requester */
local SHORT reqY = 0;                      /* topedge of requester */

local UWORD reqpts[] = {              /* xy table of border and reqion lines */
   2,1,
   2,REQH-2,
   REQW-3,REQH-2,
   REQW-3,12,
   2,12,
   2,1,
   REQW-3,1,
   REQW-3,12
   };
        
local struct Border reqborder = {     /* border for the requester */
    0,0,
    0,1,
    JAM1,
    8,
    (SHORT *)&reqpts,
    NULL
    };

/*=======================*/
/*== CYCLIC GADGET ====3=*/
/*=======================*/
#define CYC_W  66
#define CYC_H 15   

local SHORT cycbox[] = {
   -2,-2,
   CYC_W+1,-2,
   CYC_W+1,CYC_H-3,
   -2,CYC_H-3,
   -2,-2
    };
   
local struct Border cycbor = {  0,0, 0,1, JAM1, 5, cycbox,  NULL };

local struct Gadget cyclicGadget = {
   NULL,          
   10,20,
   CYC_W,11,         
   0,		/* no highlighting */
   RELVERIFY | GADGIMMEDIATE , 
   REQGADGET | BOOLGADGET, 
   (APTR)&cycbor,         
   NULL,         
   &cycText,
   0, 	/* exclude this guy*/           
   NULL,         
   SYM_CYCLIC,    
   NULL 
   };       

/*=======================*/
/*== MIRROR GADGET ====2=*/
/*=======================*/
        
local struct Gadget mirrorGadget = {
   &cyclicGadget,          
   80,20,
   CYC_W,11,         
   0,		/* no highlighting */
   RELVERIFY | GADGIMMEDIATE,
   REQGADGET | BOOLGADGET, 
   (APTR)&cycbor,         
   NULL,         
   &mirText,
   0, 	/* exclude this guy*/           
   NULL,         
   SYM_MIRROR,    
   NULL 
   };       


/*======================*/
/*== ORDER GADGET ======*/
/*======================*/

#define ORD_WIDTH 30
#define ORD_HEIGHT 13       

local UWORD dt2box1[] = {
   -2,-2,
   ORD_WIDTH+1,-2,
   ORD_WIDTH+1,ORD_HEIGHT-3,
   -2,ORD_HEIGHT-3,
   -2,-2};

local struct Border dt2bor1 = {
   0,0,
   0,1,
   JAM1,
   5,
   (SHORT *)&dt2box1,
   NULL};

local struct StringInfo ordInfo = {
   NULL,        /* default and final string */
   NULL, 	/* optional undo buffer (later) */
   0,           /* character position in buffer */
   3,           /* max characters in buffer */
   0,           /* buffer position of first displayed character */
   0,0,0,0,0,NULL,0,NULL };     /* intuition local variables */


local struct Gadget ordGadget = {
   &mirrorGadget,
   ORDX+50, ORDY,
   ORD_WIDTH,ORD_HEIGHT,
   GADGHCOMP, 
   RELVERIFY | GADGIMMEDIATE | LONGINT, 
   REQGADGET | STRGADGET,
   (APTR) &dt2bor1,      
   NULL,                 
   NULL,                 
   0,                    
   (APTR)&ordInfo,    
   SYM_ORDER,           
   NULL };            



/*=======================*/
/*== CANCEL GADGET ====1=*/
/*=======================*/

local SHORT dt0box1[] = {0,0,0,0,0,0,0,0,0,0};
local SHORT dt0box2[] = {1,2,0,2,0,0,1,0,1,2};

/* -- Blue Box -- */
local struct Border dt0bor2 = { 0,0, 0,1, JAM1, 5, dt0box2, NULL};

/* -- Orange Box -- */
local struct Border dt0bor1 = { 0,0,  3,0, JAM1, 5, dt0box1, &dt0bor2};

local struct Gadget cancelGadget = {
  &ordGadget,           
    10,-24,
    72,15,         
    GADGHCOMP | GRELBOTTOM ,     
    RELVERIFY | GADGIMMEDIATE | ENDGADGET, 
    REQGADGET | BOOLGADGET, 
    (APTR)&dt0bor1,         
    NULL,         
    &cancText,
    0,            
    NULL,         
    SYM_CANCEL,    
    NULL 
    };       

/*=======================*/
/*==== OK GADGET =====0==*/
/*=======================*/

local SHORT dt1box1[] = {0,0,0,0,0,0,0,0,0,0};
local SHORT dt1box2[] = {1,2,0,2,0,0,1,0,1,2};

/* Blue Box */   
local struct Border dt1bor2 = {  0,0, 0,1, JAM1, 5, dt1box2, NULL};

/* Orange Box */
local struct Border dt1bor1 = {  0,0, 3,0, JAM1, 5, dt1box1, &dt1bor2};

local struct Gadget okGadget = {
    &cancelGadget,          
    -40,-24,
    30,15,         
    GADGHCOMP | GRELBOTTOM | GRELRIGHT,     
    RELVERIFY | GADGIMMEDIATE | ENDGADGET,
    REQGADGET | BOOLGADGET, 
    (APTR)&dt1bor1,         
    NULL,         
    &okText,
    0,            
    NULL,         
    SYM_OK,    
    NULL
    };       


/*****************************************************************************/
/* SYM_FixBox                                                                 */
/*                                                                           */
/* Adjust a box's corners to a specified width and height                    */
/*                                                                           */
/*****************************************************************************/

local VOID SYM_FixBox(box1,box2,width,height)
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

local SymInitReq()   {
    InitRequester(&Sym_Req);
    Sym_Req.LeftEdge = reqX;
    Sym_Req.TopEdge = reqY;
    Sym_Req.Width = REQW;
    Sym_Req.Height = REQH;
    Sym_Req.ReqGadget = &okGadget;
    Sym_Req.ReqBorder = &reqborder;
    Sym_Req.ReqText = &titleText;   
    Sym_Req.BackFill = 1;
    SYM_FixBox(&dt0box1,&dt0box2,strlen(cancel)*10+10,15);
    SYM_FixBox(&dt1box1,&dt1box2,strlen(okStr)*10+10,15);
    }

local BOOL SymHandleEvent();       /* forward declaration */

local BOOL doAction;
local BOOL *symMirror= NULL;
local SHORT *symN = NULL;
local UBYTE ordUndo[5];
local  UBYTE ordStr[5];

local BOOL mirrorSet;

local InvGadg(req,gad)
    struct Requester *req;
    struct Gadget *gad;
    {
    int x,y;          
    SHORT svpen;
    x = req->LeftEdge + gad->LeftEdge,
    y = req->TopEdge + gad->TopEdge;
    svpen = screenRP.FgPen;
    SetAPen(&screenRP,1);
    SetXorFgPen(&screenRP);
    RectFill(&screenRP, x, y, x + gad->Width-1, y + gad->Height-1);
    SetDrMd(&screenRP,JAM2);
    SetAPen(&screenRP,svpen);
    }

local InvOne(){ if (mirrorSet) InvGadg(&Sym_Req,&mirrorGadget);
     else InvGadg(&Sym_Req,&cyclicGadget); }
    
local SetMirror(ms) BOOL ms; { 
    InvOne(); mirrorSet = ms; InvOne(); 	}

/*****************************************************************************/
/* SymRequest                                                                */
/*                                                                           */
/* Initialize and display the requester                                      */
/*****************************************************************************/
BOOL SymRequest(w,x,y, nsym, isMirror) struct Window *w; SHORT x,y;
    SHORT *nsym; BOOL *isMirror;
    {
    ULONG saveIDCMP;
    FixMenCols();
    reqX = x;
    reqY = y;
    symN = nsym;
    symMirror = isMirror;
    reqwindow = w;
    saveIDCMP = w->IDCMPFlags;
    
    /* --- What class of events do we want to know about? --- */
    ModifyIDCMP(reqwindow,  GADGETUP | GADGETDOWN);
    SymInitReq();
    
    stcu_d(ordStr, *nsym, 3); 
    ordInfo.Buffer = ordStr;
    ordInfo.UndoBuffer = ordUndo;
    ordInfo.LongInt = *nsym;
    
    Request(&Sym_Req,w);

    mirrorSet = *isMirror;
    InvOne();

    doAction = NO;

    ReqListen(w->UserPort,&SymHandleEvent);
    
    /* restore the IDCMP to its original state */

    ModifyIDCMP(reqwindow, saveIDCMP);
    
    UnFixMenCols();
    return(doAction);
    }                    

/*****************************************************************************/
/* Handle events for the this requester  	                             */
/*****************************************************************************/

/* #define SYMDBG */

local BOOL SymHandleEvent(event) struct IntuiMessage *event; {                                
    WORD gadgetid;
    BOOL keepGoing = YES;
#ifdef SYMDBG
    printf("SymHandleEvent, event = %lx \n",event);
#endif
    if (event->Class== GADGETUP) {
#ifdef SYMDBG
	printf(" SymHandlEvent: GADGETUP \n");
#endif
	
	gadgetid = ((struct Gadget *)event->IAddress)->GadgetID;
	if (gadgetid == SYM_CANCEL) keepGoing = NO;
	else if (gadgetid==SYM_OK) { 
	    *symMirror = mirrorSet; 
	    *symN = ordInfo.LongInt;
	    doAction = YES;  keepGoing = NO; 
	    }
	else if (gadgetid== SYM_CYCLIC) { SetMirror(NO); }
	else if (gadgetid== SYM_MIRROR) { SetMirror(YES); }
	}
    return(keepGoing);
    }



