#include "lenet.h"
#include "model.h"
#include <time.h>
#include <stdlib.h>
#include <math.h>

/*
DSPLIB_DATA(input,4)
_iq31 input[128];

DSPLIB_DATA(weight,4)
_iq31 weight[128];

*/
DSPLIB_DATA(mac_result,4)
_iq31 mac_result[1];

DSPLIB_DATA(result,4)
_q15 result[10];


unsigned short int i,j,z,o0,o1,i0,i1,l0,l1,x,w0,w1,y,k,tl1,tl2,length1,length2;
int temp1,temp2,temp3;

msp_mac_q15_params macParams;
msp_fill_q15_params fillParams;
msp_add_q15_params addParams;
msp_iq31_to_q15_params convParams;

msp_cmplx_fft_q15_params fftParams;
msp_cmplx_mpy_q15_params mpyParams;
msp_shift_q15_params shiftparams;
msp_cmplx_q15_params comparam;

#define S             128

#define GETLENGTH(array) (sizeof(array)/sizeof(*(array)))

#define GETCOUNT(array)  (sizeof(array)/sizeof(float))

#define FOREACH(i,count) for (i = 0; i < count; ++i)

#define CONVOLUTE_VALID(input,output,weight)                                            \
{                                                                                       \
    FOREACH(o0,GETLENGTH(output))                                                       \
        FOREACH(o1,GETLENGTH(*(output)))                                                \
            FOREACH(w0,GETLENGTH(weight))                                               \
                FOREACH(w1,GETLENGTH(*(weight)))                                        \
                    (output)[o0][o1] += (input)[o0 + w0][o1 + w1] * (weight)[w0][w1];   \
}



#define CONVOLUTE_VALID_ACCE(input,output,weight)                                            \
{                                                                                       \
    int w0l = GETLENGTH(weight);                         \
    int w1l = GETLENGTH(*(weight));                       \
    dma_transfer_macro(&(weight)[0][0],&we[0],w0l*w1l);\
    FOREACH(o0,GETLENGTH(output))                                                       \
        FOREACH(o1,GETLENGTH(*(output))){                                                \
            FOREACH(w0,w0l)                                               \
                memcpy(&in[w0*w0l],&(input)[o0 + w0][o1],w1l*sizeof(int16_t)); \
            MAC_(w0l,w1l);   \
            output[o0][o1] += res; \
        }\
}

#define CONVOLUTE_VALID_ACCE_old(input,output,weight)                                            \
{                                                                                       \
    int w0l = GETLENGTH(weight);                         \
    int w1l = GETLENGTH(*(weight));                       \
    memcpy(&we[0],&(weight)[0][0],w0l*w1l*sizeof(int16_t));    \
    FOREACH(o0,GETLENGTH(output))                                                       \
        FOREACH(o1,GETLENGTH(*(output))){                                                \
            FOREACH(w0,w0l)                                               \
                memcpy(&in[w0*w0l],&(input)[o0 + w0][o1],w1l*sizeof(int16_t));    \
            MAC_(w0l,w1l);   \
            memcpy(&(output)[o0][o1],&res,sizeof(float));    \
        }\
}

#define CONVOLUTE_FULL(input,output,weight)                                             \
{                                                                                       \
    FOREACH(i0,GETLENGTH(input))                                                        \
        FOREACH(i1,GETLENGTH(*(input)))                                                 \
            FOREACH(w0,GETLENGTH(weight))                                               \
                FOREACH(w1,GETLENGTH(*(weight)))                                        \
                (output)[i0 + w0][i1 + w1] += (input)[i0][i1] * (weight)[w0][w1];   \
}

#define CONVOLUTION_FORWARD(input,output,weight,bias,action)                    \
{                                                                               \
    for (x = 0; x < GETLENGTH(weight); ++x)                                 \
        for (y = 0; y < GETLENGTH(*weight); ++y)                            \
        CONVOLUTE_VALID(input[x], output[y], weight[x][y]);                 \
    FOREACH(j, GETLENGTH(output))                                               \
        FOREACH(i, GETCOUNT(output[j]))                                         \
        ((float *)output[j])[i] = action(((float *)output[j])[i] + bias[j]);  \
}


