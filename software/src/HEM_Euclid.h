// Copyright (c) 2022, Alessio Degani
// Copyright (c) 2018, Jason Justian
//
// Bjorklund pattern filter, Copyright (c) 2016 Tim Churches
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
#include "bjorklund.h"

#define PAD_LEFT 5
#define SPACING 16

class Euclid : public HemisphereApplet {
public:

    const char* applet_name() {
        return "Euclid";
    }

    void Start() {

        ForEachChannel(ch)
        {
            length[ch] = 4;
            beats[ch] = 1;
            pattern[ch] = EuclideanPattern(length[ch] - 1, beats[ch], 0);;
        }
        step = 0;
    }

    void Controller() {
        if (Clock(1)) step = 0; // Reset

        // Advance both rings
        if (Clock(0) && !Gate(1)) {

            int cv_data[2];

            cv_data[0] = DetentedIn(0);
            cv_data[1] = DetentedIn(1);
            ForEachChannel(ch)
            {
                actual_length[ch] = length[ch];
                actual_rotation[ch] = rotation[ch];
                actual_beats[ch] = beats[ch];
                for (uint8_t ch_src = 0; ch_src <= 1; ch_src++) {
                    if (cv_dest[ch_src] == 0+3*ch) {
                        actual_length[ch] = (uint8_t)constrain(actual_length[ch] + Proportion(cv_data[ch_src], HEMISPHERE_MAX_CV, 29), 3, 32);
                        // if (actual_beats[ch]  > actual_length[ch]) actual_beats[ch]  = actual_length[ch];
                        // if (actual_rotation[ch] > actual_length[ch]-1) actual_rotation[ch] = actual_length[ch]-1;
                    }
                    if (cv_dest[ch_src] == 1+3*ch) {
                        actual_beats[ch] = (uint8_t)constrain(actual_beats[ch] + Proportion(cv_data[ch_src], HEMISPHERE_MAX_CV, actual_length[ch]), 1, actual_length[ch]);
                    }
                    if (cv_dest[ch_src] == 2+3*ch) {
                        actual_rotation[ch] = (uint8_t)constrain(actual_rotation[ch] + Proportion(cv_data[ch_src], HEMISPHERE_MAX_CV, actual_length[ch]), 0, actual_length[ch]-1);
                    }
                }

                // Store the pattern for display
                pattern[ch] = EuclideanPattern(actual_length[ch]-1, actual_beats[ch], actual_rotation[ch]);
                int sb = step % actual_length[ch];
                if ((pattern[ch] >> sb) & 0x01) {
                    ClockOut(ch);
                }
            }

            // Plan for the thing to run forever and ever
            if (++step >= actual_length[0] * actual_length[1]) step = 0;
        }
    }

    void View() {
        gfxHeader(applet_name());
        DrawSteps();
        DrawEditor();
    }

    void OnButtonPress() {
        if (++cursor > 7) cursor = 0;
        ResetCursor();
    }

    void OnEncoderMove(int direction) {
        int ch = cursor < 4 ? 0 : 1;
        int f = cursor - (ch * 4); // Cursor function
        switch(f) {
            case 0:
                length[ch] = constrain(length[ch] + direction, 3, 32);
                if (beats[ch] > length[ch]) beats[ch] = length[ch];
                if (rotation[ch] > length[ch]-1) rotation[ch] = length[ch]-1;
                // SetDisplayPositions(ch, 24 - (8 * ch));
                break;
            case 1:
                beats[ch] = constrain(beats[ch] + direction, 1, length[ch]);
                break;
            case 2:
                rotation[ch] = constrain(rotation[ch] + direction, 0, length[ch]-1);
                break;
            case 3:
                cv_dest[ch] = constrain(cv_dest[ch] + direction, 0, 5);
                break;
            default:
                break;
        }
        actual_length[ch] = length[ch];
        actual_rotation[ch] = rotation[ch];
        actual_beats[ch] = beats[ch];
    }

    uint32_t OnDataRequest() {
        uint32_t data = 0;
        Pack(data, PackLocation {0,4}, length[0] - 1);
        Pack(data, PackLocation {4,4}, beats[0] - 1);
        Pack(data, PackLocation {8,4}, rotation[0]);
        Pack(data, PackLocation {12,4}, length[1] - 1);
        Pack(data, PackLocation {16,4}, beats[1] - 1);
        Pack(data, PackLocation {20,4}, rotation[1]);
        Pack(data, PackLocation {24,4}, cv_dest[0]);
        Pack(data, PackLocation {28,4}, cv_dest[1]);
        return data;
    }

