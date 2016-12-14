
#ifndef SGX_BENCHMARK_H
#define SGX_BENCHMARK_H

#include <wolfssl/wolfcrypt/types.h>

enum BenchmarkBounds {
	numBlocks = 50,  /* how many megs to test (en/de)cryption */
	ntimes = 100,
	genTimes = 100,
	agreeTimes = 100
};

static byte plain[1024 * 1024];
#endif