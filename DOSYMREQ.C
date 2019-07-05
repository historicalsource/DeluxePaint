/*----------------------------------------------------------------------*/
/*									*/
/*		dosymreq.c --						*/
/*									*/
/*----------------------------------------------------------------------*/

#include <system.h>
#include <prism.h>
extern Box screenBox;
extern struct Window *mainW;
extern SHORT cpWidth;
extern SHORT xShft,yShft;
extern BOOL mirror;
extern SHORT nSym;

DoSymReq() {
    BOOL mir = mirror;
    SHORT n = nSym;
    FreeTmpRas();
    UndoSave(); /* because use UpdtDisplay to restore screen */
    SymRequest(mainW, screenBox.w - cpWidth - 180, PMapY(120), &n, &mir);
    UpdtDisplay();
    SymSetNMir(n,mir);
    AllocTmpRas();
    }