#define CONVOLUTION_FORWARD_NBIAS(input,output,weight,bias,action)                    \
{                                                                               \
    for (x = 0; x < GETLENGTH(weight); ++x)                                 \
        for (y = 0; y < GETLENGTH(*weight); ++y)                            \
            CONVOLUTE_VALID_ACCE(input[x], output[y], weight[x][y]);                 \
    add_bias(&output[0][0][0],bias,GETLENGTH(output),GETLENGTH(**output),GETLENGTH(*output)); \
}


#define CONVOLUTION_FIRST_PART(input,output,weight,bias,action,floop,sloop,finit,sinit)                    \
{ \
    for (x = finit; x <floop ; x++)                                 \
        for (y = sinit; y <sloop ; y++)                            \
            CONVOLUTE_VALID(input[x], output[y], weight[x%2][y]);                 \
}


#define SUBSAMP_MAX_FORWARD(input,output)                                                       \
{                                                                                               \
    const int len0 = GETLENGTH(*(input)) / GETLENGTH(*(output));                                \
    const int len1 = GETLENGTH(**(input)) / GETLENGTH(**(output));                              \
    FOREACH(i, GETLENGTH(output))                                                               \
    FOREACH(o0, GETLENGTH(*(output)))                                                           \
    FOREACH(o1, GETLENGTH(**(output)))                                                          \
    {                                                                                           \
        int x0 = 0, x1 = 0, ismax;                                                              \
        FOREACH(l0, len0)                                                                       \
            FOREACH(l1, len1)                                                                   \
        {                                                                                       \
            ismax = input[i][o0*len0 + l0][o1*len1 + l1] > input[i][o0*len0 + x0][o1*len1 + x1];\
            x0 += ismax * (l0 - x0);                                                            \
            x1 += ismax * (l1 - x1);                                                            \
        }                                                                                       \
        output[i][o0][o1] = input[i][o0*len0 + x0][o1*len1 + x1];                               \
    }                                                                                           \
}

#define DOT_PRODUCT_FORWARD(input,output,weight,bias,action)                \
{                                                                           \
    for (x = 0; x < GETLENGTH(weight); ++x)                             \
        for (y = 0; y < GETLENGTH(*weight); ++y)                        \
            output[x] += input[y] * weight[x][y];   \
    FOREACH(j, GETLENGTH(bias))                                             \
        ((float *)output)[j] = action(((float *)output)[j] + bias[j]);  \
}

#define dma_transfer_macro(input, output, size) \
{   \
    __data20_write_long((uintptr_t) &DMA0SA,(uintptr_t) input); \
    __data20_write_long((uintptr_t) &DMA0DA,(uintptr_t) output);\
    DMA0SZ = size;                       \
    DMA0CTL = DMADT_5 | DMASRCINCR_3 | DMADSTINCR_3; \
    DMA0CTL |= DMAEN;                      \
    msp_benchmarkStart(MSP_BENCHMARK_BASE, 16);\
    DMA0CTL |= DMAREQ;\
    cycleCount = msp_benchmarkStop(MSP_BENCHMARK_BASE);\
}


static void MAC_(int16_t w0l,int16_t w1l)
{
    res = 0;
    macParams.length = 32;
    convParams.length = 2;
    msp_mac_q15(&macParams, we, in, &mac_result[0]);
    msp_iq31_to_q15(&convParams,&mac_result[0],&res);
    length1 += 1;
    //for(i=0;i<w0l*w1l;i++)
        //res += in[i]*we[i];
}


static void reluQ_ar(_q15 *x, int size)
{
    FOREACH(i, size)
    {
        if(x[i]<0)
            x[i] = 0;
    }
}

static void add_bias_1d(int16_t *output,const int16_t *bias,int len, bool relu)
{
    addParams.length = len;
    dma_transfer_macro(output,&circularMatrixColumn[0],len);
    dma_transfer_macro(bias,&inputMatrixColumn[0],len);
    msp_add_q15(&addParams, inputMatrixColumn, circularMatrixColumn, inputMatrixColumn);
    if(relu) reluQ_ar(inputMatrixColumn,len);
    dma_transfer_macro(inputMatrixColumn,output,len);
}

