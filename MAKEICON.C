/****************************************************************************/
/*                                                                          */
/*  FILENAME: makeicon.c                                                     */
/*                                                                          */
/*  FUNCTION: Project Icon Routines                                         */
/*                                                                          */
/*  REVISION: tomc                                                          */
/*  REVISION: dans -- modified for DPaint -- 11/16/85                       */
/*                                                                          */
/****************************************************************************/
#include <system.h>

/****************************************************************************/
/*                                                                          */
/* This stuff modified from "workbenc.h" because of compiling problems such */
/* as enum and bit fields.                                                  */
/*                                                                          */
/****************************************************************************/
#define WB_DISKMAGIC       0xE310 
#define WB_DISKVERSION     1
#define WBPROJECT          0x0400
#define NO_ICON_POSITION   (0x80000000)
#define GADGBACKFILL      0x0001

struct DiskObject
   {
   UWORD do_Magic;            /* a magic number at the start of the file */
   UWORD do_Version;          /* a version number, so we can change it */
   struct Gadget do_Gadget;   /* a copy of in core gadget */
   UWORD do_Type;             /* enum WBObjectType */ 
   char *do_DefaultTool;
   char **do_ToolTypes;
   LONG do_CurrentX;
   LONG do_CurrentY;
   APTR do_DrawerData;        /* struct DrawerData * */
   char *do_ToolWindow;       /* only applies to tools */
   LONG do_StackSize;         /* only applies to tools */
   };

/****************************************************************************/
/*                                                                          */
/* All the data needed for ProjectIcon                                      */
/*                                                                          */
/****************************************************************************/
#define PROJECT_WIDTH  58
#define PROJECT_HEIGHT 20
#define PROJECT_DEPTH  2

/* Bitmap name = picicon,  Amiga-BOB format.   */
/* Width = 58, Height = 20 */ 

short picicon0[80] = { 
  0x0,   0x0,   0x0,   0x0, 
  0x7FFF,   0xFFFF,   0xFFFF,   0xFF80, 
  0x6000,   0x0,   0x0,   0x180, 
  0x6000,   0x7FF,   0xFFFF,   0x8180, 
  0x6000,   0x401F,   0xFFC0,   0x180, 
  0x6002,   0xE000,   0x0,   0x180, 
  0x6003,   0x6002,   0x8,   0x180, 
  0x600B,   0xF807,   0x5E,   0x180, 
  0x6001,   0xFC03,   0x809F,   0x8180, 
  0x6000,   0xE07,   0xC2FF,   0xE180, 
  0x600D,   0xAF05,   0xE017,   0xF980, 
  0x6021,   0x2F83,   0xC057,   0xF980, 
  0x6025,   0xFF1F,   0x24C7,   0xF980, 
  0x6097,   0xFA6E,   0x881F,   0xF980, 
  0x601E,   0xFCEC,   0x3B5B,   0xF980, 
  0x6055,   0xFFBA,   0x559F,   0xF980, 
  0x60EF,   0xFFFF,   0xFFFF,   0xF980, 
  0x6000,   0x0,   0x0,   0x180, 
  0x7FFF,   0xFFFF,   0xFFFF,   0xFF80, 
  0x0,   0x0,   0x0,   0x0
  }; 
short picicon1[80] = { 
  0xFFFF,   0xFFFF,   0xFFFF,   0xFFC0, 
  0x8000,   0x0,   0x0,   0x40, 
  0x9FFF,   0xFFFF,   0xFFFF,   0xFE40, 
  0x9800,   0x0,   0x0,   0x640, 
  0x9800,   0x0,   0x0,   0x640, 
  0x9801,   0x0,   0x0,   0x640, 
  0x9804,   0x8004,   0x0,   0x640, 
  0x9804,   0x8008,   0x20,   0x640, 
  0x981F,   0xC0C,   0x160,   0x640, 
  0x983F,   0xFE1F,   0x8D00,   0x640, 
  0x98FF,   0xFF7F,   0xDFE8,   0x640, 
  0x9BFF,   0xFFFF,   0xFFFF,   0x640, 
  0x9FFF,   0xFFFF,   0xFFFF,   0x8640, 
  0x9FFF,   0xFFFF,   0xFFFF,   0xFE40, 
  0x9FFF,   0xFFFF,   0xFFFF,   0xFE40, 
  0x9FFF,   0xFFFF,   0xFFFF,   0xFE40, 
  0x9FFF,   0xFFFF,   0xFFFF,   0xFE40, 
  0x9FFF,   0xFFFF,   0xFFFF,   0xFE40, 
  0x8000,   0x0,   0x0,   0x40, 
  0xFFFF,   0xFFFF,   0xFFFF,   0xFFC0
  }; 




