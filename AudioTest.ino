#include <Audio.h>

#define SYNTH_DUALFILTER   0
#define SYNTH_SINGLEFILTER 1
#define SYNTH_SINGLE       2
#define SYNTH_MIXERTEST    3
#define SYNTH_MIXERTESTDC  4

#define OUTPUT_I2S 0
#define OUTPUT_DAC 1
#define OUTPUT_PWM 2

#define SYNTH_MODE  SYNTH_SINGLEFILTER
#define OUTPUT_MODE OUTPUT_I2S

AudioSynthWaveform       waveform1;
AudioSynthWaveform       waveform2;
AudioFilterStateVariable filter1;
AudioFilterStateVariable filter2;
AudioSynthWaveform       waveformCtrl;
AudioSynthWaveformDc     waveformDC;
AudioMixerXF             mixer;
#if OUTPUT_MODE == OUTPUT_I2S
AudioOutputI2S           output;
AudioControlSGTL5000     sgtl5000_1;
#elif OUTPUT_MODE == OUTPUT_DAC
AudioOutputAnalog        output;
#elif OUTPUT_MODE == OUTPUT_PWM
AudioOutputPWM           output;
#endif

#if SYNTH_MODE == SYNTH_DUALFILTER 
AudioConnection          patchCordW1(waveform1, 0, filter1, 0);
AudioConnection          patchCordW2(waveform2, 0, filter2, 0);
AudioConnection          patchCordF1(filter1, 0, mixer, 0);
AudioConnection          patchCordF2(filter2, 0, mixer, 1);
AudioConnection          patchCordL(mixer, 0, output, 0);
#if OUTPUT_MODE == OUTPUT_I2S
AudioConnection          patchCordR(mixer, 0, output, 1);
#endif

#elif SYNTH_MODE == SYNTH_SINGLEFILTER
AudioConnection          patchCordW1(waveform1, 0, filter1, 0);
AudioConnection          patchCordL(filter1, 0, output, 0);
#if OUTPUT_MODE == OUTPUT_I2S
AudioConnection          patchCordR(filter1, 0, output, 1);
#endif

#elif SYNTH_MODE == SYNTH_SINGLE
AudioConnection          patchCordL(waveform1, 0, output, 0);
#if OUTPUT_MODE == OUTPUT_I2S
AudioConnection          patchCordR(waveform1, 0, output, 1);
#endif

#elif SYNTH_MODE == SYNTH_MIXERTEST || SYNTH_MODE == SYNTH_MIXERTESTDC
AudioConnection          patchCordW1(waveform1, 0, filter1, 0);
AudioConnection          patchCordW2(waveform2, 0, filter2, 0);
AudioConnection          patchCordF1(filter1, 0, mixer, 0);
AudioConnection          patchCordF2(filter2, 0, mixer, 1);
#if SYNTH_MODE == SYNTH_MIXERTEST
AudioConnection          patchCordCTRL(waveformCtrl, 0, mixer, 2);
#else
AudioConnection          patchCordCTRL(waveformDC, 0, mixer, 2);
#endif
AudioConnection          patchCordL(mixer, 0, output, 0);
#if OUTPUT_MODE == OUTPUT_I2S
AudioConnection          patchCordR(mixer, 0, output, 1);
#endif

#endif

const float masterTuning = 220;
float tuning = masterTuning;

float masterVolume = 0.01;
float detune  = 0;
float freq    = 0;
float phase   = 0;
float balance = -1;

uint8_t wf1 = WAVEFORM_SAWTOOTH;
uint8_t wf2 = wf1;
//uint8_t wf2 = WAVEFORM_SQUARE;

