#include <Arduino.h>

#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>
#include <AudioFileSourceSD.h>
#include <AudioOutputMixer.h>
#include <driver/i2s.h>

#include <control.h>

#define NUM_CHANNELS      2

bool channelsInUse[NUM_CHANNELS];
float gainLevels[NUM_CHANNELS];

bool fadeRunning = false;
int fadeChannel;
long fadeStartingTime;
int fadeEndingTime;
float fadeStartingGain;
float fadeEndingGain;

AudioGeneratorWAV *wav[NUM_CHANNELS];
AudioFileSourceSD *file[NUM_CHANNELS];
AudioOutputI2S *out;
AudioOutputMixer *mixer;
AudioOutputMixerStub *stub[NUM_CHANNELS];


//============================================================//
//Basic functions
//============================================================//

void stopWav(int channel) {
  wav[channel]->stop();
  stub[channel]->stop();
  channelsInUse[channel] = false;
}

void playWav(const char *filename, int channel, float gain) {

  if (channelsInUse[channel]) stopWav(channel);
  
  file[channel] = new AudioFileSourceSD(filename);
  stub[channel]->SetGain(gain);
  wav[channel]->begin(file[channel], stub[channel]);
  channelsInUse[channel] = true;
  gainLevels[channel] = gain;

}

bool isPlaying(int channel) {return wav[channel]->isRunning();}

void setGain(int channel, float gain) {
  stub[channel]->SetGain(gain);
  gainLevels[channel] = gain;
}

float getGain(int channel) {return gainLevels[channel];}


//============================================================//
//Advanced functions
//============================================================//

void stopFade() {fadeRunning = false;}

void fadeTo(int channel, int timeMs, float gain) {

  fadeStartingTime = millis();
  fadeEndingTime = timeMs;
  fadeStartingGain = gainLevels[channel];
  fadeEndingGain = gain;
  fadeChannel = channel;
  if (fadeStartingGain == gain) return;
  fadeRunning = true;

}

void fadeUpdate() {
  if (gainLevels[fadeChannel] != fadeEndingGain) {
    int timeElapsed = millis() - fadeStartingTime;
    gainLevels[fadeChannel] = mapFloat(timeElapsed, 0, fadeEndingTime, fadeStartingGain, fadeEndingGain);

    if (fadeStartingGain > fadeEndingGain && gainLevels[fadeChannel] < fadeEndingGain) gainLevels[fadeChannel] = fadeEndingGain;
    if (fadeStartingGain < fadeEndingGain && gainLevels[fadeChannel] > fadeEndingGain) gainLevels[fadeChannel] = fadeEndingGain;

    setGain(fadeChannel, gainLevels[fadeChannel]);

  } else {
    fadeRunning = false;
  }

  if(gainLevels[fadeChannel] == 0.0) stopWav(fadeChannel);
}


//============================================================//
//Utility functions
//============================================================//

void audioInit() {

  out = new AudioOutputI2S();
  out->SetOutputModeMono(true);
  mixer = new AudioOutputMixer(256, out);

  for (int channel = 0; channel < NUM_CHANNELS; channel++) {
    channelsInUse[channel] = false;
    stub[channel] = mixer->NewInput();
    wav[channel] = new AudioGeneratorWAV();
    gainLevels[channel] = 0.0;
  }

}

void audioLoop(/*void *parameter*/)  {
  
  //for(;;) {

    for(int channel = 0; channel < NUM_CHANNELS; channel++) {
      if(channelsInUse[channel] && wav[channel]->isRunning() && !wav[channel]->loop()) stopWav(channel);
    }

    if(fadeRunning) fadeUpdate();

  //}

}

void dualCoreAudioLoop(void *parameter) {

  for(;;) for(int channel = 0; channel < NUM_CHANNELS; channel++) if(channelsInUse[channel] && wav[channel]->isRunning() && !wav[channel]->loop()) stopWav(channel);

}