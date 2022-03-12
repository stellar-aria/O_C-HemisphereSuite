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
            range = RANGE;
            step = STEP;
        }
        cursor = 0;
    }

    void Controller() {
        // Main LOOP
        // for triggers read from Clock(0|1)
        // for CV in read from In(0|1)
        // for CV out write to Out(0|1, value)
        ForEachChannel(ch)
        {
            if (Clock(ch) || MasterClockForwarded()) {
                int randInt = random(0, 1000);
                int randStep = random(1, step);
                currentVal[ch] += randStep * (((randInt > PROB_UP) && (currentVal[ch] < range)) -
                                              ((randInt < PROB_DN) && (currentVal[ch] > -range)));

                // outVal[ch] = Proportion(currentVal[ch], MAX_INT_VAL, 2*range);
                // outVal[ch] = (int)((float)currentVal[ch] * ((float)range/(float)MAX_INT_VAL));
                // outVal[ch] = outVal[ch] - range + step;

                outVal[ch] = currentVal[ch];

                // Out(ch, constrain(outVal[ch], -HEMISPHERE_MAX_CV, HEMISPHERE_MAX_CV));
                // HEMISPHERE_MAX_CV


            }
            // gfxCircle(x, y, r);
            // gfxLine(x1, y1, x2, y2);
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
    int outVal[2];
    int cursor; // 0=Y clk src, 1=Y clk div, 2=Range,  3=step, 4=Smoothnes
    
    void DrawDisplay() {
        gfxPrint(1, 38, "x");
        gfxPrint(1, 50, "y");
        // ForEachChannel(ch)
        // {
            gfxPrint(7, 38, currentVal[0]);
            gfxPrint(7, 50, currentVal[1]);
            // gfxCircle(x, y, r);
            // gfxLine(x1, y1, x2, y2);
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