    void OnDataReceive(uint32_t data) {
        ForEachChannel(ch) {
            length[ch] = Unpack(data, PackLocation {0+12*ch,4}) + 1;
            beats[ch] = Unpack(data, PackLocation {4+12*ch,4}) + 1;
            rotation[ch] = Unpack(data, PackLocation {8+12*ch,4});
            cv_dest[ch] = Unpack(data, PackLocation {24+4*ch,4});
            actual_length[ch] = length[ch];
            actual_beats[ch] = beats[ch];
            actual_rotation[ch] = rotation[ch];
        }
        // length[0] = Unpack(data, PackLocation {0,4}) + 1;
        // beats[0] = Unpack(data, PackLocation {4,4}) + 1;
        // rotation[0] = Unpack(data, PackLocation {8,4});
        // length[1] = Unpack(data, PackLocation {12,4}) + 1;
        // beats[1] = Unpack(data, PackLocation {16,4}) + 1;
        // rotation[1] = Unpack(data, PackLocation {20,4});
        // cv_dest[0] = Unpack(data, PackLocation {24,4});
        // cv_dest[1] = Unpack(data, PackLocation {28,4});
        // actual_length[0] = length[0];
        // actual_beats[0] = beats[0];
        // actual_rotation[0] = rotation[0];
        // actual_length[1] = length[1];
        // actual_beats[1] = beats[1];
        // actual_rotation[1] = rotation[1];
    }

protected:
    void SetHelp() {
        //                               "------------------" <-- Size Guide
        help[HEMISPHERE_HELP_DIGITALS] = "1=Clock 2=Reset";
        help[HEMISPHERE_HELP_CVS]      = "Rotate 1=Ch1 2=Ch2";
        help[HEMISPHERE_HELP_OUTS]     = "Clock A=Ch1 B=Ch2";
        help[HEMISPHERE_HELP_ENCODER]  = "Length/Hits Ch1,2";
        //                               "------------------" <-- Size Guide
    }

private:
    int step;
    uint8_t cursor = 0; // Ch1: 0=Length, 1=Hits, 2=Rotation; Ch2: 3=Length, 4=Hits, 5=Rotation
    // AFStepCoord disp_coord[2][32];
    uint32_t pattern[2];

    // Settings
    uint8_t length[2];
    uint8_t beats[2];
    uint8_t rotation[2];
    uint8_t actual_length[2];
    uint8_t actual_beats[2];
    uint8_t actual_rotation[2];
    uint8_t cv_dest[2];
    int cv_data[2];

    void DrawSteps() {
        gfxLine(0, 45, 63, 45);
        gfxLine(0, 62, 63, 62);
        gfxLine(0, 53, 63, 53);
        gfxLine(0, 54, 63, 54);
        ForEachChannel(ch) {
            for (int i = 0; i < 16; i++) {
                if ((pattern[ch] >> ((i + step) % (actual_length[ch]) )) & 0x1) {
                    gfxRect(4 * i + 1, 48 + 9 * ch, 3, 3);
                    //gfxLine(4 * i + 2, 47 + 9 * ch, 4 * i + 2, 47 + 9 * ch + 4);
                } else {
                    gfxPixel(4 * i + 2, 47 + 9 * ch + 2);
                }

                if ((i + step) % (actual_length[ch]) == 0) {
                    //gfxLine(4 * i, 46 + 9 * ch, 4 * i, 52 + 9 * ch);
                    gfxLine(4 * i, 46 + 9 * ch, 4 * i, 46 + 9 * ch + 1);
                    gfxLine(4 * i, 52 + 9 * ch - 1, 4 * i, 52 + 9 * ch);
                }
            }
        }
    }

