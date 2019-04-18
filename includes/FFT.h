/*
 * Fast Fourier Transformation/Mel-Frequency
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#pragma once
#include <cmath>
#include "ExpFilter.h"

class FFT{
    private:
        float* _hammer=NULL;
        uint16_t _num_mel_bands, _num_samples, _sample_rate;
        float _min_frequency, _max_frequency, _min_volume_threshold;
        float * _y_data_cal;
        float** _melmat=NULL;
        float hz2mel(float f);
        float mel2hz(float m);
        void compute_hammer();
        void compute_melmat(uint16_t num_mel_bands, float freq_min, float freq_max, uint16_t num_fft_bands, uint16_t sample_rate);
        class ExpFilter * _mel_gain, * _mel_smoothing;

    public:
        FFT(uint16_t samples, uint16_t n_mel_bin, float min_frequency, float max_frequency, uint16_t sample_rate, float min_volume_threshold);
        ~FFT();
        void fft(float * real, float * imag);
        void fft(float * real);
        void abs(float * real, float * imag);
        void hamming(float *real);
        void t2mel(float * y_data, float * mel_data);
};

FFT::FFT(uint16_t samples, uint16_t n_mel_bin, float min_frequency, float max_frequency, uint16_t sample_rate, float min_volume_threshold){
    _num_samples = samples;
    _num_mel_bands = n_mel_bin;
    _min_frequency = min_frequency;
    _max_frequency = max_frequency;
    _sample_rate = sample_rate;
    _min_volume_threshold = min_volume_threshold;

    compute_hammer();
    compute_melmat(_num_mel_bands, _min_frequency, _max_frequency, _num_samples/2, _sample_rate);

    _mel_gain = new ExpFilter(1, 0.06, 0.99);
    _mel_smoothing = new ExpFilter(_num_mel_bands, 0.5, 0.99);

    _y_data_cal = (float*)malloc(_num_samples*sizeof(float));
}

FFT::~FFT(){
    delete _mel_gain;
    delete _mel_smoothing;
    free(_y_data_cal);
    if (_melmat){
        for(uint16_t i=0; i<_num_mel_bands; i++)
            delete []_melmat[i];
        delete []_melmat;
    }
    if(_hammer)
        delete [] _hammer;
}

void FFT::compute_hammer(){
    _hammer = new float[_num_samples];
    for(uint16_t i=0; i<_num_samples; i++)
        _hammer[i] = (0.54 - 0.46*cos(2.0*M_PI*i/(_num_samples-1)));
}

void FFT::hamming(float *real){
    for(uint16_t i=0; i<0; i++)
        real[i] *= _hammer[i];
}


float FFT::hz2mel(float f){
    return 2595.0*log10(1.0+f/700.0);
}

float FFT::mel2hz(float m){
    return 700.0*(pow(10.0,m/2595.0)-1.0);
}

void FFT::compute_melmat(uint16_t num_mel_bands, float freq_min, float freq_max, uint16_t num_fft_bands, uint16_t sample_rate){

    _melmat = new float*[num_mel_bands];
    for(uint16_t i=0; i<num_mel_bands; i++)
        _melmat[i] = new float[num_fft_bands];

    float lowFreqMel = hz2mel(freq_min);
    float highFreqMel = hz2mel (freq_max);
    
    float* filterCentreFreq = new float[num_mel_bands+2];
    for(uint16_t i=0; i<num_mel_bands+2; i++)
        filterCentreFreq[i] = mel2hz(lowFreqMel + (highFreqMel-lowFreqMel)/(num_mel_bands+1)*i);

    float* fftBinFreq = new float[num_fft_bands];
    for (uint16_t i=0; i<num_fft_bands; i++)
        fftBinFreq[i]=(sample_rate/2.0/(num_fft_bands-1)*i);

    for (uint16_t filt=1; filt<=num_mel_bands; filt++) {
        for (uint16_t bin=0; bin<num_fft_bands; bin++) {
            float weight;
            if (fftBinFreq[bin] < filterCentreFreq[filt-1])
                weight = 0.0;
            else if (fftBinFreq[bin] <= filterCentreFreq[filt])
                weight = (fftBinFreq[bin] - filterCentreFreq[filt-1]) / (filterCentreFreq[filt] - filterCentreFreq[filt-1]);
            else if (fftBinFreq[bin] <= filterCentreFreq[filt+1])
                weight = (filterCentreFreq[filt+1] - fftBinFreq[bin]) / (filterCentreFreq[filt+1] - filterCentreFreq[filt]);
            else
                weight = 0.0;
            _melmat[filt-1][bin] = weight;
        }
    }
    delete [] filterCentreFreq;
    delete [] fftBinFreq;
}

void FFT::fft(float * real){
    float *imag;
    imag = new float[_num_samples]();
    fft(real, imag);
    abs(real, imag);
    delete [] imag;
}

void FFT::fft(float * real, float * imag){
    uint16_t j = 0;
    float tmp;
    for (uint16_t i = 0; i < (_num_samples - 1); i++) {
        if (i < j) {
            tmp = real[i];
            real[i] = real[j];
            real[j] = tmp;
        }
        uint16_t k = (_num_samples >> 1);
        while (k <= j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }

    // Compute the POWER  
    uint8_t power = 0;
    while (((_num_samples >> power) & 1) != 1) power++;

    // Compute the FFT
    float c1 = -1.0;
    float c2 = 0.0;
    uint16_t l2 = 1;
    for (uint8_t l = 0; (l < power); l++) {
        uint16_t l1 = l2;
        l2 <<= 1;
        float u1 = 1.0;
        float u2 = 0.0;
        for (j = 0; j < l1; j++) {
            for (uint16_t i = j; i < _num_samples; i += l2) {
                    uint16_t i1 = i + l1;
                    float t1 = u1 * real[i1] - u2 * imag[i1];
                    float t2 = u1 * imag[i1] + u2 * real[i1];
                    real[i1] = real[i] - t1;
                    imag[i1] = imag[i] - t2;
                    real[i] += t1;
                    imag[i] += t2;
            }
            float z = ((u1 * c1) - (u2 * c2));
            u2 = ((u1 * c2) + (u2 * c1));
            u1 = z;
        }
        c2 = sqrt((1.0 - c1) / 2.0);
        c2 = -c2;
        c1 = sqrt((1.0 + c1) / 2.0);
    }
}

void FFT::abs(float * real, float * imag){
    for(uint16_t i=0; i<_num_samples/2; i++){
        real[i] = sqrt(real[i]*real[i]+imag[i]*imag[i]);
    }
}

void FFT::t2mel(float * y_data, float * mel_data){
  float minData, maxData;
  minData = y_data[0];
  maxData = y_data[0];
  int jj;
  for(jj=1; jj<_num_samples; jj++){
    if(y_data[jj]<minData) minData=y_data[jj];
    if(y_data[jj]>maxData) maxData=y_data[jj];
    if(maxData-minData>_min_volume_threshold) break;
  }
  if(jj==_num_samples){
    for(int i=0; i<_num_mel_bands; i++)
      mel_data[i] = 0.0;
    return;
  }

  memcpy(_y_data_cal, y_data, sizeof(float)*_num_samples);
  hamming(_y_data_cal);
  fft(_y_data_cal);

  float max_mel;
  max_mel = 0.0;
  for (int i = 0; i < _num_mel_bands; i++) {
    mel_data[i] = 0.0;
    for (int j = 0; j < _num_samples/2; j++) {
      mel_data[i] += _y_data_cal[j] * _melmat[i][j];
    }
    mel_data[i] = mel_data[i] * mel_data[i];
    max_mel = std::max(mel_data[i],max_mel);
  }

  _mel_gain->update(&max_mel);
  if (max_mel > 0.0)
    for (int i = 0; i < _num_mel_bands; i++)
      mel_data[i] /= max_mel;

  _mel_smoothing->update(mel_data);
}


