#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "../common.h"

extern bool gConsoleOpen;

void console_open();
void console_close();
void console_toggle();

void console_update();
void console_draw(rct_drawpixelinfo *dpi);

void console_input(int c);
void console_write(const char *src);
void console_writeline(const char *src);
void console_execute(const char *src);
void console_clear();

#endif