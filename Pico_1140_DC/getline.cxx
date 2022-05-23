/*
MIT License

Copyright (c) 2021 Klaus Zerbe

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <cstdlib>          // need this include, if you need heap management (malloc, realloc, free)
#include <pico/stdlib.h>    // needed for Pico SDK support
#include "tusb.h"

const uint startLineLength = 8; // the linebuffer will automatically grow for longer lines
const char eof = 255;           // EOF in stdio.h -is -1, but getchar returns int 255 to avoid blocking

void serial_putchar(char c)
{
	tud_cdc_write_char(c);
    tud_cdc_write_flush();
}
char serial_getchar()
{
    while (!tud_cdc_available());
    
	return tud_cdc_read_char();
}

/*
 *  read a line of any  length from stdio (grows)
 *
 *  @param fullDuplex input will echo on entry (terminal mode) when false
 *  @param linebreak defaults to "\n", but "\r" may be needed for terminals
 *  @return entered line on heap - don't forget calling free() to get memory back
 */
char *ReadLine(bool fullDuplex = true, char lineBreak = '\n') {
    // th line buffer
    // will allocated by pico_malloc module if <cstdlib> gets included
    char * pStart = (char*)malloc(startLineLength); 
    char * pPos = pStart;  // next character position
    size_t maxLen = startLineLength; // current max buffer size
    size_t len = maxLen; // current max length
    int c;

    if(!pStart) {
        return NULL; // out of memory or dysfunctional heap
    }

    while(1) {
        c = serial_getchar(); // expect next character entry
        if(c == eof || c == lineBreak) {
            break;     // non blocking exit
        }

        if (fullDuplex) {
            serial_putchar(c); // echo for fullDuplex terminals
        }

        if(--len == 0) { // allow larger buffer
            len = maxLen;
            // double the current line buffer size
            char *pNew  = (char*)realloc(pStart, maxLen *= 2);
            if(!pNew) {
                free(pStart);
                return NULL; // out of memory abort
            }
            // fix pointer for new buffer
            pPos = pNew + (pPos - pStart);
            pStart = pNew;
        }

        // stop reading if lineBreak character entered 
        if((*pPos++ = c) == lineBreak) {
            break;
        }
    }

    *pPos = '\0';   // set string end mark
    return pStart;
}

// standalone configuration for tests
#ifdef TEST_GETLINE
int main() {
    stdio_init_all(); // needed for redirect stdin/stdout to Pico's USB or UART ports

    while(1) {
        char *pLine = getLine(true, '\r');
        if (!*pLine) {
            printf("returned empty - nothing blocked");
        } else {
            printf("entered: %s\r\n\r\n", pLine);
            free(pLine); // dont forget freeing buffer !!
        }
    }
}
#endif