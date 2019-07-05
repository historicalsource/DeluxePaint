/*----------------------------------------------------------------------*/
/*									*/
/*		MouseBut.c						*/
/*									*/
/*----------------------------------------------------------------------*/

#include <exec\types.h>
#include <hardware\cia.h>
#include <hardware\custom.h>

extern struct Custom custom;
extern struct CIA ciaa;

SHORT MouseBut() {
    register SHORT v = 0;
    if ( (ciaa.ciapra & 0x40) == 0) v |= 1;
    if ( (custom.potinp & 0x400) == 0) v |= 2;
    return(v);
    }
    