    void DrawEditor() {
        int f = 0;

        ForEachChannel(ch)
        {
            f = cursor - (ch * 4); // Cursor function

            // Length cursor
            gfxPrint(PAD_LEFT + 0*SPACING, 25+10*ch, actual_length[ch]);
            if (f == 0) gfxCursor(PAD_LEFT + 0*SPACING, 33+10*ch, 12);
            for (int ch_dest = 0; ch_dest < 2; ch_dest++){
                if (cv_dest[ch_dest] == 0+3*ch) gfxBitmap(PAD_LEFT + 0*SPACING+10, 23+10*ch, 3, ch_dest?SUB_TWO:SUP_ONE);
            }

            // Beats cursor
            gfxPrint(PAD_LEFT + 1*SPACING, 25+10*ch, actual_beats[ch]);
            if (f == 1) gfxCursor(PAD_LEFT + 1*SPACING, 33+10*ch, 12);
            for (int ch_dest = 0; ch_dest < 2; ch_dest++){
                if (cv_dest[ch_dest] == 1+3*ch) gfxBitmap(PAD_LEFT + 1*SPACING+10, 23+10*ch, 3, ch_dest?SUB_TWO:SUP_ONE);
            }

            // Rotation cursor
            gfxPrint(PAD_LEFT + 2*SPACING, 25+10*ch, actual_rotation[ch]);
            if (f == 2) gfxCursor(PAD_LEFT + 2*SPACING, 33+10*ch, 12);
            for (int ch_dest = 0; ch_dest < 2; ch_dest++){
                if (cv_dest[ch_dest] == 2+3*ch) gfxBitmap(PAD_LEFT + 2*SPACING+10, 23+10*ch, 3, ch_dest?SUB_TWO:SUP_ONE);
            }

            // CV destination
            gfxPrint(PAD_LEFT + 3*SPACING, 25+10*ch, cv_dest[ch]);
            // if (f == 3) gfxCursor(13 + 24*ch, 53, 12);
            if (f == 3) gfxCursor(PAD_LEFT + 3*SPACING, 33+10*ch, 12);

            // int curr_cv = 0;
            // for (int ch_src = 0; ch_src < 2; ch_src++) {
            //     if (cv_dest[ch] == 0+3*ch_src) curr_cv = length[ch_src] + cv_data[ch];
            //     if (cv_dest[ch] == 1+3*ch_src) curr_cv = beats[ch_src] + cv_data[ch];
            //     if (cv_dest[ch] == 2+3*ch_src) curr_cv = rotation[ch_src] + cv_data[ch];
            // }
            // gfxPrint(12 + 24*ch + pad(10, pattern[ch]), 55, pattern[ch]);
        }

        gfxBitmap(PAD_LEFT + 0*SPACING, 15, 8, LEFT_RIGHT_ICON);
        gfxBitmap(PAD_LEFT + 1*SPACING, 15, 8, X_NOTE_ICON);
        gfxBitmap(PAD_LEFT + 2*SPACING, 15, 8, LOOP_ICON);
        gfxBitmap(PAD_LEFT + 3*SPACING, 15, 8, CV_ICON);
        // gfxLine(34, 15, 34, 55);
    }
};


////////////////////////////////////////////////////////////////////////////////
//// Hemisphere Applet Functions
///
///  Once you run the find-and-replace to make these refer to Euclid,
///  it's usually not necessary to do anything with these functions. You
///  should prefer to handle things in the HemisphereApplet child class
///  above.
////////////////////////////////////////////////////////////////////////////////
Euclid Euclid_instance[2];

void Euclid_Start(bool hemisphere) {
    Euclid_instance[hemisphere].BaseStart(hemisphere);
}

void Euclid_Controller(bool hemisphere, bool forwarding) {
    Euclid_instance[hemisphere].BaseController(forwarding);
}

void Euclid_View(bool hemisphere) {
    Euclid_instance[hemisphere].BaseView();
}

void Euclid_OnButtonPress(bool hemisphere) {
    Euclid_instance[hemisphere].OnButtonPress();
}

void Euclid_OnEncoderMove(bool hemisphere, int direction) {
    Euclid_instance[hemisphere].OnEncoderMove(direction);
}

void Euclid_ToggleHelpScreen(bool hemisphere) {
    Euclid_instance[hemisphere].HelpScreen();
}

uint32_t Euclid_OnDataRequest(bool hemisphere) {
    return Euclid_instance[hemisphere].OnDataRequest();
}

void Euclid_OnDataReceive(bool hemisphere, uint32_t data) {
    Euclid_instance[hemisphere].OnDataReceive(data);
}