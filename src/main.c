#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include "enumerator.h"

int main() {
    while(1) {
        clear_console();
        enumerate_services();
        Sleep(500);
    }
}