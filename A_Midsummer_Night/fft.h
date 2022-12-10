#ifndef FFT_H
#define FFT_H

#include <math.h>
#include <complex>

#define M_PI 3.14159

class cFFT {
private:
    unsigned int N, which;
    unsigned int log_2_N;
    float pi2;
    bool invers_FFT;
    unsigned int* reversed;
    std::complex<float>** T;
    std::complex<float>* c[2];
    std::complex<float>* _c;
protected:
public:
    //cFFT(unsigned int N);
    cFFT(unsigned int N, bool I);//N为傅立叶N值，I为是否为IFFT
    ~cFFT();
    unsigned int reverse(unsigned int i);
    std::complex<float> t(unsigned int x, unsigned int N);
    //void fft(std::complex<float>* input, std::complex<float>* output, int stride, int offset);
    void fft(std::complex<float>* input, int stride, int offset);
};

#endif