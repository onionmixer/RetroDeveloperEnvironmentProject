#include "Dialogs.r"
#include "Processes.r"

/* Modal dialog: 280x120 window, centered, with one button. */
resource 'DLOG' (128) {
    { 80, 120, 200, 400 },         /* top, left, bottom, right (in global coords; centerMainScreen overrides) */
    dBoxProc,                      /* shadowed system box, no title bar */
    visible,
    noGoAway,
    0,
    128,                           /* DITL id */
    "Hello",
    centerMainScreen
};

resource 'DITL' (128) {
    {
        /* item 1: "Hello" push button, centered */
        { 75, 100, 100, 180 },
        Button { enabled, "Hello" };
    }
};

/* SIZE resource — partition / Finder hints. 100 KiB partition is plenty. */
resource 'SIZE' (-1) {
    reserved,
    acceptSuspendResumeEvents,
    reserved,
    canBackground,
    doesActivateOnFGSwitch,
    backgroundAndForeground,
    dontGetFrontClicks,
    ignoreChildDiedEvents,
    is32BitCompatible,
    notHighLevelEventAware,
    onlyLocalHLEvents,
    notStationeryAware,
    dontUseTextEditServices,
    reserved,
    reserved,
    reserved,
    100 * 1024,
    100 * 1024
};
