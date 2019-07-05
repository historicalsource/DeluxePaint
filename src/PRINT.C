/*--------------------------------------------------------------*/
/*								*/	
/*   print.c  							*/
/*								*/	
/*--------------------------------------------------------------*/
#include <system.h>
#include <prism.h>
#include <devices\printer.h>

extern struct RastPort tempRP;
extern struct ViewPort *vport;
extern Box screenBox;
extern struct BitMap hidbm;

static union {
    struct IOStdReq ios;
    struct IODRPReq iodrp;
    struct IOPrtCmdReq iopc;  
    } printerIO; 

static struct MsgPort *replyport;

int pDumpRPort(rastPort, colorMap, modes, sx,sy, sw,sh, dc,dr, special)
    struct RastPort *rastPort;
    struct ColorMap *colorMap;
    ULONG modes;
    int sx, sy, sw, sh, dc, dr;
    UWORD special;
    {   
    int error;
    if ((error = OpenDevice("printer.device", 0, &printerIO, 0)) != 0)
	{
	dprintf("EventInit \"printer.device\" OpenDevice error: %d.\n", error);
	return(error);
	}
    replyport = (struct MsgPort *)CreatePort(0,0);
    if (replyport == NULL) return(0);
    printerIO.ios.io_Message.mn_ReplyPort = replyport;
    
    printerIO.iodrp.io_Command = PRD_DUMPRPORT;
    printerIO.iodrp.io_RastPort = rastPort;
    printerIO.iodrp.io_ColorMap = colorMap;
    printerIO.iodrp.io_Modes = modes;
    printerIO.iodrp.io_SrcX = sx;
    printerIO.iodrp.io_SrcY = sy;
    printerIO.iodrp.io_SrcWidth = sw;
    printerIO.iodrp.io_SrcHeight = sh;
    printerIO.iodrp.io_DestCols = dc;
    printerIO.iodrp.io_DestRows = dr;
    printerIO.iodrp.io_Special = special;
    DoIO(&printerIO); 
    CloseDevice(&printerIO);
    DeletePort(replyport);
    }
    
PrintPic() {
    ZZZCursor();
    UndoSave();
    tempRP.BitMap = &hidbm;
    pDumpRPort(&tempRP, vport->ColorMap, vport->Modes, 0,0, 
			screenBox.w, screenBox.h, 0,0,0);
    UnZZZCursor();
    }
