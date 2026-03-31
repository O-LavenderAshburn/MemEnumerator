#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>
#include "enumerator.h"


void hide_cursor() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 1;
    info.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &info);
}

int main() {
    int scrollOffset = 0;
    hide_cursor();
    while(1) {
        if (_kbhit()) {
            int key = _getch();
            if (key == 72) scrollOffset--;  // up arrow
            if (key == 80) scrollOffset++;  // down arrow
            if (scrollOffset < 0) scrollOffset = 0;
        }

        clear_console();
        enumerate_processes(scrollOffset);
        Sleep(500);
    }
}