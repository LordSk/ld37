#include "lsk_utils.h"

static u32 state = 123456;
u32 lsk_rand()
{
	state = state * 2147001325 + 715136305;
	return 0x31415926 ^ ((state >> 16) + (state << 16));
}

f64 lsk_randf()
{
	return (lsk_rand()%1000000000)/(f64)1000000000.0;
}
