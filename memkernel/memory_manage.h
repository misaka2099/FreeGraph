#pragma once 
//per bit -> Init vector len
#include "data_type.h"
void set(int start, int len, BYTE* bitmap);
void reset(int start, int len, BYTE* bitmap);
bool is_free(int start, int len, int &first_set , BYTE* bitmap);
void allocate(int &start, int len, BYTE* bitmap);
void deallocate(int start, int len, BYTE* bitmap);