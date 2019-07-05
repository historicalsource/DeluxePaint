/*--------------------------------------------------------------*/
/*								*/
/*        Pane.c  -- Pane manager	 			*/
/*								*/
/*--------------------------------------------------------------*/

#include <system.h>
#include <prism.h>
#include <librarie\dos.h>

#define local static

extern Box mainBox;
extern short xShft,yShft;
extern struct RastPort *mainRP;
extern Box screenBox;
extern SHORT checkpat[];
extern struct Window *mainW;
extern struct ViewPort *vport;
extern SHORT nColors;
extern BOOL skipRefresh,updatePending;
		    
BOOL inMenu = NO;
		    
Pane *paneList = NULL;
Pane *actPane = NULL, *newPane = NULL;
BOOL stopFlag = FALSE;

local SHORT but=0,lastBut=0,msx=0,msy=0;

PaneInit() { paneList = NULL; }

/* #define DBGPANE   */

/*===============================================================*/
#ifdef frogofog
PaneClear(p,col) Pane *p; SHORT col; {
    PushGrDest(mainRP,&screenBox); 
    PFillBox(&p->box);
    PopGrDest();
    }

/*** The frame in PC Prism was OUTSIDE the pane's box **/
void PaneUnDisplay(ph) Pane *ph; {
    Box b;
    b = ph->box;
    if (ph->flags&BORDER) { b.x--; b.y--; b.w +=2; b.h += 2; }
    if (!PaneClearing) return;
    PushGrDest(mainRP,&screenBox);
    PFillBox(&b);
    PopGrDest();
    }

PaneFrame(ph) Pane *ph; {
    Box b;
    b = ph->box;
    b.x--; b.y--; b.w +=2; b.h += 2; 
    PushGrDest(mainRP,&screenBox); 
    SetFGCol(0);
    PThinFrame(&b);
    PopGrDest();
    }
#endif

/*===============================================================*/

PaneInstall(ph) Pane *ph; {
    PaneDelete(ph);
    ph->next = paneList;
    paneList = ph;
    }

PaneSize(ph,x,y,w,h) Pane *ph; SHORT x,y,w,h; {
    ph->box.x = x;   ph->box.y = y;
    ph->box.w = w;   ph->box.h = h;
    }

BOOL PaneClearing = NO;
PaneSetClear(b) BOOL b; { PaneClearing = b; }
local PaneDelete(ph) Pane *ph; {
    Pane *p;
    Pane *prv = NULL;
    for (p=paneList; (p!=NULL)&&(p!=ph); p= p->next) prv=p; 
    if (p!=NULL) if (prv) prv->next = p->next; else paneList = p->next;
    if (ph==actPane) {
	actPane = NULL;
	while(MouseBut()) ;
	}
    }
	    
PaneRemove(ph) Pane *ph; {   PaneDelete(ph);  }

Pane *PaneFindPlace(x,y) SHORT x,y; {
    Pane *p;
    for (p=paneList; p!=NULL; p = p->next) if (BoxContains(&p->box,x,y)) return(p); 
    return(NULL);
    }

PaneStop() { stopFlag = TRUE; }

PaneRefresh(bx) Box *bx; {
    Pane *p; Box c;
    for(p=paneList; p!=NULL; p=p->next)
        if (BoxAnd(&c,bx,&p->box)) PanePaint(p,&c);
    }

local PanePaintList(p) Pane *p; {
    if (p->next!=NULL) PanePaintList(p->next);
    PanePaint(p,&screenBox);
    }
    
PaneRefrAll() { 
    if (paneList!=NULL) PanePaintList(paneList); /* paints in bottom up order */
    }

PanePaint(ph,bx) Pane *ph; Box *bx; {
    if (ph->paintProc!=NULL) (*(ph->paintProc))(ph,bx);
    }

/* note: box is the extent of the active area: the border, if 
   any will lie outside of this */    
PaneCreate(ph,box,flags,cProc,mProc,pProc,context)
    Pane *ph;
    Box *box; 		/* extent of Pane on screen */
    SHORT flags;     
    ProcHandle cProc;	/* pointer to proc phich gets typing action*/
    ProcHandle mProc;	/* pointer to proc phich gets mouse action*/
    ProcHandle pProc;	/* pointer to proc to repaint Pane contents*/
    ULONG context;	/* client data */
    {
    ph->box = *box;
    ph->flags = flags;
    ph->charProc = cProc;
    ph->mouseProc = mProc;
    ph->paintProc = pProc;
    ph->client = context;
    ph->next = NULL;
    }

