/*
 * prototype_01_mac_finder — minimal Mac Toolbox application.
 *
 * Behaviour: shows a modal dialog containing a single "Hello" button;
 * clicking the button quits the program. Built for 68K System 6 / 7
 * via the Retro68 toolchain.
 */

#include <Quickdraw.h>
#include <Dialogs.h>
#include <Fonts.h>
#include <Events.h>

enum {
    kHelloDialogID = 128,
    kHelloButtonItem = 1
};

int main(void)
{
#if !TARGET_API_MAC_CARBON
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(NULL);
#endif
    InitCursor();

    DialogPtr dlg = GetNewDialog(kHelloDialogID, NULL, (WindowPtr)-1);
    if (dlg == NULL) {
        return 1;
    }

    short item;
    do {
        ModalDialog(NULL, &item);
    } while (item != kHelloButtonItem);

    DisposeDialog(dlg);
    FlushEvents(everyEvent, -1);
    return 0;
}
