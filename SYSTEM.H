/*
 *  system.h, general includer for intuition
 *
 *  Confidential Information: Commodore-Amiga, Inc.
 *  Copyright (c) Commodore-Amiga, Inc.
 *
 *                      Modification History
 * date      author :      Comments
 * ------      ------      -------------------------------------
 * 1-30-85     -=RJ=-      created this file!
 *
*******************************************************************/

#include <exec\types.h>
#include <exec\nodes.h>
#include <exec\lists.h>
#include <exec\libraries.h>
#include <exec\tasks.h>
#include <exec\ports.h>

#include <exec\devices.h>

#include <exec\interrupts.h>

#include <exec\io.h>
#include <exec\memory.h>
/* #include <exec\alerts.h> */

/* ALWAYS INCLUDE GFX.H before any other amiga includes */

#include <hardware\blit.h>

#define blitNode bltnode

#include <graphics\gfx.h>

#include <graphics\regions.h>
#include <graphics\collide.h>
#include <graphics\copper.h>
#include <graphics\display.h>
#include <hardware\dmabits.h> 
#include <graphics\gels.h>

#include <graphics\clip.h>

#include <graphics\rastport.h>
#include <graphics\view.h>
#include <graphics\gfxbase.h>
/*#include <hardware\intbits.h>*/
#include <graphics\gfxmacros.h>

#include <graphics\layers.h>

#include <graphics\text.h>
#include <graphics\sprite.h>
/*#include <hardware\custom.h>*/

#include <librarie\dos.h>
/* #include <libraries\dosexten.h> */

/*???extern struct Custom Custom;*/
/*extern struct Custom custom;*/

#include <devices\timer.h>

#define TimeVal timeval

#include <devices\inputevent.h>
#include <devices\keymap.h>

/*???#define IECLASS_NULL 0*/
#include <intuitio\intuitio.h>

/*???#include "intuitionbase.h"*/
/* #include "intuinternal.h" */
