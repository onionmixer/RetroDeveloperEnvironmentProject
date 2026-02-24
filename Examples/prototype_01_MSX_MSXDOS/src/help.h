#ifndef HELP_H
#define HELP_H

#define HELP_DISPLAY_ROWS 23
#define HELP_LINE_WIDTH   41
#define MAX_HELP_LINES    96

typedef struct {
    char lines[MAX_HELP_LINES][HELP_LINE_WIDTH];
    unsigned char line_count;
    unsigned char scroll_offset;
} HelpScreen;

void help_init(HelpScreen *hs);
void help_scroll(HelpScreen *hs, signed char direction);
void help_render(const HelpScreen *hs);

#endif
