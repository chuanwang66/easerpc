#include "seq.h"

#include <Windows.h>

static long seq = 0;

long fetch_new_sequence_no() {
	return InterlockedIncrement(&seq);
}