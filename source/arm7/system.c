// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005-2010 Michael Noland (joat)
// Copyright (C) 2005-2010 Jason Rogers (Dovoto)
// Copyright (C) 2005-2010 Dave Murphy (WinterMute)

#include <nds/ndstypes.h>
#include <nds/system.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/interrupts.h>
#include <nds/bios.h>
#include <nds/arm7/clock.h>
#include <nds/arm7/sdmmc.h>
#include <nds/arm7/i2c.h>

bool sleepIsEnabled = true;
bool __dsimode = false; // set in crt0

void twlEnableSlot1(void);
void twlDisableSlot1(void);

void enableSlot1(void)
{
    if (isDSiMode())
        twlEnableSlot1();
}

void disableSlot1(void)
{
    if (isDSiMode())
        twlDisableSlot1();
}

void powerValueHandler(u32 value, void *user_data)
{
	(void)user_data;

	u32 temp;
	u32 ie_save;
	int battery, backlight, power;

	switch(value & 0xFFFF0000) {
		//power control
	case PM_REQ_LED:
		ledBlink(value);
		break;
	case PM_REQ_ON:
		temp = readPowerManagement(PM_CONTROL_REG);
		writePowerManagement(PM_CONTROL_REG, temp | (value & 0xFFFF));
		break;
	case PM_REQ_OFF:
		temp = readPowerManagement(PM_CONTROL_REG) & (~(value & 0xFFFF));
		writePowerManagement(PM_CONTROL_REG, temp);
		break;

	case PM_REQ_SLEEP:
			
		ie_save = REG_IE;
		// Turn the speaker down.
		if (REG_POWERCNT & 1) swiChangeSoundBias(0,0x400);
		// Save current power state.
		power = readPowerManagement(PM_CONTROL_REG);
		// Set sleep LED.
		writePowerManagement(PM_CONTROL_REG, PM_LED_CONTROL(1));
		// Register for the lid interrupt.
		REG_IE = IRQ_LID;

		// Power down till we get our interrupt.
		swiSleep(); //waits for PM (lid open) interrupt

		//100ms
		swiDelay(838000);
		
		// Restore the interrupt state.
		REG_IE = ie_save;

		// Restore power state.
		writePowerManagement(PM_CONTROL_REG, power);

		// Turn the speaker up.
		if (REG_POWERCNT & 1) swiChangeSoundBias(1,0x400); 

		// update clock tracking
		resyncClock();
		break;

	case PM_REQ_SLEEP_DISABLE:
		sleepIsEnabled = false;
		break;

	case PM_REQ_SLEEP_ENABLE:
		sleepIsEnabled = true;
		break;
	case PM_REQ_BATTERY:
		if (!isDSiMode()) {
			battery = (readPowerManagement(PM_BATTERY_REG) & 1)?3:15;
			backlight = readPowerManagement(PM_BACKLIGHT_LEVEL);
			if (backlight & (1<<6)) battery += (backlight & (1<<3))<<4;
		} else {
			battery = i2cReadRegister(I2C_PM,I2CREGPM_BATTERY);
		}
		fifoSendValue32(FIFO_PM, battery);
		break;
	case PM_REQ_SLOT1_DISABLE:
		disableSlot1();
		break;
	case PM_REQ_SLOT1_ENABLE:
		enableSlot1();
		break;
	}
}

void systemSleep(void)
{
    if (!sleepIsEnabled)
         return;

    // Puts arm9 to sleep, which then notifies arm7 above, which causes arm7 to
    // sleep.
    fifoSendValue32(FIFO_SYSTEM, PM_REQ_SLEEP);
}

int sleepEnabled(void)
{
    return sleepIsEnabled;
}

void storageMsgHandler(int bytes, void *user_data);
void storageValueHandler(u32 value, void *user_data);
void firmwareMsgHandler(int bytes, void *user_data);

void installSystemFIFO(void)
{
    fifoSetValue32Handler(FIFO_PM, powerValueHandler, 0);
    fifoSetValue32Handler(FIFO_STORAGE, storageValueHandler, 0);
    fifoSetDatamsgHandler(FIFO_STORAGE, storageMsgHandler, 0);
    fifoSetDatamsgHandler(FIFO_FIRMWARE, firmwareMsgHandler, 0);
}