BOOL fakeMv = NO;
BOOL active = YES;
	    	    
PaneReset() { actPane = NULL; }
PaneFakeMove() { fakeMv=YES; }
    
CursInst() {}

SHORT dbgpane = NO;

struct IntuiMessage *pushBMsg = NULL;

struct IntuiMessage *GetPushBMsg() {
    return((pushBMsg!=NULL)? pushBMsg: 
        (struct IntuiMessage *)GetMsg(mainW->UserPort));
    }

LONG CheckForKey() {
    struct IntuiMessage *message;
    LONG class,code;
    message = GetPushBMsg();
    if (message==NULL) return(-1);
    class = message->Class; 
    code = message->Code;
    pushBMsg = message;
    return((class==RAWKEY)? code: -1);
    }

LONG kbhit() { return(CheckForKey() != -1); }

/***
ClearPushB() {
    if (pushBMsg!=NULL) ReplyMsg(pushBMsg);
    pushBMsg = NULL; 
    }
***/

AbortKeyHit() {
    LONG code = CheckForKey();
    int res = 0;
    if (code != -1) if (CharFromKeyCode(code) == ' ') {
	res = TRUE;
	};
    return(res);
    }

SHORT getch() {
    struct IntuiMessage *message;
    SHORT c = NoChar;
    message = GetPushBMsg();
    if (message) {
	if (pushBMsg->Class==RAWKEY) 
	    c = CharFromKeyCode(pushBMsg->Code);
	    else c = NoChar;
	ReplyMsg(pushBMsg);
	}
    pushBMsg = NULL;
    return(c);
    }
    
BOOL WaitRefrMessage() {
    struct IntuiMessage *message;
    LONG class;
    for (;;) {
	Wait(1 << mainW->UserPort->mp_SigBit);
	message = (struct IntuiMessage *)GetMsg(mainW->UserPort);
	if (message!=NULL) {
	    class = message->Class;
	    if (class==REFRESHWINDOW)  {
		ReplyMsg(message);
		return(YES);
		}
	    pushBMsg = message;
	    return(NO); 
	    }
	}
    }

MakeButtonBeUp() { lastBut = but = 0; }

CallMouseProc(ev,but) MouseEvent ev; SHORT but; {
    if (!inMenu)
	(*actPane->mouseProc) (actPane,ev,but,msx-actPane->box.x, msy-actPane->box.y);
    }
    
ChgPane(new)  Pane *new; {
    if (actPane) CallMouseProc(LEAVE,but);
    if (new==NULL)  CursInst(Arrow); 
    actPane = new;
    if (actPane) {
#ifdef DBGPANE
	dprintf("ChgPane: new pane =  %lx\n", new);
#endif
	CallMouseProc(ENTER,but); 
	}
    }

#define doTimer  

#ifdef doTimer		
static struct timerequest timermsg = {0};
#endif
		
#define WAITTIME 6000		/* 6 milliseconds */

/* Check for change in current pane based on (msx,msy) **/
/* returns TRUE if changed */
BOOL ChkChgPane() {
    Pane *new;
    if (inMenu) return(FALSE);
    if (actPane==NULL) {
	new = PaneFindPlace(msx,msy);
	if (new == NULL) return(FALSE);
	}
    else if ( (BoxContains(&actPane->box,msx,msy))||
		    (lastBut&&actPane->flags&HANGON)) return(FALSE);
	else new = PaneFindPlace(msx,msy); 
    ChgPane(new);
    return(TRUE);
    }
		
