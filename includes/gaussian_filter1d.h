/*
 * Gaussian Filter 1 dimension
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#pragma once
#include <cmath>

class gaussian_filter1d{
    private:
        float *_gaussian_kernel1d;
        int radius;
    public:
        gaussian_filter1d(float sigma);
        ~gaussian_filter1d();
        void process(float * data, int num);
        void process(uint8_t * data, int num);
};

gaussian_filter1d::gaussian_filter1d(float sigma){
    float sum=0;
    radius=(int)(4.0*sigma+0.5);
    _gaussian_kernel1d = new float[radius*2+1];
    _gaussian_kernel1d[radius] = 1.0;
    for(int i=1; i<=radius; i++){
        _gaussian_kernel1d[radius+i] = exp(-0.5/(sigma * sigma)*i*i);
    }

    for(int i=1; i<=radius; i++){
        sum += _gaussian_kernel1d[radius+i];
    }
    sum = 2.0*sum + 1.0;

    _gaussian_kernel1d[radius] = 1.0/sum;
    for(int i=1; i<=radius; i++){
        _gaussian_kernel1d[radius+i] /= sum;
        _gaussian_kernel1d[radius-i] = _gaussian_kernel1d[radius+i];
    }
}

gaussian_filter1d::~gaussian_filter1d(){
    delete[] _gaussian_kernel1d;
}

void gaussian_filter1d::process(uint8_t * data, int num){
    uint8_t * origin = new uint8_t[num];
    for(int i=0; i<num; i++){
        origin[i] = data[i];
    }

    for(int i=0; i<num; i++){
        data[i] = (uint8_t)(origin[i]*_gaussian_kernel1d[radius]);
        for( int j=1; j<=radius; j++ ){
            data[i] += (uint8_t)(origin[(i-j<0)?(j-i-1):(i-j)]*_gaussian_kernel1d[radius-j]);
            data[i] += (uint8_t)(origin[(i+j>=num)?(2*num-j-i-1):(i+j)]*_gaussian_kernel1d[radius+j]);
        }
    }
    delete [] origin;
}

void gaussian_filter1d::process(float * data, int num){
    float * origin = new float[num];
    for(int i=0; i<num; i++){
        origin[i] = data[i];
    }

    for(int i=0; i<num; i++){
        data[i] = origin[i]*_gaussian_kernel1d[radius];
        for( int j=1; j<=radius; j++ ){
            data[i] += origin[(i-j<0)?(j-i-1):(i-j)]*_gaussian_kernel1d[radius-j];
            data[i] += origin[(i+j>=num)?(2*num-j-i-1):(i+j)]*_gaussian_kernel1d[radius+j];
        }
    }
    delete [] origin;
}

