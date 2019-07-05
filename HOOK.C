/*--------------------------------------------------------------*/
/*								*/
/*	hook.c							*/
/*								*/
/*--------------------------------------------------------------*/

#include <system.h>

typedef struct {
    struct MsgPort msgport;
    UWORD *pointer;
    } Hook;

SetHook(name,ptr) char *name; UWORD *ptr; {
    Hook *hook = (Hook *)AllocMem(sizeof(Hook),MEMF_CLEAR|MEMF_PUBLIC);
    struct MsgPort *p = (struct MsgPort *)hook;
    p->mp_Flags = 0;
    p->mp_Node.ln_Pri = 0;
    p->mp_Node.ln_Name = name;
    p->mp_Node.ln_Type = NT_MSGPORT;
    AddPort(p);
    hook->pointer = ptr;
    }

UWORD *FindHook(name) UBYTE *name; {
    Hook *hook = (Hook *)FindPort(name);
    return((hook != NULL)? hook->pointer: NULL);
    }

RemHook(name) UBYTE *name; {
    Hook *hook = (Hook *)FindPort(name);
    if (hook!=NULL) {
	RemPort(hook);
	FreeMem(hook,sizeof(Hook));
	}
    }


