#include <stdio.h>

#include "util.h"

int read_file(const char* filepath, char* ret_buf, u32 max_buffer_size)
{
    FILE* fp = fopen(filepath,"r");
    
    if(!fp)
        return -1;

    int c;
    int i = 0;

    for(;;)
    {
        c = fgetc(fp);
        if(c == EOF)
            break;

        ret_buf[i++] = c;

        if(i >= max_buffer_size)
            break;
    }
    return i;
}
