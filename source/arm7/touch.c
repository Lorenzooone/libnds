// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005-2010 Michael Noland (joat)
// Copyright (C) 2005-2010 Jason Rogers (Dovoto)
// Copyright (C) 2005-2010 Dave Murphy (WinterMute)

// Touch screen control for the ARM7

#include <nds/ndstypes.h>
#include <nds/system.h>
#include <nds/arm7/codec.h>
#include <nds/arm7/touch.h>
#include <nds/arm7/input.h>
#include <nds/interrupts.h>

#include <stdlib.h>

static u8 last_time_touched = 0;

static u8 range_counter_1 = 0;
static u8 range_counter_2 = 0;
static u8 range = 20;
static u8 min_range = 20;

static u8 CheckStylus(void)
{
	SerialWaitBusy();

	REG_SPICNT = SPI_ENABLE | SPI_BAUD_2MHz | SPI_DEVICE_TOUCH | SPI_CONTINUOUS; //0x8A01;
	REG_SPIDATA = TSC_MEASURE_TEMP1;

	SerialWaitBusy();

	REG_SPIDATA = 0;

	SerialWaitBusy();

	REG_SPICNT = SPI_ENABLE | SPI_BAUD_2MHz | SPI_DEVICE_TOUCH;// 0x8201;
	REG_SPIDATA = 0;

	SerialWaitBusy();

	if(last_time_touched == 1){
		if( !(REG_KEYXY & KEYXY_TOUCH) )
			return 1;
		else{
			REG_SPICNT = SPI_ENABLE | SPI_BAUD_2MHz | SPI_DEVICE_TOUCH | SPI_CONTINUOUS;
			REG_SPIDATA = TSC_MEASURE_TEMP1;

			SerialWaitBusy();

			REG_SPIDATA = 0;

			SerialWaitBusy();

			REG_SPICNT = SPI_ENABLE | SPI_BAUD_2MHz | SPI_DEVICE_TOUCH;
			REG_SPIDATA = 0;

			SerialWaitBusy();

			return !(REG_KEYXY & KEYXY_TOUCH) ? 2 : 0;
		}
	}else{
		return !(REG_KEYXY & KEYXY_TOUCH) ? 1 : 0;
	}
}

uint16 touchRead(uint32 command)
{
	uint16 result, result2;

	uint32 oldIME = REG_IME;

	REG_IME = 0;

	SerialWaitBusy();

	// Write the command and wait for it to complete
	REG_SPICNT = SPI_ENABLE | SPI_BAUD_2MHz | SPI_DEVICE_TOUCH | SPI_CONTINUOUS; //0x8A01;
	REG_SPIDATA = command;
	SerialWaitBusy();

	// Write the second command and clock in part of the data
	REG_SPIDATA = 0;
	SerialWaitBusy();
	result = REG_SPIDATA;

	// Clock in the rest of the data (last transfer)
	REG_SPICNT = SPI_ENABLE | 0x201;
	REG_SPIDATA = 0;
	SerialWaitBusy();

	result2 = REG_SPIDATA >>3;

	REG_IME = oldIME;

	// Return the result
	return ((result & 0x7F) << 5) | result2;
}

uint32 touchReadTemperature(int *t1, int *t2)
{
	*t1 = touchRead(TSC_MEASURE_TEMP1);
	*t2 = touchRead(TSC_MEASURE_TEMP2);
	return 8490 * (*t2 - *t1) - 273*4096;
}

int16 readTouchValue(uint32 command, int16 *dist_max, u8 *err)
{
	int16 values[5];
	int32 aux1, aux2, aux3, dist, dist2, result = 0;
	u8 i, j, k;

	*err = 1;

	SerialWaitBusy();

	REG_SPICNT = SPI_ENABLE | SPI_BAUD_2MHz | SPI_DEVICE_TOUCH | SPI_CONTINUOUS;
	REG_SPIDATA = command;

	SerialWaitBusy();

	for(i=0; i<5; i++){
		REG_SPIDATA = 0;
		SerialWaitBusy();

		aux1 = REG_SPIDATA;
		aux1 = aux1 & 0xFF;
		aux1 = aux1 << 16;
		aux1 = aux1 >> 8;

		values[4-i] = aux1;

		REG_SPIDATA = command;
		SerialWaitBusy();

		aux1 = REG_SPIDATA;
		aux1 = aux1 & 0xFF;
		aux1 = aux1 << 16;

		aux1 = values[4-i] | (aux1 >> 16);
		values[4-i] = ((aux1 & 0x7FF8) >> 3);
	}

	REG_SPICNT = SPI_ENABLE | SPI_BAUD_2MHz | SPI_DEVICE_TOUCH;
	REG_SPIDATA = 0;
	SerialWaitBusy();

	dist = 0;
	for(i=0; i<4; i++){
		aux1 = values[i];

		for(j=i+1; j<5; j++){
			aux2 = values[j];
			aux2 = abs(aux1 - aux2);
			if(aux2>dist) dist = aux2;
		}
	}

	*dist_max = dist;

	for(i=0; i<3; i++){
		aux1 = values[i];

		for(j=i+1; j<4; j++){
			aux2 = values[j];
			dist = abs(aux1 - aux2);

			if( dist <= range ){
				for(k=j+1; k<5; k++){
					aux3 = values[k];
					dist2 = abs(aux1 - aux3);

					if( dist2 <= range ){
						result = aux2 + (aux1 << 1);
						result = result + aux3;
						result = result >> 2;
						result = result & (~7);

						*err = 0;

						break;
					}
				}
			}
		}
	}

	if((*err) == 1){
		result = values[0] + values[4];
		result = result >> 1;
		result = result & (~7);
	}

	return (result & 0xFFF);
}

