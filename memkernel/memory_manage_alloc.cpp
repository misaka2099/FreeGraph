#include "memory_manage.h"
void set(int start, int len, BYTE* bitmap)
{
    for (int i = start; i < start + len; i++)
    {
        bitmap[i / 8] |= ((BYTE)1 << (i % 8));
    }
}


bool is_free(int start, int len, int &first_set , BYTE* bitmap)
{
    for (int i = start; i < start + len; i++)
    {
        if (bitmap[i / 8] & (1 << (i % 8)))
        {
            first_set = i;
            return false;
        }
    }
    return true;
} 
void allocate(int &start, int len, BYTE* bitmap)
{
    start = -1;
    if (len == 0) return;
    int first_set = -1;
    for (int i = 0 ; i < CHANNEL_CAPACITY * CHANNEL_NUM ; i++)
    {
        if (is_free(i, len, first_set, bitmap))
        {
            start = i;
            set(start, len, bitmap);
            return;
        }
        else
        {
            i = first_set;
        }
    }
}