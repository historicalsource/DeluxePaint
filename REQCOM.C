/****************************************************************************/
/* reqcom.c  -- common code used by requestors	                           */
/*                                                                          */
/* Initialize the timer device                                              */
/*                                                                          */
/****************************************************************************/
#include <system.h>
#include <librarie\dos.h>

#define NO 0
#define YES 1

#define useTimer

struct timerequest timermsg = {0};

/****************************************************************************/
/* FN_Listen()                                                              */
/*                                                                          */
/****************************************************************************/

#define WAITTIME 40000

ReqListen(mailbox,handleEvent)
    struct MsgPort *mailbox;      
    BOOL (*handleEvent)();   
    {     
    struct IntuiMessage *message = NULL;
    struct IntuiMessage event;
    ULONG wakeupbit = NULL;  /* Used to see what woke us up */

    /* Set the timer so we will get a message sooner or later */
#ifdef useTimer
    InitTimer(&timermsg);
    SetTimer(&timermsg,0,WAITTIME); 
#endif
    
    /* --- This is it! LOOP getting Intuition messages --- */
    do   {
	event.Class=-1;   
	event.Code=-1;   
	event.Qualifier=-1;  
	
	/* See if we have a message */
	message = (struct IntuiMessage *)GetMsg(mailbox);
	if (message==NULL) {                          
            /* No message, wait for mailbox or timer */
#ifdef useTimer
	wakeupbit = Wait( 1<<mailbox->mp_SigBit | TimerSigBit(&timermsg));
#else
       wakeupbit = Wait( 1 << mailbox->mp_SigBit);
#endif
            if( wakeupbit & (1 << mailbox->mp_SigBit ))  {
    		/* Got some mail, get the message */
		message = (struct IntuiMessage *)GetMsg(mailbox);
		if (message != NULL)  { event=*message; ReplyMsg(message);  }
		}
#if useTimer
	    if (wakeupbit & TimerSigBit(&timermsg))  {           
		/* Time's up, set the timer again */     
		GetTimerMsg(&timermsg);
		SetTimer(&timermsg,0,WAITTIME);
		}
#endif
	    }
	else {  event=*message; ReplyMsg(message); }
	} while ( (*handleEvent)(&event) );

#ifdef useTimer
    KillTimer(&timermsg); 
#endif
    }