static void add_bias(int16_t *output,const int16_t *bias,int len, int len1, int len2)
{
    int slength, length;

    short int counter=0;
    FOREACH(j, len)
    {
        slength = length = len1*len2;
        memcpy(&we[0], bias + j,sizeof(int16_t));
        fillParams.value = we[0];
        fillParams.length = 256;
        msp_fill_q15(&fillParams,&circularMatrixColumn[0]);
        while(true)
        {
            if(slength<=0)
            {
                counter = 0;
                output = (output + len1*len2);
                break;
            }
            if(slength > 256)
                length = 256;
            else
                length = slength;


            addParams.length = length;


            dma_transfer_macro((output+counter*256),&inputMatrixColumn[0],length);

            msp_add_q15(&addParams, inputMatrixColumn, circularMatrixColumn, inputMatrixColumn);

            reluQ_ar(inputMatrixColumn,length);

            dma_transfer_macro(&inputMatrixColumn[0],(output+counter*256),length);

            counter ++;
            slength -= 256;
        }
    }

}

static void FC_convolution_last_old(int16_t(*action)(int16_t))
{
    for (x = 0; x < 10; ++x)
    {
        for (y = 0; y < 128; ++y)
        {
            f_output[x] += f_layerLast[y]*weight5_6[x][y];
        }
        f_output[x] = action(f_output[x] + n_bias5_6[x]);
    }
}

static void FC_convolution_last(int16_t(*action)(int16_t))
{
    macParams.length = 256;
    convParams.length = 2;

    dma_transfer_macro(&f_layerLast[0],&inputMatrixColumn[0], 256);
    for (x = 0; x < 10; ++x)
    {
        dma_transfer_macro(&weight5_6[x][0],&circularMatrixColumn[0], 256);

        msp_mac_q15(&macParams, circularMatrixColumn, inputMatrixColumn, &mac_result[0]);
        msp_iq31_to_q15(&convParams,&mac_result[0],&res);
        result[x] = res;
    }

    memcpy( &f_output[0], &result[0], 10* sizeof( int16_t ) );

    add_bias_1d(f_output,n_bias5_6,GETLENGTH(n_bias5_6),false);

}

static void FC_convolution_last_acce(int bl,const int16_t *weightPtr,int16_t *inputPtr, int inputLen, int16_t *outputPtr, int outputLen,const int *biasPtr)
{
    //msp_mac_iq31_params macParams;
    macParams.length = bl;
    convParams.length = 2;

    dma_transfer_macro( inputPtr,&inputMatrixColumn[0], bl);
    for (x = 0; x < outputLen; ++x)
    {
        dma_transfer_macro( weightPtr +(x*bl),&circularMatrixColumn[0], bl);

        //msp_mac_iq31(&macParams, circularMatrixColumn, inputMatrixColumn, &result[x]);
        msp_mac_q15(&macParams, circularMatrixColumn, inputMatrixColumn, &mac_result[0]);
        msp_iq31_to_q15(&convParams,&mac_result[0],&res);
        result[x] = res;
    }

    dma_transfer_macro(result, outputPtr, outputLen);

    add_bias_1d(outputPtr,biasPtr,outputLen,true);
}

int16_t relu(int16_t x)
{
    return x*(x > 0);
}


float relugrad(float y)
{
    return y > 0;
}


static void fftmultiplication(int size_,int shft)
{
    shiftparams.length = size_*2;
    mpyParams.length = size_;

    fftParams.length = size_;
    fftParams.bitReverse = 1;
    fftParams.twiddleTable = msp_cmplx_twiddle_table_256_q15;

    int i,j=size_*2;

    for(i=size_;i>0;i--)
    {
        circularMatrixColumn[j-2] = circularMatrixColumn[i-1];
        circularMatrixColumn[j-1] = 0;

        inputMatrixColumn[j-2] = inputMatrixColumn[i-1];
        inputMatrixColumn[j-1] = 0;

        j = j-2;
    }

    msp_cmplx_fft_fixed_q15(&fftParams, circularMatrixColumn);

    msp_cmplx_fft_fixed_q15(&fftParams, &inputMatrixColumn[0]);         // Run LEA once for cycle count

    msp_cmplx_mpy_q15(&mpyParams, inputMatrixColumn, circularMatrixColumn, inputMatrixColumn);

    msp_cmplx_ifft_fixed_q15(&fftParams, &inputMatrixColumn[0]);


    shiftparams.shift = shft;
    msp_shift_q15(&shiftparams,inputMatrixColumn,inputMatrixColumn);
    // real number extraction needed
    i=0;
    for (z = 0; z < size_; ++z){
        inputMatrixColumn[z]=inputMatrixColumn[i];
        i=i+2;
    }
    i = 0;
}



