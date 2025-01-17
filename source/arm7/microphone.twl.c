// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2017 fincs

// DSi microphone control

#include <nds/interrupts.h>
#include <nds/fifocommon.h>
#include <nds/timers.h>
#include <nds/arm7/audio.h>
#include <nds/arm7/codec.h>

void micSetAmp_TWL(u8 control, u8 gain)
{
	static const u8 gaintbl[] = { 0x1F, 0x2B, 0x37, 0x43 };

	if (control == PM_AMP_ON) {
		cdcWriteReg(CDC_SOUND, CDC_SOUND_MIC_BIAS, 0x03); // set adc bias
		bool adcOn = cdcReadReg(CDC_CONTROL, CDC_CONTROL_ADC_MIC) & 0x80;
		bool dacOn = cdcReadReg(CDC_CONTROL, CDC_CONTROL_DAC_CTRL) & 0xC0;
		cdcWriteReg(CDC_CONTROL, CDC_CONTROL_ADC_MIC, 0x80); // turn on adc
		if (!adcOn || !dacOn) {
			swiDelay(0x28E91F); // 20ms
		}
		cdcWriteReg(CDC_CONTROL, CDC_CONTROL_ADC_VOL_FINE, 0x00); // unmute adc
		cdcWriteReg(CDC_SOUND, CDC_SOUND_MIC_GAIN, gaintbl[gain&3]); // set gain
	} else if (control == PM_AMP_OFF) {
		cdcWriteReg(CDC_CONTROL, CDC_CONTROL_ADC_VOL_FINE, 0x80); // mute adc
		cdcWriteReg(CDC_CONTROL, CDC_CONTROL_ADC_MIC, 0x00); // turn off adc
		cdcWriteReg(CDC_SOUND, CDC_SOUND_MIC_BIAS, 0x00); // disable adc bias
	}
}

u16 micReadData16_TWL(void)
{
	u16 data = 0x8000;
	int timeout = 0;

	REG_MICCNT &= ~MICCNT_ENABLE;
	REG_MICCNT &= ~( (3<<13) | 0xF );
	REG_MICCNT |= MICCNT_CLEAR_FIFO | MICCNT_FORMAT(2);
	REG_MICCNT |= MICCNT_ENABLE;

	while (timeout++ < 200) {
		if (!(REG_MICCNT & MICCNT_EMPTY)) {
			data = REG_MICDATA;
			break;
		}
	}

	return data ^ 0x8000; // convert to unsigned
}
