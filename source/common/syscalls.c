// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2011 Dave Murphy (WinterMute)
// Copyright (C) 2017 fincs

#include <nds/ndstypes.h>
#include <nds/system.h>
#include <nds/bios.h>

int swiDecompressLZSSVram(void * source, void * destination, uint32 toGetSize, TDecompressionStream * stream) {

	if (isDSiMode()) {
		return swiDecompressLZSSVramTWL(source,destination,toGetSize,stream);
	} else {
		return swiDecompressLZSSVramNTR(source,destination,toGetSize,stream);
	}
}
