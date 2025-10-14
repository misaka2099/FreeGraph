#include "memory_manage.h"


void deallocate(int start, int len, BYTE* bitmap)
{
    if (len == 0) return;
    reset(start, len, bitmap);
}
void reset(int start, int len, BYTE* bitmap)
{
    for (int i = start; i < start + len; i++)
    {
        bitmap[i / 8] &= ~((BYTE)1 << (i % 8));
    }
}