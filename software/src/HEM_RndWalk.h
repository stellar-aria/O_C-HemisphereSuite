// Copyright (c) 2022, Alessio Degani
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <Arduino.h>
#include "OC_core.h"
#include "HemisphereApplet.h"

#define PROB_UP 500
#define PROB_DN 500

#define MAX_RANGE 255
#define MAX_STEP 255
#define MAX_SMOOTH 255

#define HEM_1V 1536 //ADC value for 1 Volt
#define HEM_1ST 128 //ADC value for 1 semitone
#define HEM_HST 64 //ADC value for half semitone

class RndWalk : public HemisphereApplet {
public:

    const char* applet_name() { // Maximum 10 characters
        return "RndWalk";
    }

    void Start() {
        ForEachChannel(ch)
        {
            // rndSeed[ch] = random(1, 255);
            currentVal[ch] = 0;
            currentOut[ch] = 0;
            UpdateAlpha();
        }
        cursor = 0;
    }

    void Controller() {
        // Main LOOP
        // for triggers read from Clock(0|1)
        // for CV in read from In(0|1)
        // for CV out write to Out(0|1, value)
        // INPUT
        int maxVal = HEMISPHERE_MAX_CV;
        switch (cvRange) {
            case 0:
                maxVal = HEM_HST;
                break;
            case 1:
                maxVal = HEM_1ST;
                break;
            case 2:
                maxVal = HEM_1V;
                break;
            default:
                break;
        }
        int rangeCv = Proportion(In(0), HEMISPHERE_MAX_CV, MAX_RANGE);
        int stepCv = Proportion(In(1), HEMISPHERE_MAX_CV, MAX_STEP);
        
        ForEachChannel(ch) {
            // OUTPUT
            if ( ((ch == 0) && Clock(0)) ||
            ((ch == 1) && Clock(yClkSrc)) )
            {
                if ((ch == 1) && ((clkMod++ % yClkDiv) > 0) ){
                    continue;
                }
                int randInt = random(0, 1000);
                int randStep = (float)(random(1, constrain(step+stepCv, 0, MAX_STEP)))/MAX_STEP*maxVal/2;
                int rangeScaled = (int)( ((float)constrain(range + rangeCv, 0, MAX_RANGE))/MAX_RANGE * maxVal);
                currentVal[ch] += randStep * (((randInt > PROB_UP) && (currentVal[ch] < rangeScaled)) -
                                              ((randInt < PROB_DN) && (currentVal[ch] > -rangeScaled)));
            }
            currentOut[ch] = alpha*currentOut[ch] + (1-alpha)*(float)currentVal[ch];

            Out(ch, constrain((int)currentOut[ch], -HEMISPHERE_3V_CV, HEMISPHERE_MAX_CV));
        }
    }

    void View() {
        gfxHeader(applet_name());
        DrawDisplay();
    }

    void OnButtonPress() {
        cursor++;
        if (cursor > 5) cursor = 0;
        ResetCursor();
    }

    void OnEncoderMove(int direction) {
        // Parameter Change handler
        // var cursor is the param pointer
        // var direction is the the movement of the encoder
        // use valConstrained = constrain(val, min, max) to apply value limit
        if (cursor == 0) {
            range = constrain(range + direction, 0, MAX_RANGE);
        } else if (cursor == 1) {
            step = constrain(step + direction, 1, MAX_STEP);
        } else if (cursor == 2) {
            smoothness = constrain(smoothness + direction, 0, MAX_SMOOTH);
            UpdateAlpha();
        } else if (cursor == 3) {
            yClkSrc = constrain(yClkSrc + direction, 0, 1);
        } else if (cursor == 4) {
            yClkDiv = constrain(yClkDiv + direction, 1, 32);
        } else if (cursor == 5) {
            cvRange = constrain(cvRange + direction, 0, 3);
            int maxVal = HEMISPHERE_MAX_CV;
            switch (cvRange) {
                case 0:
                    maxVal = HEM_HST;
                    break;
                case 1:
                    maxVal = HEM_1ST;
                    break;
                case 2:
                    maxVal = HEM_1V;
                    break;
                default:
                    break;
            }
            ForEachChannel(ch) {
                currentVal[ch] = currentVal[ch]%maxVal;
            }
        }

    }

    uint32_t OnDataRequest() {
        uint32_t data = 0;
        Pack(data, PackLocation {0,1}, yClkSrc);
        Pack(data, PackLocation {1,4}, yClkDiv);
        Pack(data, PackLocation {5,8}, range);
        Pack(data, PackLocation {13,8}, step);
        Pack(data, PackLocation {21,8}, smoothness);
        Pack(data, PackLocation {29,2}, cvRange);
        return data;
    }

    void OnDataReceive(uint32_t data) {
        yClkSrc = Unpack(data, PackLocation {0,1});
        yClkDiv = Unpack(data, PackLocation {1,4});
        range = Unpack(data, PackLocation {5,8});
        step = Unpack(data, PackLocation {13,8});
        smoothness = Unpack(data, PackLocation {21,8});
        cvRange = Unpack(data, PackLocation {29,2});
        UpdateAlpha();
    }

protected:
    void SetHelp() {
        //                               "------------------" <-- Size Guide
        help[HEMISPHERE_HELP_DIGITALS] = "1=X Clock 2=Y Clk";
        help[HEMISPHERE_HELP_CVS]      = "1=Range 2=step";
        help[HEMISPHERE_HELP_OUTS]     = "RndWalk A=X B=Y";
        help[HEMISPHERE_HELP_ENCODER]  = "T=Set P=Select";
        //                               "------------------" <-- Size Guide
    }
    
private:
    // Parameters (saved in EEPROM)
    bool yClkSrc = 0; // 0=TR1, 1=TR2
    uint8_t yClkDiv = 1; // 4 bits [1 .. 32]
    int range = 20; // 8 bits
    int step = 20; // 8 bits
    uint8_t smoothness = 20; // 8 bits
    uint8_t cvRange = 3; // 2 bit
    uint8_t clkMod = 0; //not stored, used for clock division
    float alpha; // not stored, used for smoothing

