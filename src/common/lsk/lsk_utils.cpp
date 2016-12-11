#include "lsk_utils.h"

static u32 seed = 123456;
u32 lsk_rand()
{
	seed = seed * 2147001325 + 715136305;
	return 0x31415926 ^ ((seed >> 16) + (seed << 16));
}

f64 lsk_randf()
{
	return (lsk_rand()%1000000000)/(f64)1000000000.0;
}

void lsk_randSetSeed(u32 seed_)
{
	seed = seed_;
}
