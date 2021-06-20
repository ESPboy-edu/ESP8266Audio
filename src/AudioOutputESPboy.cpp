/*
  AudioOutputI2S
  Base class for I2S interface port
  
  Copyright (C) 2017  Earle F. Philhower, III

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Arduino.h>
#include "AudioOutputESPboy.h"
#include <sigma_delta.h>

static unsigned char soundBuf1[MAX_BUFFER_SIZE];
static unsigned char soundBuf2[MAX_BUFFER_SIZE];
static int soundBufSwap;	//0 buf1 plays, buf2 accepts data; 1 vice versa
static int soundWr;
static int soundRd;
static int soundBufSize;

void ICACHE_RAM_ATTR sound_callback()
{
	if(!soundBufSwap)
	{
		sigmaDeltaWrite(0, soundBuf1[soundRd++]);
	}
	else
	{
		sigmaDeltaWrite(0, soundBuf2[soundRd++]);
	}

	if(soundRd>=soundBufSize)
	{
		soundRd=0;
		soundWr=0;
		soundBufSwap^=1;
	}
}

AudioOutputESPboy::AudioOutputESPboy(int sPin)
{
	audioLogger->println(F("ESPboy audio output set"));
	audioLogger->println(sPin);
	
	active=false;
	soundPin=sPin;
	hertz=44100;	//default sample rate
	
	memset(soundBuf1, 0, sizeof(soundBuf1));
	memset(soundBuf2, 0, sizeof(soundBuf2));
	
    SetGain(1.7);
	
	begin();
}

AudioOutputESPboy::~AudioOutputESPboy()
{
	stop();
}

bool AudioOutputESPboy::SetRate(int hz)
{
	audioLogger->println(F("SetRate"));
	audioLogger->println(hz);
	
	this->hertz = hz;
	soundBufSize=this->hertz/100;
	if(soundBufSize>MAX_BUFFER_SIZE) soundBufSize=MAX_BUFFER_SIZE;
	
	if(active)
	{
		timer1_write(80000000 / this->hertz);
	}

	return true;
}

bool AudioOutputESPboy::ConsumeSample(int16_t sample[2])
{
	if(!active) return false;

	if(soundWr>=soundBufSize) return false;

	int16_t ms[2];

	ms[0] = sample[0];
	ms[1] = sample[1];
	MakeSampleStereo16( ms );

	int32_t out = Amplify((ms[LEFTCHANNEL] + ms[RIGHTCHANNEL])>>1)/127;

	if(out<0) out=-out;
	
	out=(out*out+out*out+out*out)/700;
	
	if(out>255) out=255;

	//if(out>0xc0) out=255; else out=0;	//loud 1-bit sound

	if(!soundBufSwap)
	{	
		soundBuf2[soundWr++]=out;
	}
	else
	{
		soundBuf1[soundWr++]=out;
	}
	
	return true;
}

bool AudioOutputESPboy::begin()
{
	audioLogger->println(F("ESPboy audio begin"));
	audioLogger->println(hertz);

	active=false;
	
	soundWr=0;
	soundRd=0;
	
	sigmaDeltaSetup(0, F_CPU / 256);
	sigmaDeltaAttachPin(soundPin);
	sigmaDeltaEnable();

	noInterrupts();
	timer1_attachInterrupt(sound_callback);
	timer1_enable(TIM_DIV1, TIM_EDGE, TIM_LOOP);
	timer1_write(80000000 / this->hertz);
	interrupts();
		
	active=true;

	return true;
}

bool AudioOutputESPboy::stop()
{
	if(!active) return false;

	audioLogger->println(F("ESPboy audio stop"));
	
	timer1_disable();
	sigmaDeltaDisable();
  
	active=false;
	
	return true;
}
