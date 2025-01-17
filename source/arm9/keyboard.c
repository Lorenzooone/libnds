// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2007 Jason Rogers (dovoto)

// stdin integration for a simple keyboard

#include "keyboardGfx.h"
#include <nds/ndstypes.h>
#include <nds/interrupts.h>
#include <nds/arm9/keyboard.h>
#include <nds/arm9/input.h>
#include <nds/arm9/decompress.h>
#include <nds/arm9/background.h>
#include <string.h>
#include <stdio.h>

s16 lastKey = -1;

//default keyboard map
const s16 SimpleKbdLower[] = {

	DVK_FOLD, DVK_FOLD, NOKEY, '1', '1', '2', '2', '3', '3', '4', '4', '5', '5', '6', '6', '7',
	'7', '8', '8', '9', '9', '0', '0', '-', '-', '=', '=', DVK_BACKSPACE, DVK_BACKSPACE, DVK_BACKSPACE, DVK_BACKSPACE, DVK_BACKSPACE ,

	DVK_TAB, DVK_TAB, DVK_TAB, DVK_TAB, 'q', 'q', 'w', 'w', 'e', 'e', 'r', 'r', 't', 't', 'y', 'y',
	'u', 'u', 'i', 'i', 'o', 'o', 'p', 'p', '[', '[', ']', ']', '\\', '\\', '`', '`' ,

	DVK_CAPS, DVK_CAPS, DVK_CAPS, DVK_CAPS, DVK_CAPS, 'a', 'a', 's', 's', 'd', 'd', 'f', 'f', 'g', 'g', 'h',
	'h', 'j', 'j', 'k', 'k', 'l', 'l', ';', ';', '\'', '\'', DVK_ENTER, DVK_ENTER, DVK_ENTER, DVK_ENTER, DVK_ENTER ,

	DVK_SHIFT, DVK_SHIFT, DVK_SHIFT, DVK_SHIFT, DVK_SHIFT, DVK_SHIFT, 'z', 'z', 'x', 'x', 'c', 'c', 'v', 'v', 'b', 'b',
	'n', 'n', 'm', 'm', ',', ',', '.', '.', '/', '/', NOKEY, NOKEY, DVK_UP, DVK_UP, NOKEY, NOKEY ,

	DVK_CTRL,	DVK_CTRL,  DVK_CTRL,  DVK_CTRL,  DVK_CTRL,  DVK_ALT,   DVK_ALT,   DVK_ALT,
	DVK_ALT,	DVK_SPACE, DVK_SPACE, DVK_SPACE, DVK_SPACE, DVK_SPACE, DVK_SPACE, DVK_SPACE,
	DVK_SPACE,	DVK_SPACE, DVK_SPACE, DVK_SPACE, DVK_SPACE, DVK_MENU,  DVK_MENU,  DVK_MENU,
	DVK_MENU,	DVK_MENU,  DVK_LEFT,  DVK_LEFT,  DVK_DOWN,  DVK_DOWN,  DVK_RIGHT, DVK_RIGHT

};

const s16 SimpleKbdUpper[] = {

	DVK_FOLD, DVK_FOLD, NOKEY, '!', '!', '@', '@', '#', '#', '$', '$', '%', '%', '^', '^', '&',
	'&', '*', '*', '(', '(', ')', ')', '_', '_', '+', '+', DVK_BACKSPACE, DVK_BACKSPACE, DVK_BACKSPACE, DVK_BACKSPACE, DVK_BACKSPACE ,

	DVK_TAB, DVK_TAB, DVK_TAB, DVK_TAB, 'Q', 'Q', 'W', 'W', 'E', 'E', 'R', 'R', 'T', 'T', 'Y', 'Y',
	'U', 'U', 'I', 'I', 'O', 'O', 'P', 'P', '{', '{', '}', '}', '|', '|', '~', '~' ,

	DVK_CAPS, DVK_CAPS, DVK_CAPS, DVK_CAPS, DVK_CAPS, 'A', 'A', 'S', 'S', 'D', 'D', 'F', 'F', 'G', 'G', 'H',
	'H', 'J', 'J', 'K', 'K', 'L', 'L', ':', ':', '"', '"', DVK_ENTER, DVK_ENTER, DVK_ENTER, DVK_ENTER, DVK_ENTER ,

	DVK_SHIFT, DVK_SHIFT, DVK_SHIFT, DVK_SHIFT, DVK_SHIFT, DVK_SHIFT, 'Z', 'Z', 'X', 'X', 'C', 'C', 'V', 'V', 'B', 'B',
	'N', 'N', 'M', 'M', '<', '<', '>', '>', '?', '?', NOKEY, NOKEY, DVK_UP, DVK_UP, NOKEY, NOKEY ,

	DVK_CTRL,	DVK_CTRL,  DVK_CTRL,  DVK_CTRL,  DVK_CTRL,  DVK_ALT,   DVK_ALT,   DVK_ALT,
	DVK_ALT,	DVK_SPACE, DVK_SPACE, DVK_SPACE, DVK_SPACE, DVK_SPACE, DVK_SPACE, DVK_SPACE,
	DVK_SPACE,	DVK_SPACE, DVK_SPACE, DVK_SPACE, DVK_SPACE, DVK_MENU,  DVK_MENU,  DVK_MENU,
	DVK_MENU,	DVK_MENU,  DVK_LEFT,  DVK_LEFT,  DVK_DOWN,  DVK_DOWN,  DVK_RIGHT, DVK_RIGHT

};

