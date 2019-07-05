/*----------------------------------------------------------------------*/
/*  									*/
/*       message.c -- Error and Informative messages			*/
/*  									*/
/*----------------------------------------------------------------------*/

#include <system.h>
extern struct Window *mainW;

#define local static
local struct BitMap sparebm = {0};
local struct IntuiText itInfo2 = {0,0,JAM1,10,20,NULL,NULL,NULL};
local struct IntuiText itInfo1 = {0,0,JAM1,10,10,NULL,NULL,&itInfo2};

local UBYTE proceed[] = "Proceed";
local struct IntuiText itProceed = {0,0,JAM1,4,4,NULL,proceed,NULL};

#define idcmpFlags  MOUSEBUTTONS | GADGETUP | MOUSEMOVE

InfoMessage(str1,str2) UBYTE *str1, *str2;{
    itInfo1.IText = str1;
    itInfo2.IText = str2;
    UndoSave();
    AutoRequest(mainW, &itInfo1, NULL, &itProceed, 
	idcmpFlags, idcmpFlags,	240,70);
    UpdtDisplay();
    }
    