void setup() {
  Serial1.begin(115200);
  
  AudioMemory(10);
#if OUTPUT_MODE == OUTPUT_I2S
  sgtl5000_1.enable();
  sgtl5000_1.volume(masterVolume);
#endif
  
  filter1.frequency(15000.);
  filter2.frequency(15000.);
  
  waveform1.begin(wf1);
  waveform2.begin(wf2);
  applyTuning();
#if SYNTH_MODE == SYNTH_MIXERTEST
  waveformCtrl.begin(1,freq,WAVEFORM_SINE);
#endif
#if SYNTH_MODE == SYNTH_MIXERTESTDC
  applyBalance();
#endif
  waveform1.amplitude(1);
  waveform2.amplitude(1);

  delay(1000);
  Serial1.println("Startup complete");
}

inline void updateMasterVolume() {
  float knob = (float) analogRead(A1) / 1280.0;
  if( fabs(masterVolume-knob) > 0.01) {
    masterVolume = knob;
    Serial1.print("Volume: ");
    Serial1.println(masterVolume);
#if OUTPUT_MODE == OUTPUT_I2S
    sgtl5000_1.volume(masterVolume);
#endif
  }
}

inline void updatePhase() {
  // read the volume knob
  float knob = analogRead(A1)*360./1023.;
  if ( fabs(phase-knob) > 1.)
  {
    phase = knob;
    Serial1.print("Setting phase to ");
    Serial1.println(phase);
    applyPhase();
  }
}

inline void applyTuning() 
{
  waveform1.frequency(tuning);
  waveform2.frequency(tuning * pow(2,detune/768.));
  applyPhase();
}

  
inline void updateTuning() {
  float knob = analogRead(A1)/1023.;
  static float last = -1;
  if (fabs(knob-last) > 0.01)
  {
    last = knob;
    tuning = masterTuning * pow(2,((2*knob)-1)/12.);
    Serial1.print("Setting tuning to ");
    Serial1.println(tuning);
    applyTuning();
  }
}

inline void updateDetune() {
  float knob = analogRead(A1)*128/1023.-64;
  if (fabs(detune-knob)>0.5)
  {
    detune = knob;
    Serial1.print("Setting detune to ");
    Serial1.println(detune);
    applyTuning();
  }
}

inline void updateFrequency()
{
  if (freq<0) freq=0;
  waveformCtrl.frequency(freq);
  Serial1.print("Frequency: ");
  Serial1.println(freq);
}

inline void applyPhase()
{
  if (phase<0) phase=0;
  AudioNoInterrupts();
  waveform1.phase(0);
  waveform2.phase(phase);
  AudioInterrupts();
}


inline void sweepPhase()
{
  static float dp = 0.001;
  static int sign=1;
  if (phase > 135) sign = -1;
  if (phase < 45)  sign = 1;
  phase += sign*dp;
  applyPhase();
}

inline void applyBalance()
{
  if (balance > 1) balance = 1;
  else if (balance < -1) balance = -1;
  waveformDC.amplitude(balance);
}

void loop() {
  //updateMasterVolume();
  //updateTuning();
  //updatePhase();
  //updateDetune();
  //sweepPhase();
  if (Serial1.available()) 
  {
    switch (Serial1.read())
    {
    case '.':
      freq += 1;
      updateFrequency();
      break;
    case ',':
      freq += 0.05;
      updateFrequency();
      break;
    case 'm':
      freq -= 0.05;
      updateFrequency();
      break;
    case 'n':
      freq -= 1;
      updateFrequency();
      break;
    case 'v':
      phase += 1;
      applyPhase();
      break;
    case 'c':
      phase += 0.1;
      applyPhase();
      break;
    case 'x':
      phase -= 0.1;
      applyPhase();
      break;
    case 'z':
      phase -= 1;
      applyPhase();
      break;
    case 's':
      balance += 0.1;
      applyBalance();
      break;
    case 'a':
      balance -= 0.1;
      applyBalance();
      break;
    case 'i':
      Serial1.println(AudioMemoryUsageMax());
      AudioMemoryUsageMaxReset(); 
      Serial1.println(AudioProcessorUsageMax());
      AudioProcessorUsageMaxReset(); 
      break;
    }
  };
}