KeyMap capsLock = {
	keyboardGfxMap + 32 * 20,
	keyboardGfxMap,
	SimpleKbdUpper,
	32,
	5
};

KeyMap lowerCase = {
	keyboardGfxMap + 32 * 30,
	keyboardGfxMap + 32 * 10,
	SimpleKbdLower,
	32,
	5
};


Keyboard defaultKeyboard = {
	3, //background
	1, //display on sub screen
	0, //offset x
	0, //offset y
	8, //grid width
	16, //grid height
	Lower, //start with lower case
	0, //shifted
	0, //visible
	{&lowerCase, //keymap for lowercase
	&capsLock, //keymap for caps lock
	0, //keymap for numeric entry
	0}, //keymap for reduced footprint
	(const u16*)keyboardGfxTiles,  //graphics tiles
	keyboardGfxTilesLen, //graphics tiles length
	keyboardGfxPal, //palette
	keyboardGfxPalLen, //size of palette
	20, //map base
	0, //tile base
	0, //tile offset
	3, //scroll speed
	0, //keypress callback
	0, //key release callback
};

Keyboard *curKeyboard = NULL;

s16 keyboardGetKey(int x, int y) {
	if (curKeyboard == NULL) return NOKEY;

	KeyMap * keymap = curKeyboard->mappings[curKeyboard->state];
	x = (x - curKeyboard->offset_x) / curKeyboard->grid_width;
	y = (y + curKeyboard->offset_y) / curKeyboard->grid_height;

	if(x < 0 || y < 0 || x >= keymap->width || y >= keymap->height)
		return NOKEY;

	lastKey = keymap->keymap[x + y * keymap->width];

	return lastKey;
}

void keyboardShiftState(void) {
	if (curKeyboard == NULL) return;

	curKeyboard->state = curKeyboard->state == Upper ? Lower : Upper;

	KeyMap * map = curKeyboard->mappings[curKeyboard->state];

	dmaCopy(map->mapDataReleased, bgGetMapPtr(curKeyboard->background),
			map->width * map->height * curKeyboard->grid_height * curKeyboard->grid_width / 64 * 2);
}

void swapKeyGfx(int key, bool pressed) {
	if(curKeyboard == NULL || key == NOKEY) return;

	KeyMap * keymap = curKeyboard->mappings[curKeyboard->state];

	u16* map = bgGetMapPtr(curKeyboard->background);

	const u16* source = pressed ? keymap->mapDataPressed : keymap->mapDataReleased;

	int gx, gy;

	int gw = curKeyboard->grid_width >> 3;
	int gh = curKeyboard->grid_height >> 3;

	for(gy = 0; gy < keymap->height; gy++)
	for(gx = 0; gx < keymap->width; gx++) {
		if(keymap->keymap[gx + gy * keymap->width] == key) {
			int tx, ty;

			for(ty = 0; ty < gh; ty++)
				for(tx = 0; tx < gw; tx++) {
					int ox = tx + gx * gw;
					int oy = (ty + gy * gh) * 32;

					map[ox + oy] = source[ox + oy];
				}
		}
	}
}