static struct Image projectImage=
   {
   0,0,
   PROJECT_WIDTH,PROJECT_HEIGHT,
   PROJECT_DEPTH,
   (USHORT *)&picicon0[0],
   3,0,
   NULL
   };

static struct Gadget projectGadget=
   {
   NULL,
   0,0,
   PROJECT_WIDTH,PROJECT_HEIGHT,
   GADGIMAGE | GADGHCOMP,
   RELVERIFY | GADGIMMEDIATE,
   BOOLGADGET,
   (APTR)&projectImage,
   NULL,
   NULL,
   0,
   NULL,
   0,
   NULL
   };


/****************************************************************************/
/*                                                                          */
/* All the data needed for BrushIcon                                        */
/*                                                                          */
/****************************************************************************/
#define BRUSH_WIDTH  43
#define BRUSH_HEIGHT 8
#define BRUSH_DEPTH  2

/* Bitmap name = brushico,  Amiga-BOB format.   */
/* Width = 43, Height = 8 */ 

short brushico0[24] = { 
  0x1FF0,   0x0,   0x0, 
  0x7FF8,   0x0,   0x0, 
  0xFFFB,   0xFFFF,   0xFF80, 
  0xFFFB,   0xFFFF,   0xFF80, 
  0xFFF8,   0x0,   0x0, 
  0xFFF0,   0x0,   0x0, 
  0xE000,   0x0,   0x0, 
  0x8000,   0x0,   0x0
  }; 
short brushico1[24] = { 
  0x1FFC,   0x0,   0x0, 
  0x7FFE,   0x0,   0x0, 
  0xFFFC,   0x0,   0x60, 
  0xFFFC,   0x0,   0x60, 
  0xFFFF,   0xFFFF,   0xFFE0, 
  0xFFFC,   0x0,   0x0, 
  0xFFF8,   0x0,   0x0, 
  0xE000,   0x0,   0x0
  }; 


static struct Image brushImage=
   {
   0,0,
   BRUSH_WIDTH,BRUSH_HEIGHT,
   BRUSH_DEPTH,
   (USHORT *)&brushico0[0],
   3,0,
   NULL
   };

static struct Gadget brushGadget=
   {
   NULL,
   0,0,
   BRUSH_WIDTH,BRUSH_HEIGHT,
   GADGIMAGE | GADGHCOMP,
   RELVERIFY | GADGIMMEDIATE,
   BOOLGADGET,
   (APTR)&brushImage,
   NULL,
   NULL,
   0,
   NULL,
   0,
   NULL
   };

#define PICTURE 0
#define BRUSH 1
/****************************************************************************/
/*                                                                          */
/* WriteIcon()			                                            */
/*                                                                          */
/****************************************************************************/
static BOOL WriteIcon(drawer,name,type)
    UBYTE *drawer,*name;    SHORT type;
    {
    BOOL success=TRUE;
    LONG newlock,oldlock;

    struct DiskObject myDiskObject;

    myDiskObject.do_Magic=WB_DISKMAGIC;
    myDiskObject.do_Version=WB_DISKVERSION;
    myDiskObject.do_Type=WBPROJECT;
    myDiskObject.do_ToolTypes=NULL;
    myDiskObject.do_CurrentX=NO_ICON_POSITION;
    myDiskObject.do_CurrentY=NO_ICON_POSITION;
    myDiskObject.do_DrawerData=NULL;
    myDiskObject.do_ToolWindow=NULL;
    myDiskObject.do_StackSize=NULL;
    
    if (type==PICTURE) {
	myDiskObject.do_Gadget=projectGadget;
	myDiskObject.do_DefaultTool=":dpaintx";
	}
    else {
	myDiskObject.do_Gadget=brushGadget;
	myDiskObject.do_DefaultTool="";
	}
    newlock=Lock(drawer,SHARED_LOCK);
    success=(newlock!=NULL);
    if (success)    {
	oldlock = CurrentDir(newlock);
	success=(BOOL)PutDiskObject(name,&myDiskObject);
	newlock = CurrentDir(oldlock);
	}
    return(success);
    }

BOOL WriteProjectIcon(drawer,name)    UBYTE *drawer,*name;
    {   return (WriteIcon(drawer,name,PICTURE));   }

BOOL WriteBrushIcon(drawer,name)    UBYTE *drawer,*name;
    {   return (WriteIcon(drawer,name,BRUSH));   }
