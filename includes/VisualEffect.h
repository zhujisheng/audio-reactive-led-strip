/*
 * Transfer from Mel-Frequency to Leds Effect
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#pragma once
#include <FastLED.h>
#include <cmath>
#include "ExpFilter.h"
#include "gaussian_filter1d.h"

class VisualEffect{
    private:
      class ExpFilter *_gain, *_p_filt_r, *_p_filt_g, *_p_filt_b, *_common_mode, *_r_filt, *_g_filt, *_b_filt;
      class gaussian_filter1d *_gauss02, *_gauss40;
      uint8_t *_leds[3];
      uint16_t _mel_num, _leds_num;
      void mirror(CRGB * physic_leds);
      float * _spectrum, * _prev_spectrum;
    public:
        VisualEffect(uint16_t mel_num, uint16_t leds_num);
        void visualize_scroll(float * mel_data, CRGB * physic_leds);
        void visualize_energy(float * mel_data, CRGB * physic_leds);
        void visualize_spectrum(float * mel_data, CRGB * physic_leds);
        ~VisualEffect();
};

VisualEffect::VisualEffect(uint16_t mel_num, uint16_t leds_num){
    _mel_num = mel_num;
    _leds_num = leds_num;
    _leds[0] = new uint8_t[_leds_num/2];
    _leds[1] = new uint8_t[_leds_num/2];
    _leds[2] = new uint8_t[_leds_num/2];
    _gain = new ExpFilter(_mel_num, 0.001, 0.99);
    _p_filt_r = new ExpFilter(leds_num/2, 0.05, 0.99);
    _p_filt_g = new ExpFilter(leds_num/2, 0.05, 0.99);
    _p_filt_b = new ExpFilter(leds_num/2, 0.05, 0.99);
    _common_mode = new ExpFilter(_leds_num/2, 0.99, 0.01);
    _r_filt = new ExpFilter(_leds_num/2, 0.2, 0.99);
    _g_filt = new ExpFilter(_leds_num/2, 0.04, 0.3);
    _b_filt = new ExpFilter(_leds_num/2, 0.1, 0.5);
    _gauss02 = new gaussian_filter1d(0.2);
    _gauss40 = new gaussian_filter1d(4.0);

    _spectrum = new float[leds_num/2];
    _prev_spectrum = new float[leds_num/2];
}

VisualEffect::~VisualEffect(){
  delete [] _leds[0];
  delete [] _leds[1];
  delete [] _leds[2];
  delete _gain;
  delete _p_filt_r;
  delete _p_filt_g;
  delete _p_filt_b;
  delete _common_mode;
  delete _r_filt;
  delete _g_filt;
  delete _b_filt;
  delete _gauss02;
  delete _gauss40;
  delete [] _spectrum;
  delete [] _prev_spectrum;
}

void VisualEffect::mirror(CRGB * physic_leds){
  for(int i=0; i< _leds_num/2; i++){
    physic_leds[_leds_num/2+i].r = _leds[0][i];
    physic_leds[_leds_num/2+i].g = _leds[1][i];
    physic_leds[_leds_num/2+i].b = _leds[2][i];
    physic_leds[_leds_num/2-i-1].r = _leds[0][i];
    physic_leds[_leds_num/2-i-1].g = _leds[1][i];
    physic_leds[_leds_num/2-i-1].b = _leds[2][i];
  }
}

void VisualEffect::visualize_scroll(float * mel_data, CRGB * physic_leds){
  float rr,gg,bb;

  for (int i = 0; i < _mel_num; i++)
    mel_data[i] = mel_data[i] * mel_data[i];

  _gain->update0(mel_data);
  for (int i = 0; i < _mel_num; i++)
    if (_gain->value()[i] > 0.0)
      mel_data[i] /= (_gain->value()[i]);

  rr=0.0;
  gg=0.0;
  bb=0.0;
  for(int i=0; i<_mel_num; i++)
    if(i<_mel_num/3)
      rr = std::max(rr,mel_data[i]);
    else if(i>_mel_num*2/3)
      bb = std::max(bb,mel_data[i]);
    else
      gg = std::max(gg,mel_data[i]);

  for(int i=_leds_num/2-1; i>0; i--){
    _leds[0][i] = (_leds[0][i-1]==0) ? 0:_leds[0][i-1]-1;
    _leds[1][i] = (_leds[1][i-1]==0) ? 0:_leds[1][i-1]-1;
    _leds[2][i] = (_leds[2][i-1]==0) ? 0:_leds[2][i-1]-1;
  }
  _gauss02->process(_leds[0],_leds_num/2);
  _gauss02->process(_leds[1],_leds_num/2);
  _gauss02->process(_leds[2],_leds_num/2);

  _leds[0][0]  = 255*rr;
  _leds[1][0]  = 255*gg;
  _leds[2][0]  = 255*bb;

  mirror(physic_leds);
}

void VisualEffect::visualize_energy(float * mel_data, CRGB * physic_leds){
  float rr,gg,bb;
  int ri,gi,bi;

  _gain->update0(mel_data);
  for (int i = 0; i < _mel_num; i++)
    if (_gain->value()[i] > 0.0)
      mel_data[i] /= (_gain->value()[i]);

  rr=0.0;
  gg=0.0;
  bb=0.0;
  ri=bi=gi=0;
  for(int i=0; i<_mel_num; i++)
    if(i<_mel_num/3){
      ri++;
      rr += mel_data[i];
    }
    else if(i>_mel_num*2/3){
      bi++;
      bb += mel_data[i];
    }
    else{
      gi++;
      gg += mel_data[i];
    }
  rr =rr*_leds_num/2/ri;
  bb =bb*_leds_num/2/bi;
  gg =gg*_leds_num/2/gi;

  for(int i=0; i<_leds_num/2; i++){
    _leds[0][i] = (i+1>rr)?0:255;
    _leds[1][i] = (i+1>gg)?0:255;
    _leds[2][i] = (i+1>bb)?0:255;
  }

  _p_filt_r->update(_leds[0]);
  _p_filt_g->update(_leds[1]);
  _p_filt_b->update(_leds[2]);
  _gauss40->process(_leds[0],_leds_num/2);
  _gauss40->process(_leds[1],_leds_num/2);
  _gauss40->process(_leds[2],_leds_num/2);

  mirror(physic_leds);
}

void VisualEffect::visualize_spectrum(float * mel_data, CRGB * physic_leds){
  float one_unit = 1.0/(_leds_num/2-1);
  int j=1;
  _spectrum[0] = mel_data[0];
  _spectrum[_leds_num/2-1] = mel_data[_mel_num-1];

  for(int i=1; i<_mel_num; i++){
    float t;
    while((t=one_unit*j*(_mel_num-1))<i){
      //_spectrum[j]=(0.5-0.5*cos(PI*((i-t)*mel_data[i-1]+(t-i+1)*mel_data[i])));
      _spectrum[j]=((i-t)*mel_data[i-1]+(t-i+1)*mel_data[i]);
      j++;
    }
  }
  _common_mode->update0(_spectrum);

  for(int i=0; i<_leds_num/2; i++){
    _leds[0][i] = (_spectrum[i]-_common_mode->value()[i])*255;
    _leds[1][i] = fabs(_spectrum[i]-_prev_spectrum[i])*255;
    _leds[2][i] = _spectrum[i]*255;
    _prev_spectrum[i] = _spectrum[i];
  }
  _r_filt->update(_leds[0]);
  _g_filt->update(_leds[1]);
  _b_filt->update(_leds[2]);
  _gauss02->process(_leds[0],_leds_num/2);
  _gauss02->process(_leds[1],_leds_num/2);
  _gauss02->process(_leds[2],_leds_num/2);
  mirror(physic_leds);
}

