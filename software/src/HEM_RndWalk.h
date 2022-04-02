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

#define RANGE 200
#define STEP 20
#define MAX_INT_VAL 65532

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
            range = RANGE;
            step = STEP;
        }
        cursor = 0;
    }

    void Controller() {
        float alpha = (float)smoothness/100;
        // Main LOOP
        // for triggers read from Clock(0|1)
        // for CV in read from In(0|1)
        // for CV out write to Out(0|1, value)
        ForEachChannel(ch)
        {
            if ((Clock(ch) || MasterClockForwarded()) || 
                ((ch == 1) && (yClkSrc == 0) && (Clock(0)))) {
                
                int randInt = random(0, 1000);
                int randStep = random(1, step)*10;
                int rangeScaled = (int)( ((float)range)/100.0 * HEMISPHERE_MAX_CV );
                currentVal[ch] += randStep * (((randInt > PROB_UP) && (currentVal[ch] < rangeScaled)) -
                                              ((randInt < PROB_DN) && (currentVal[ch] > -rangeScaled)));

                // outVal[ch] = Proportion(currentVal[ch], MAX_INT_VAL, 2*range);
                // outVal[ch] = (int)((float)currentVal[ch] * ((float)range/(float)MAX_INT_VAL));
                // outVal[ch] = outVal[ch] - range + step;
                // HEMISPHERE_MAX_CV


            }

            // gfxCircle(x, y, r);
            // gfxLine(x1, y1, x2, y2);

            currentOut[ch] = alpha*currentOut[ch] + (1-alpha)*(float)currentVal[ch];
            Out(ch, constrain((int)currentOut[ch], -HEMISPHERE_MAX_CV, HEMISPHERE_MAX_CV));
        }
    }

    void View() {
        gfxHeader(applet_name());
        DrawDisplay();
    }

    void OnButtonPress() {
        cursor++;
        if (cursor > 4) cursor = 0;
        ResetCursor();
    }

    void OnEncoderMove(int direction) {
        // Parameter Change handler
        // var cursor is the param pointer
        // var direction is the the movement of the encoder
        // use valConstrained = constrain(val, min, max) to apply value limit
        if (cursor == 0) {
            range = constrain(range + direction, 0, 100);
        } else if (cursor == 1) {
            step = constrain(step + direction, 0, 100);
        } else if (cursor == 2) {
            smoothness = constrain(smoothness + direction, 0, 100);
        } else if (cursor == 3) {
            yClkSrc = constrain(yClkSrc + direction, 0, 1);
        } else if (cursor == 4) {
            yClkDiv = constrain(yClkDiv + direction, 1, 32);
        }

    }

    uint32_t OnDataRequest() {
        uint32_t data = 0;
        Pack(data, PackLocation {0,1}, yClkSrc);
        Pack(data, PackLocation {1,4}, yClkDiv);
        Pack(data, PackLocation {5,10}, range);
        Pack(data, PackLocation {15,10}, step);
        Pack(data, PackLocation {25,7}, smoothness);
        return data;
    }

    void OnDataReceive(uint32_t data) {
        yClkSrc = Unpack(data, PackLocation {0,1});
        yClkDiv = Unpack(data, PackLocation {1,4});
        range = Unpack(data, PackLocation {5,10});
        step = Unpack(data, PackLocation {15,10});
        smoothness = Unpack(data, PackLocation {25,7});
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
    bool yClkSrc; // 0=TR1, 1=TR2
    uint8_t yClkDiv; // 4 bits [1 .. 32]
    uint16_t range; // 10 bits
    uint16_t step; // 10 bits
    uint8_t smoothness; // 7 bits

    // Runtime parameters
    // unsigned int rndSeed[2];
    int currentVal[2];
    float currentOut[2];
    int cursor; // 0=Y clk src, 1=Y clk div, 2=Range,  3=step, 4=Smoothnes
    
    void DrawDisplay() {

        if (cursor < 2) {
            gfxPrint(1, 15, "Range");
            gfxPrint(41, 15, range);
            if (cursor == 0) gfxCursor(41, 22, 18);

            gfxPrint(1, 25, "Step");
            gfxPrint(41, 25, step);
            if (cursor == 1) gfxCursor(41, 32, 18);
        } else if (cursor < 4) {
            gfxPrint(1, 15, "Smooth");
            gfxPrint(41, 15, smoothness);
            if (cursor == 2) gfxCursor(41, 22, 18);

            gfxPrint(1, 25, "Y TR");
            if (yClkSrc == 0) {
                gfxPrint(41, 25, "TR1");
            } else {
                gfxPrint(41, 25, "TR2");
            }
            if (cursor == 3) gfxCursor(41, 32, 18);
        } else {
            gfxPrint(1, 15, "Y DIV");
            gfxPrint(41, 15, yClkDiv);
            if (cursor == 4) gfxCursor(41, 22, 18);
        }

        gfxPrint(1, 38, "x");
        gfxPrint(1, 50, "y");

        gfxPrint(7, 38, currentVal[0]);
        gfxPrint(7, 50, currentVal[1]);

        // ForEachChannel(ch)
        // {
        //     int w = ProportionCV(ViewOut(ch)/HEMISPHERE_MAX_CV*range, 62);
        //     gfxInvert(1, 38 + (12 * ch), w, 10);
        // }
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