static void FC_Convolution(int bl,int scale,const int16_t *weightPtr, int weightLen,int16_t *inputPtr, int inputLen, int16_t *outputPtr, int outputLen,const int *biasPtr)
{
    length1=length2=0;
    while(true)
    {
        if(length1>=weightLen)
            break;
        int round = (length1%outputLen);
        dma_transfer_macro(inputPtr+(length1%inputLen),&circularMatrixColumn[0], bl);
        dma_transfer_macro(weightPtr + length1,&inputMatrixColumn[0], bl);


        fftmultiplication(bl,scale);

        dma_transfer_macro(outputPtr +round,&circularMatrixColumn[0], bl);
        addParams.length = bl;
        msp_add_q15(&addParams, inputMatrixColumn, circularMatrixColumn, inputMatrixColumn);
        dma_transfer_macro(&inputMatrixColumn[0],outputPtr +round, bl);


        length1 += bl;
    }

    add_bias_1d(outputPtr,biasPtr,outputLen,true);
}


static void forward(int16_t(*action)(int16_t))
{
    CONVOLUTION_FORWARD_NBIAS(f_input,f_layer1, n_weight0_1, n_bias0_1, action);
    SUBSAMP_MAX_FORWARD(f_layer1, f_layer2);

    CONVOLUTION_FORWARD_NBIAS(f_layer2, f_layer3, n_weight2_3, n_bias2_3, action);
    SUBSAMP_MAX_FORWARD(f_layer3, f_layer4);

    FC_Convolution(16,4,weight4_5, GETLENGTH(weight4_5),&f_layer4[0][0][0],GETLENGTH(f_layer4),f_layerLast,GETLENGTH(f_layerLast),n_bias4_5);

    FC_convolution_last_acce(256,&weight5_6[0][0],f_layerLast,GETLENGTH(f_layerLast),f_output,OUTPUT,n_bias5_6);
}




static uint8 get_result(uint8 count)
{
    int16_t *output = (int16_t *)f_output;
    //const int outlen = GETCOUNT(features->output);
    uint8 result = 0;
    int16_t maxvalue = *output;
    for (i = 1; i < count; ++i)
    {
        if (output[i] > maxvalue)
        {
            maxvalue = output[i];
            result = i;
        }
    }
    return result;
}

static void normalization()
{
    float mean = 0.5;
    float std = 0.5;
    float t = 0;
    FOREACH(j, 28)
        FOREACH(k, 28)
    {
        t = ((float)f_input[0][j][k]/255 - mean)/std;
        f_input[0][j][k] =  t * pow(2,15);
    }
}

uint8 Predict(image input,uint8 count)
{
    //fftmultiplication();
    normalization();
    forward(relu);
    return get_result(count);
}

/* Weight matrices:
 * n_weight0_1[1][8][5][5] = 200
 * n_weight2_3[8][25][5][5] = 5000
 * weight4_5[6400] 6400
 * weight5_6[10][256] = 2560
 * total weights = 14160
 */
void Update(uint16_t pos, int16_t value)
{
    int16_t *p;
    uint16_t i;

    if (pos < 200)
    {
        p = n_weight0_1;
        i = pos;
    }
    else if (pos < 5200)
    {
        p = n_weight2_3;
        i = pos - 200;
    }
    else if (pos < 11600)
    {
        p = weight4_5;
        i = pos - 5200;
    }
    else
    {
        p = weight5_6;
        i = pos - 11600;
    }
    p[i] = value;
}

int Check()
{
    if (n_weight0_1[0][0][0][0] == 1 && n_weight0_1[0][0][0][1] == 2 && weight5_6[0][0] == 9)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

