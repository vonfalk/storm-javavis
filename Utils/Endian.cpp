#include "stdafx.h"
#include "Endian.h"

int16 byteSwap(int16 v) {
	int16 r = (v & 0xFF00) >> 8;
	r |= (v & 0x00FF) << 8;
	return r;
}

nat16 byteSwap(nat16 v) {
	nat16 r = (v & 0xFF00) >> 8;
	r |= (v & 0x00FF) << 8;
	return r;
}

int byteSwap(int v) {
	int r = (v & 0xFF000000) >> 24;
	r |= (v & 0x00FF0000) >> 8;
	r |= (v & 0x0000FF00) << 8;
	r |= (v & 0x000000FF) << 24;
	return r;
}

nat byteSwap(nat v) {
	nat r = (v & 0xFF000000) >> 24;
	r |= (v & 0x00FF0000) >> 8;
	r |= (v & 0x0000FF00) << 8;
	r |= (v & 0x000000FF) << 24;
	return r;
}
