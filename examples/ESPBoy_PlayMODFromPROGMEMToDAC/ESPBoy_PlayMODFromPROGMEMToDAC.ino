#include <Arduino.h>
#include "AudioFileSourcePROGMEM.h"
#include "AudioGeneratorMOD.h"
//#include "AudioOutputI2S.h"
#include "AudioOutputESPboy.h"
#include "lib/ESPboyInit.h"
#include "lib/ESPboyInit.cpp"

ESPboyInit myESPboy;

// enigma.mod sample from the mod archive: https://modarchive.org/index.php?request=view_by_moduleid&query=42146
#include "enigma.h"

AudioGeneratorMOD *mod;
AudioFileSourcePROGMEM *file;
//AudioOutputI2S *out;
AudioOutputESPboy *out;

void setup(){
  myESPboy.begin("ESP826AUDIO WAV play");
  Serial.begin(115200);
  delay(1000);

  audioLogger = &Serial;
  file = new AudioFileSourcePROGMEM( enigma_mod, sizeof(enigma_mod) );
  // out = new AudioOutputI2S(0, 1); Uncomment this line, comment the next one to use the internal DAC channel 1 (pin25) on ESP32
  //out = new AudioOutputI2S();
 out = new AudioOutputESPboy(D3);  //sound pin
  pinMode(D3, OUTPUT);
  mod = new AudioGeneratorMOD();
  mod->SetBufferSize(3*1024);
  mod->SetSampleRate(44100);
  mod->SetStereoSeparation(32);
  mod->begin(file, out);
}

void loop()
{
  if (mod->isRunning()) {
    if (!mod->loop()) mod->stop();
  } else {
    Serial.printf("MOD done\n");
    delay(1000);
  }
}