    // Runtime parameters
    // unsigned int rndSeed[2];
    int currentVal[2];
    float currentOut[2];
    int cursor; // 0=Y clk src, 1=Y clk div, 2=Range,  3=step, 4=Smoothnes
    
    void DrawDisplay() {

        if (cursor < 3) {
            gfxPrint(1, 15, "Range");
            gfxPrint(43, 15, range);
            if (cursor == 0) gfxCursor(43, 22, 18);

            gfxPrint(1, 25, "Step");
            gfxPrint(43, 25, step);
            if (cursor == 1) gfxCursor(43, 32, 18);
            gfxPrint(1, 35, "Smooth");
            gfxPrint(43, 35, smoothness);
            if (cursor == 2) gfxCursor(43, 42, 18);
        } else {

            gfxPrint(1, 15, "Y TRIG");
            if (yClkSrc == 0) {
                gfxPrint(43, 15, "TR1");
            } else {
                gfxPrint(43, 15, "TR2");
            }
            if (cursor == 3) gfxCursor(43, 22, 18);
    
            gfxPrint(1, 25, "Y CLK /");
            gfxPrint(43, 25, yClkDiv);
            if (cursor == 4) gfxCursor(43, 32, 18);

            gfxPrint(1, 35, "CV Rng");
            if (cvRange == 0) {
                gfxPrint(43, 35, ".5st");
            } else if (cvRange == 1) {
                gfxPrint(43, 35, "1 st");
            } else if (cvRange == 2) {
                gfxPrint(43, 35, "1oct");
            } else if (cvRange == 3) {
                gfxPrint(43, 35, "FULL");
            }
            if (cursor == 5) gfxCursor(43, 42, 24);
        }

        // gfxPrint(1, 38, "x");
        // gfxPrint(1, 50, "y");

        // gfxPrint(7, 38, currentVal[0]);
        // gfxPrint(7, 50, currentVal[1]);

        int maxVal = HEMISPHERE_MAX_CV;
        switch (cvRange) {
            case 0:
                maxVal = HEM_HST;
                break;
            case 1:
                maxVal = HEM_1ST;
                break;
            case 2:
                maxVal = HEM_1V;
                break;
            default:
                break;
        }
        // gfxPrint(1, 47, currentVal[0]);
        // gfxPrint(1, 55, currentVal[1]);
        gfxPrint(1, 47, "x");
        gfxPrint(55, 55, "y");
        ForEachChannel(ch) {
            int w = 0;
            if (range > 0) {
                w = (currentOut[ch]/((float)range/MAX_RANGE*maxVal))*31;
                if (w > 31) {
                    w = 31;
                }
                if (w < -31) {
                    w = -31;
                }
            }
            if (w >= 0) {
                gfxInvert(31, 48 + (8 * ch), w, 7);
            } else {
                gfxInvert(31+w, 48 + (8 * ch), -w, 7);
            }
        }
        
    }

    void UpdateAlpha() {

        // switch (smoothness) {
        //     case 90:
        //         alpha = 0.95;
        //         break;
        //     case 91:
        //         alpha = 0.99;
        //         break;
        //     case 92:
        //         alpha = 0.999;
        //         break;
        //     case 93:
        //         alpha = 0.9999;
        //         break;
        //     case 94:
        //         alpha = 0.99999;
        //         break;
        //     case 95:
        //         alpha = 0.999999;
        //         break;
        //     case 96:
        //         alpha = 0.9999999;
        //         break;
        //     case 97:
        //         alpha = 0.99999999;
        //         break;
        //     case 98:
        //         alpha = 0.999999999;
        //         break;
        //     case 99:
        //         alpha = 0.9999999999;
        //         break;
        //     default:
        //         alpha = (float)smoothness/100;
        // }
        alpha = log(1+smoothness)/log(1+MAX_SMOOTH);
        // alpha = (float)smoothness/(float)MAX_SMOOTH;
    }
};


////////////////////////////////////////////////////////////////////////////////
//// Hemisphere Applet Functions
///
///  Once you run the find-and-replace to make these refer to RndWalk,
///  it's usually not necessary to do anything with these functions. You
///  should prefer to handle things in the HemisphereApplet child class
///  above.
////////////////////////////////////////////////////////////////////////////////
RndWalk RndWalk_instance[2];

void RndWalk_Start(bool hemisphere) {
    RndWalk_instance[hemisphere].BaseStart(hemisphere);
}

void RndWalk_Controller(bool hemisphere, bool forwarding) {
    RndWalk_instance[hemisphere].BaseController(forwarding);
}

void RndWalk_View(bool hemisphere) {
    RndWalk_instance[hemisphere].BaseView();
}

void RndWalk_OnButtonPress(bool hemisphere) {
    RndWalk_instance[hemisphere].OnButtonPress();
}

void RndWalk_OnEncoderMove(bool hemisphere, int direction) {
    RndWalk_instance[hemisphere].OnEncoderMove(direction);
}

void RndWalk_ToggleHelpScreen(bool hemisphere) {
    RndWalk_instance[hemisphere].HelpScreen();
}

uint32_t RndWalk_OnDataRequest(bool hemisphere) {
    return RndWalk_instance[hemisphere].OnDataRequest();
}

void RndWalk_OnDataReceive(bool hemisphere, uint32_t data) {
    RndWalk_instance[hemisphere].OnDataReceive(data);
}