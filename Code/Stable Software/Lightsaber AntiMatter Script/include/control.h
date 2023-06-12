#include <Arduino.h>

#define pi  3.141592
#define e   2.718281

#define iterationCount  30

float calculateMagnitudeVelocity(float x, float y, float z) {return sqrt(x*x + y*y + z*z);}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;}

//Preconditions:
//Point of contact is less than the length of the blade
uint32_t calculateColor(float ledHeight, int mmFromContact, int mu, int maximumBrightness, uint32_t color, float minSlope) {
    
    //Distance function: f(x) = (a/sigma*(sqrt(2*PI)))*e^-(1/2)*((x-mu)/sigma)^2    INCORRECT
    //Color function: g(x) = (b(a-1)(x-255)/255)+x 
    
    float brightnessLevel = (float)mmFromContact*pow(e, sq(ledHeight-(float)mu)*log(1.0/(float)mmFromContact)/sq((float)maximumBrightness));

    if (brightnessLevel < 1.0) {
        return color;
    }

    uint32_t rVal = color>>16;
    uint32_t gVal = (color<<16)>>24;
    uint32_t bVal = (color<<24)>>24;                        

    rVal = (uint32_t)((brightnessLevel*(minSlope-1.0)*((float)rVal-255.0)/255.0)+(float)rVal);
    gVal = (uint32_t)((brightnessLevel*(minSlope-1.0)*((float)gVal-255.0)/255.0)+(float)gVal);
    bVal = (uint32_t)((brightnessLevel*(minSlope-1.0)*((float)bVal-255.0)/255.0)+(float)bVal);

    return 0x00000000 | bVal | (gVal<<8) | (rVal<<16);

}

uint32_t findLargest(uint32_t colourValues[], int length) {
    uint32_t theLargest = 0;
    for(int i = 0; i < length; i++) if(colourValues[i] > theLargest) theLargest = colourValues[i];
    return theLargest;
}

uint32_t calculatePixel(uint32_t pixel1, uint32_t pixel2, uint32_t pixel3, uint32_t pixel4) {
    
    uint32_t pixels[4] = {pixel1, pixel2, pixel3, pixel4};
    uint32_t rVal[4];
    uint32_t gVal[4];
    uint32_t bVal[4];

    for(int i = 0; i < 4; i++) {
        rVal[i] = pixels[i]>>16;
        gVal[i] = (pixels[i]<<16)>>24;
        bVal[i] = (pixels[i]<<24)>>24;
    }

    uint32_t largestRVal = findLargest(rVal, 4);
    uint32_t largestBVal = findLargest(bVal, 4);
    uint32_t largestGVal = findLargest(gVal, 4);

    return 0x00000000 | largestBVal | (largestGVal<<8) | (largestRVal<<16);
}

uint32_t bumpPixelBrightness(uint32_t color, int brightnessLevel) {

    if(brightnessLevel == 0) return color;
    
    uint32_t rVal = color>>16;
    uint32_t gVal = (color<<16)>>24;
    uint32_t bVal = (color<<24)>>24;

    //Check this 0.1 level
    if(brightnessLevel > 0) {
        rVal = (uint32_t)(((float)brightnessLevel*(0.1-1.0)*((float)rVal-255.0)/255.0)+(float)rVal);
        gVal = (uint32_t)(((float)brightnessLevel*(0.1-1.0)*((float)gVal-255.0)/255.0)+(float)gVal);
        bVal = (uint32_t)(((float)brightnessLevel*(0.1-1.0)*((float)bVal-255.0)/255.0)+(float)bVal);
    } else {
        float floatBrightnessLevel = mapFloat(brightnessLevel, -255.0, 0.0, 0.0, 1.0); //Figure out these settings
        rVal = (uint32_t)(floatBrightnessLevel*rVal);
        gVal = (uint32_t)(floatBrightnessLevel*gVal);
        bVal = (uint32_t)(floatBrightnessLevel*bVal);
    }

    return 0x00000000 | bVal | (gVal<<8) | (rVal<<16);
}

int getBumpBrightness(float distanceAlong, float waveDensity, float amplitude, float animationCycle, float decayFactor, float decayTransformation, int maxDesiredBrightness, float verticalOffset) {
    
    if(distanceAlong < 0 || distanceAlong > 760) return -1;

    int theBrightness = int(
        (
            amplitude/2.0
        )*
        (
            pow(e, decayTransformation-decayFactor*distanceAlong)
        )*
        (
            sin(
                pi*
                (
                    (2*waveDensity*distanceAlong)/760.0-animationCycle
                )
            ) + verticalOffset
        )
        ); //CHECK THIS

    if(abs(theBrightness) > maxDesiredBrightness) return -2;

    return theBrightness;

}