PListen() {
    SHORT lastx=0,lasty=0;
    struct IntuiMessage *message;
    struct MsgPort *mp = mainW->UserPort;


#ifdef doTimer
    ULONG wakeup;
    InitTimer(&timermsg);
    SetTimer(&timermsg, 0, WAITTIME);
#endif

    but = lastBut = 0;
    stopFlag = FALSE;

    for (;;) {
	if (stopFlag) {
	    if (HookActive()){
		InfoMessage("Can't quit--","bitmap in use.");
		stopFlag = NO;
		}
	    else break;
	    }
	
	/* --- Read cursor coordinates out of Window structure ---*/
	Forbid(); 
	msx = mainW->MouseX + mainW->LeftEdge; 
	msy = mainW->MouseY + mainW->TopEdge;
	Permit(); 

	/*---- see if we can make right button do the correct thing */
	if (msy < mainBox.y) mainW->Flags &= (~RMBTRAP);
	    else mainW->Flags |= RMBTRAP;
	    
	/* (1) ----- Check message port */
	message = GetPushBMsg();
	pushBMsg = NULL;
	if (message != NULL) { 
	    DoMessage(message); goto finish; 
	    }
	
	/* (2) ----- Check for entering or leaving Panes */
	if (ChkChgPane()) goto finish;
	if (!inMenu) {
	    if (actPane != NULL) {
		/* (3) ----- Check for mouse motion */
		if ((msx!=lastx)||(msy!=lasty)||fakeMv) {
		    CallMouseProc(MOVED,but); goto finish;
		    }
		/* (4) ----- Check for "always" flag */
		if (actPane->flags&ALWAYS) {
		    CallMouseProc(LOOPED,but); goto finish;
		    }
		}
	    }
#ifdef doTimer
	/* (5) ---- Nothing happened: so wait a while */
	wakeup = Wait( (1 << mp->mp_SigBit) | TimerSigBit(&timermsg));
	if (wakeup&mp->mp_SigBit) {
	    message = (struct IntuiMessage *)GetMsg(mainW->UserPort);
	    if (message != NULL) DoMessage(message);
	    }
	if (wakeup&TimerSigBit(&timermsg)) {
	    GetTimerMsg(&timermsg); /* no reply ??? */
	    SetTimer(&timermsg, 0,WAITTIME);
	    }
#endif
	finish:
	    lastBut = but;
	    lastx = msx; lasty = msy;
	} /* End of main loop */
#ifdef doTimer
    KillTimer(&timermsg);
#endif
    }

DoMessage(message)    struct IntuiMessage *message; {
    MouseEvent event;
    LONG class,code,qualifier,ch;
    event = NONE;
    but = lastBut;
    class = message->Class;
    code = message->Code;
    qualifier = message->Qualifier;
    msx = message->MouseX + mainW->LeftEdge; 
    msy = message->MouseY + mainW->TopEdge;
    ChkChgPane(); /* dont want event going to wrong window */
    
#ifdef DBGPANE
    dprintf("Msg: class=%6lx, code=%4lx, qual=%4lx, x=%ld, y=%ld\n",
	class,code,qualifier,msx,msy);
#endif

    switch(class)   {
	case MENUVERIFY:
	    inMenu = YES;
#ifdef DBGPANE
	    dprintf(" MENUVERIFY message \n");
#endif
	    FixMenCols();
	    break;
	case RAWKEY:
	    ch = CharFromKeyCode(code);
	    if ((ch!=NoChar)&&actPane&&(!inMenu))
	    (*actPane->charProc)(actPane, ch);
	    break;
	case MOUSEBUTTONS:
	    skipRefresh = NO; /* just in case it got left on*/
	
	/* disallow 2 buttons down at once. if one button 
	    goes down, ignore other until first goes up
	    but = , 1, or 2, never 3 */
	    switch (code) {
		case SELECTDOWN: if (but==0)
			{ lastBut = but= 1; event = BDOWN;} break;
		case SELECTUP: if (but == 1) {
			but = 0; event = BUP;} break;
		case MENUDOWN: if (but==0) 
			{ lastBut = but = 2; event = BDOWN;} break;
		case MENUUP: if (but == 2) {
				but = 0; event = BUP;} break;
		}
	    if ((event!=NONE)&&actPane) CallMouseProc(event,lastBut);
	    break;
	case MENUPICK:
	    inMenu = NO;
	    UnFixMenCols();
	    skipRefresh = YES;
	    DoMenEvent(code); 
	    break;
	case REFRESHWINDOW:
#ifdef DBGPANE
	    dprintf(" REFRESHWINDOW \n"); 
#endif
	    if ((!skipRefresh)&&(!updatePending)){
/*
		ClearFB();
		PaneRefrAll();
*/
		}
	    skipRefresh = NO;
	    break;
	case ACTIVEWINDOW:
	    active = YES;
	    if (SharingBitMap()) UpdtDisplay(); /* In case process is sharing bitmap*/
	    break;
	case INACTIVEWINDOW:
	    if (SharingBitMap()) UndoSave(); /* In case process is sharing bitmap*/
	    active = NO;
	    break;
	case CLOSEWINDOW: stopFlag = FALSE; break;
	}
    ReplyMsg(message);
    /* done processing IDCMP message */
    }
    
