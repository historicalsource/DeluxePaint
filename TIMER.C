/****************************************************************************/
/* timer.c  -- implements timer 	 	                           */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
#include <system.h>
#include <librarie\dos.h>

#define NO 0
#define YES 1

/****************************************************************************/
/* InitTimer()                                                              */
/*                                                                          */
/* Initialize the timer device                                              */
/*                                                                          */
/****************************************************************************/

BOOL InitTimer(timermsg) struct timerequest *timermsg; {
    LONG error;
    struct MsgPort *timerport = NULL;    
    /* --- Create the timer message and port --- */
    timerport = (struct MsgPort *)CreatePort(0,0);
    if (timerport == NULL) return(NO);
    timermsg->tr_node.io_Message.mn_ReplyPort = timerport;
    error = OpenDevice(TIMERNAME,UNIT_VBLANK,timermsg,0);
    return ((BOOL)(error==0));
    }

/****************************************************************************/
/* SetTimer()                                                               */
/*                                                                          */
/* Set the timer so we will be awakened later on                            */
/*                                                                          */
/****************************************************************************/
SetTimer(timermsg,sec,micro) 
    struct timerequest *timermsg;ULONG sec,micro;   
    {
    timermsg->tr_node.io_Command=TR_ADDREQUEST;
    timermsg->tr_time.tv_secs=sec;
    timermsg->tr_time.tv_micro=micro;
    SendIO(timermsg);
    }

/****************************************************************************/
/* KillTimer()                                                              */
/*                                                                          */
/* Free storage from timerport				                   */
/*                                                                          */
/****************************************************************************/
KillTimer(timermsg) struct timerequest *timermsg; {
    struct MsgPort *timerport = timermsg->tr_node.io_Message.mn_ReplyPort;
    if (timerport!=NULL) DeletePort(timerport);
    timermsg->tr_node.io_Message.mn_ReplyPort = NULL;
    }

TimerSigBit(timermsg) struct timerequest *timermsg; {
    return(1 << timermsg->tr_node.io_Message.mn_ReplyPort->mp_SigBit);
    }

struct IntuiMessage *GetTimerMsg(timermsg) struct timerequest *timermsg; {
    return((struct IntuiMessage *)GetMsg(timermsg->tr_node.io_Message.mn_ReplyPort));
    }