s16 keyboardUpdate(void) {
	if (curKeyboard == NULL) return -1;

	static int pressed = 0;
	touchPosition touch;

	static u32 oldKeys = 0;

	u32 keys = keysCurrent();

	u32 temp = keys;
	keys &= ~oldKeys;
	oldKeys = temp;

	if(pressed) {
		if(!(keysCurrent() & KEY_TOUCH)) {
			pressed = 0;

			if(lastKey != NOKEY) swapKeyGfx(lastKey, false);

			if(lastKey == DVK_CAPS) {
				keyboardShiftState();
				return -1;
			} else if(lastKey == DVK_SHIFT) {
				keyboardShiftState();
				curKeyboard->shifted = curKeyboard->shifted ? 0 : 1;
				return -1;
			}

			if(curKeyboard->shifted) {
				keyboardShiftState();
				curKeyboard->shifted = 0;
			}
			if(curKeyboard->OnKeyReleased) curKeyboard->OnKeyReleased(lastKey);
		}

		return -1;
	} else {
		if(keys & KEY_TOUCH) {
			touchRead(&touch);

			int key = keyboardGetKey(touch.px, touch.py);

			if(key == NOKEY) return -1;

			pressed = 1;

			swapKeyGfx(key, true);

			if(curKeyboard->OnKeyPressed) curKeyboard->OnKeyPressed(lastKey);

			return lastKey;
		}
	}

	return -1;
}

Keyboard* keyboardGetDefault(void) {
	return &defaultKeyboard;
}

Keyboard* keyboardInit(Keyboard* keyboard, int layer, BgType type, BgSize size, int mapBase, int tileBase, bool mainDisplay, bool loadGraphics) {
	if(keyboard) {
		curKeyboard = keyboard;
	} else {
		if(curKeyboard == NULL) {
			curKeyboard = &defaultKeyboard;
		}
		keyboard = curKeyboard;
	}

	*keyboard = defaultKeyboard;

	keyboard->keyboardOnSub = !mainDisplay;

	if(keyboard->keyboardOnSub)
		keyboard->background = bgInitSub(layer, type, size, mapBase, tileBase);
	else
		keyboard->background = bgInit(layer, type, size, mapBase, tileBase);

	bgSetControlBits(keyboard->background, BIT(13));

	keyboard->mapBase = mapBase;
	keyboard->tileBase = tileBase;

	KeyMap* map = keyboard->mappings[keyboard->state];

	if(loadGraphics) {
		u16* pal = keyboard->keyboardOnSub ? BG_PALETTE_SUB : BG_PALETTE;

		decompress(keyboard->tiles, bgGetGfxPtr(keyboard->background),  LZ77Vram);

		dmaCopy(map->mapDataReleased, bgGetMapPtr(keyboard->background),
			map->width * map->height * keyboard->grid_height * keyboard->grid_width * 2 / 64);

		dmaCopy(keyboard->palette, pal, keyboard->paletteLen);
	}

	keyboard->offset_x = 0;
	keyboard->offset_y = -192 + map->height * keyboard->grid_height;

	keyboard->visible = 0;

	bgHide(keyboard->background);

	bgUpdate();

	return keyboard;
}

Keyboard* keyboardDemoInit(void) {
	return keyboardInit(keyboardGetDefault(), 3, BgType_Text4bpp, BgSize_T_256x512, defaultKeyboard.mapBase, defaultKeyboard.tileBase, false, true);
}

void keyboardShow(void) {
	if (curKeyboard == NULL) return;

	int i;

	swiWaitForVBlank();

	curKeyboard->visible = 1;

	bgSetScroll(curKeyboard->background, 0, -192);

	bgShow(curKeyboard->background);

	if(curKeyboard->scrollSpeed)
	{
		for(i = -192; i < curKeyboard->offset_y; i += curKeyboard->scrollSpeed)
		{
			swiWaitForVBlank();
			bgSetScroll(curKeyboard->background, 0, i);
			bgUpdate();
		}
	}

	bgSetScroll(curKeyboard->background, 0, curKeyboard->offset_y);
	bgUpdate();
}

void keyboardHide(void) {
	if (curKeyboard == NULL) return;

	int i;

	curKeyboard->visible = 0;

	if(curKeyboard->scrollSpeed)
	{
		for(i = curKeyboard->offset_y; i > -192; i-= curKeyboard->scrollSpeed)
		{
			swiWaitForVBlank();
			bgSetScroll(curKeyboard->background, 0, i);
			bgUpdate();
		}
	}
	bgHide(curKeyboard->background);
	bgUpdate();
}

s16 keyboardGetChar(void) {
	if (curKeyboard == NULL) return 0;

	int pressed;

	while(1) {
		swiWaitForVBlank();
		scanKeys();
		pressed = keysDown();

		if(pressed & KEY_TOUCH) {
			touchPosition touch;
			touchRead(&touch);

			int key = keyboardGetKey(touch.px, touch.py);

			if(key != NOKEY)
				return key;
		}
	}

	return 0;
}

void keyboardGetString(char * buffer, int maxLen) {
	char *end = buffer + maxLen;
	char c;

	while(buffer < end) {
		c = (char)keyboardGetChar();

		if(c == 0 || c == DVK_ENTER) break;

		*buffer++ = c;
	}
	*buffer = 0;
}




