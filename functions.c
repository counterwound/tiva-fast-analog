#include "functions.h"

int getPeak(uint8_t* data, int length){
    int i;
    uint8_t max = 0;
    for(i = 0; i < length; i++){
        if(data[i] > max){
            max = data[i];
        }
    }
    return (max * (3.3/4095));
}

int getFWHM(uint8_t* data, int length){
    int max = getPeak(data,length) * (4095/3.3);
    int halfmax = max / 2;
    int i1 = 0;
    int i2 = 0;
    int i;

    for(i = 0; i < length - 1; i++){
        if(data[i] < halfmax <= data[i+1]){
            i1 = i;
        }
        else if(data[i] > halfmax >= data[i+1]){
            i2 = i;
        }
        if((i2 != 0) && (i1 != 0) && (i1 != i2)){
            break;
        }
    }

    return (i2 - i1);
}