void UpdateRange(uint8 *this_range, int16 last_dist_max, u8 data_error, u8 tsc_touched)
{
	//range_counter_1 = counter_0x380A98C
	//range_counter_2 = counter_0x380A990
	//Initial values:
	// range = 20
	// min_range = 20

	if(tsc_touched != 0){
		if( data_error == 0){
			range_counter_2 = 0;

			if( last_dist_max >= ((*this_range) >> 1)){
				range_counter_1 = 0;
			}else{
				range_counter_1++;

				if(range_counter_1 >= 4){
					range_counter_1 = 0;

					if((*this_range) > min_range){
						(*this_range)--;
						range_counter_2 = 3;
					}
				}
			}
		}else{
			range_counter_1 = 0;
			range_counter_2++;

			if(range_counter_2 >= 4){

				range_counter_2 = 0;

				if((*this_range) < 35){  //0x23 = 35
					*this_range = (*this_range) + 1;
				}
			}
		}
	}else{
		range_counter_2 = 0;
		range_counter_1 = 0;
	}
}

static void touchReadDSMode(touchPosition *touchPos)
{
	int16 dist_max_y, dist_max_x, dist_max;
	u8 error, error_where, first_check, i;

	first_check = CheckStylus();
	if (first_check != 0) {
		error_where = 0;

		touchPos->z1 =  readTouchValue(TSC_MEASURE_Z1 | 1, &dist_max, &error);
		touchPos->z2 =  readTouchValue(TSC_MEASURE_Z2 | 1, &dist_max, &error);

		touchPos->rawx = readTouchValue(TSC_MEASURE_X | 1, &dist_max_x, &error);
		if(error==1) error_where += 1;

		touchPos->rawy = readTouchValue(TSC_MEASURE_Y | 1, &dist_max_y, &error);
		if(error==1) error_where += 2;

		REG_SPICNT = SPI_ENABLE | SPI_BAUD_2MHz | SPI_DEVICE_TOUCH | SPI_CONTINUOUS;
		for(i=0; i<12; i++){
			REG_SPIDATA = 0;

			SerialWaitBusy();
		}

		REG_SPICNT = SPI_ENABLE | SPI_BAUD_2MHz | SPI_DEVICE_TOUCH;
		REG_SPIDATA = 0;

		SerialWaitBusy();

		if(first_check == 2) error_where = 3;

		switch( CheckStylus() ){
		case 0:
			last_time_touched = 0;
			break;
		case 1:
			last_time_touched = 1;

			if(dist_max_x > dist_max_y)
				dist_max = dist_max_x;
			else
				dist_max = dist_max_y;

			break;
		case 2:
			last_time_touched = 0;
			error_where = 3;

			break;
		}
	} else {
		dist_max = 0;
		error_where = 3;
		touchPos->rawx = 0;
		touchPos->rawy = 0;
		last_time_touched = 0;
	}

	UpdateRange(&range, dist_max, error_where, last_time_touched);
}

static s32 xscale, yscale;
static s32 xoffset, yoffset;

void touchInit(void)
{
	xscale = ((PersonalData->calX2px - PersonalData->calX1px) << 19) / ((PersonalData->calX2) - (PersonalData->calX1));
	yscale = ((PersonalData->calY2px - PersonalData->calY1px) << 19) / ((PersonalData->calY2) - (PersonalData->calY1));

	xoffset = ((PersonalData->calX1 + PersonalData->calX2) * xscale  - ((PersonalData->calX1px + PersonalData->calX2px) << 19) ) / 2;
	yoffset = ((PersonalData->calY1 + PersonalData->calY2) * yscale  - ((PersonalData->calY1px + PersonalData->calY2px) << 19) ) / 2;

	if (cdcIsAvailable()) {
		int oldIME = enterCriticalSection();
		cdcTouchInit();
		leaveCriticalSection(oldIME);
	}
}

bool touchPenDown(void)
{
	bool down;
	int oldIME = enterCriticalSection();
	if (cdcIsAvailable()) {
		down = cdcTouchPenDown();
	} else {
		down = !(REG_KEYXY & KEYXY_TOUCH);
	}
	leaveCriticalSection(oldIME);
	return down;
}

void touchReadXY(touchPosition *touchPos)
{
	int oldIME = enterCriticalSection();
	if (cdcIsAvailable()) {
		cdcTouchRead(touchPos);
	} else {
		touchReadDSMode(touchPos);
	}
	leaveCriticalSection(oldIME);

	s16 px = ( touchPos->rawx * xscale - xoffset + xscale/2 ) >>19;
	s16 py = ( touchPos->rawy * yscale - yoffset + yscale/2 ) >>19;

	if ( px < 0) px = 0;
	if ( py < 0) py = 0;
	if ( px > (SCREEN_WIDTH -1)) px = SCREEN_WIDTH -1;
	if ( py > (SCREEN_HEIGHT -1)) py = SCREEN_HEIGHT -1;

	touchPos->px = px;
	touchPos->py = py;
}
