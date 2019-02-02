#include "stdafx.h"

#include "utility.hpp"


#if defined(_MSC_VER)
unsigned char bit_scan_reverse(unsigned long* index, unsigned int mask) {
	return _BitScanReverse(index, mask);
}

unsigned char bit_scan_forward(unsigned long* index, unsigned int mask) {
	return _BitScanForward(index, mask);
}
#else
unsigned char inline bit_scan_reverse(unsigned long* index, unsigned int mask) {
	if(mask) {
		*index = 31 - __builtin_clz(mask);
		return 1;
	} else {
		return 0;
	}
}

unsigned char inline bit_scan_forward(unsigned long* index, unsigned int mask) {
	if(mask) {
		*index = __builtin_ctz(mask);
		return 1;
	} else {
		return 0;
	}
}
#endif
