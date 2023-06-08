// Copyright (c) 2018, Jason Justian
//
// Based on Braids Quantizer, Copyright 2015 Émilie Gillet.
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

/*
 * Turing Machine based on https://thonk.co.uk/documents/random%20sequencer%20documentation%20v2-1%20THONK%20KIT_LargeImages.pdf
 *
 * Thanks to Tom Whitwell for creating the concept, and for clarifying some things
 * Thanks to Jon Wheeler for the CV length and probability updates
 */
#include "hemisphere/applet_base.hpp"
#include "braids/quantizer.h"
#include "braids/quantizer_scales.h"
#include "oc/scales.h"

#define TM_MAX_SCALE oc::Scales::NUM_SCALES

#define TM_MIN_LENGTH 2
#define TM_MAX_LENGTH 16

using namespace hemisphere;

class TM : public AppletBase {
public:

    const char* applet_name() {
        return "ShiftReg";
    }

    void Start() {
        reg = random(0, 65535);
        p = 0;
        length = 16;
        quant_range = 24;  //APD: Quantizer range
        cursor = 0;
        quantizer.Init();
        scale = oc::Scales::SCALE_SEMI;
        quantizer.Configure(oc::Scales::GetScale(scale), 0xffff); // Semi-tone
    }

    void Controller() {

        // CV 1 control over length
        int lengthCv = DetentedIn(0);
        if (lengthCv < 0) length = TM_MIN_LENGTH;        
        if (lengthCv > 0) {
            length = constrain(ProportionCV(lengthCv, TM_MAX_LENGTH + 1), TM_MIN_LENGTH, TM_MAX_LENGTH);
        }
      
        // CV 2 bi-polar modulation of probability
        int pCv = Proportion(DetentedIn(1), HEMISPHERE_MAX_CV, 100);
        bool clk = Clock(0);
        
        if (clk) {
            // If the cursor is not on the p value, and Digital 2 is not gated, the sequence remains the same
            int prob = (cursor == 1 || Gate(1)) ? p + pCv : 0;

            AdvanceRegister( constrain(prob, 0, 100) );
        }
 
        // Send 5-bit quantized CV
        // APD: Scale this to the range of notes allowed by quant_range: 32 should be all
        // This defies the faithful Turing Machine sim aspect of this code but gives a useful addition that the Disting adds to the concept
        int32_t note = Proportion(reg & 0x1f, 0x1f, quant_range);
        Out(0, quantizer.Lookup(note + 64));

        switch (cv2) {
        case 0:
          // Send 8-bit proportioned CV
          Out(1, Proportion(reg & 0x00ff, 255, HEMISPHERE_MAX_CV) );
          break;
        case 1:
          if (clk)
            ClockOut(1);
          break;
        case 2:
          // only trigger if 1st bit is high
          if (clk && (reg & 0x01) == 1)
            ClockOut(1);
          break;
        case 3: // duplicate of Out A
          Out(1, quantizer.Lookup(note + 64));
          break;
        case 4: // alternative 6-bit pitch
          note = Proportion( (reg >> 8 & 0x3f), 0x3f, quant_range);
          Out(1, quantizer.Lookup(note + 64));
          break;
        }
    }

    void View() {
        gfxHeader(applet_name());
        DrawSelector();
        DrawIndicator();
    }

    void OnButtonPress() {
        isEditing = !isEditing;
    }

    void OnEncoderMove(int direction) {
        if (!isEditing) {
            cursor += direction;
            if (cursor < 0) cursor = 4;
            if (cursor > 4) cursor = 0;

            ResetCursor();  // Reset blink so it's immediately visible when moved
        } else {
            switch (cursor) {
            case 0:
                length = constrain(length + direction, TM_MIN_LENGTH, TM_MAX_LENGTH);
                break;
            case 1:
                p = constrain(p + direction, 0, 100);
                break;
            case 2:
                scale += direction;
                if (scale >= TM_MAX_SCALE) scale = 0;
                if (scale < 0) scale = TM_MAX_SCALE - 1;
                quantizer.Configure(oc::Scales::GetScale(scale), 0xffff);
                break;
            case 3:
                quant_range = constrain(quant_range + direction, 1, 32);
                break;
            case 4:
                cv2 = constrain(cv2 + direction, 0, 4);
                break;
            }
        }
    }
        
    uint64_t OnDataRequest() {
        uint64_t data = 0;
        Pack(data, PackLocation {0,16}, reg);
        Pack(data, PackLocation {16,7}, p);
        Pack(data, PackLocation {23,4}, length - 1);
        Pack(data, PackLocation {27,5}, quant_range - 1);
        Pack(data, PackLocation {32,4}, cv2);
        Pack(data, PackLocation {36,8}, constrain(scale, 0, 255));

        return data;
    }

    void OnDataReceive(uint64_t data) {
        reg = Unpack(data, PackLocation {0,16});
        p = Unpack(data, PackLocation {16,7});
        length = Unpack(data, PackLocation {23,4}) + 1;
        quant_range = Unpack(data, PackLocation{27,5}) + 1;
        cv2 = Unpack(data, PackLocation {32,4});
        scale = Unpack(data, PackLocation {36,8});
        quantizer.Configure(oc::Scales::GetScale(scale), 0xffff);
    }

protected:
    void SetHelp() {
        //                               "------------------" <-- Size Guide
        help[HEMISPHERE_HELP_DIGITALS] = "1=Clock 2=p Gate";
        help[HEMISPHERE_HELP_CVS]      = "1=Length 2=p Mod";
        help[HEMISPHERE_HELP_OUTS]     = "A=Quant5-bit B=CV2";
        help[HEMISPHERE_HELP_ENCODER]  = "Len/Prob/Scl/Range";
        //                               "------------------" <-- Size Guide
    }
    
private:
    int length; // Sequence length
    int cursor;  // 0 = length, 1 = p, 2 = scale
    bool isEditing = false;
    braids::Quantizer quantizer;

    // Settings
    uint16_t reg; // 16-bit sequence register
    int p; // Probability of bit 15 changing on each cycle
    //int8_t scale; // Scale used for quantized output
    int scale;  // Logarhythm: hold larger values
    //int tmp = 0;
    uint8_t quant_range;  // APD
    uint8_t cv2 = 0; // 2nd output mode: 0=mod; 1=trig; 2=trig-on-msb; 3=duplicate of A; 4=alternate pitch

    void DrawSelector() {
        gfxBitmap(1, 14, 8, LOOP_ICON);
        gfxPrint(12 + pad(10, length), 15, length);
        gfxPrint(32, 15, "p=");
        if (cursor == 1 || Gate(1)) { // p unlocked
            int pCv = Proportion(DetentedIn(1), HEMISPHERE_MAX_CV, 100);
            int prob = constrain(p + pCv, 0, 100);
            gfxPrint(pad(100, prob), prob);
        } else { // p is disabled
            gfxBitmap(49, 14, 8, LOCK_ICON);
        }
        gfxBitmap(1, 24, 8, SCALE_ICON);
        gfxPrint(12, 25, oc::scale_names_short[scale]);
        gfxBitmap(41, 24, 8, NOTE4_ICON);
        gfxPrint(49, 25, quant_range); // APD
        gfxPrint(1, 35, "CV2:");
        switch (cv2) {
        case 0: // modulation output
            gfxBitmap(28, 35, 8, WAVEFORM_ICON);
            break;
        case 2: // clock out only on msb
            gfxPrint(36, 35, "1");
        case 1: // clock out icon
            gfxBitmap(28, 35, 8, CLOCK_ICON);
            break;
        case 3: // double output A
            gfxBitmap(28, 35, 8, LINK_ICON);
            break;
        case 4: // alternate 6-bit pitch
            gfxBitmap(28, 35, 8, CV_ICON);
            break;
        }

        //gfxPrint(1, 35, tmp);
        switch (cursor) {
            case 0: gfxCursor(13, 23, 12); break; // Length Cursor
            case 1: gfxCursor(45, 23, 18); break; // Probability Cursor
            case 2: gfxCursor(12, 33, 25); break; // Scale Cursor
            case 3: gfxCursor(49, 33, 14); break; // Quant Range Cursor // APD
            case 4: gfxCursor(27, 43, (cv2 == 2) ? 18 : 10); // cv2 mode
        }
        if (isEditing) {
            switch (cursor) {
                case 0: gfxInvert(13, 14, 12, 9); break;
                case 1: gfxInvert(45, 14, 18, 9); break;
                case 2: gfxInvert(12, 24, 25, 9); break;
                case 3: gfxInvert(49, 24, 14, 9); break;
                case 4: gfxInvert(27, 34, (cv2 == 2) ? 18 : 10, 9); // cv2 mode
            }
        }
    }

    void DrawIndicator() {
        gfxLine(0, 45, 63, 45);
        gfxLine(0, 62, 63, 62);
        for (int b = 0; b < 16; b++)
        {
            int v = (reg >> b) & 0x01;
            if (v) gfxRect(60 - (4 * b), 47, 3, 14);
        }
    }

    void AdvanceRegister(int prob) {
        // Before shifting, determine the fate of the last bit
        int last = (reg >> (length - 1)) & 0x01;
        if (random(0, 99) < prob) last = 1 - last;

        // Shift left, then potentially add the bit from the other side
        reg = (reg << 1) + last;
    }

};


////////////////////////////////////////////////////////////////////////////////
//// Hemisphere Applet Functions
///
///  Once you run the find-and-replace to make these refer to TM,
///  it's usually not necessary to do anything with these functions. You
///  should prefer to handle things in the HemisphereApplet child class
///  above.
////////////////////////////////////////////////////////////////////////////////
TM TM_instance[2];

void TM_Start(bool hemisphere) {
    TM_instance[hemisphere].BaseStart(hemisphere);
}

void TM_Controller(bool hemisphere, bool forwarding) {
    TM_instance[hemisphere].BaseController(forwarding);
}

void TM_View(bool hemisphere) {
    TM_instance[hemisphere].BaseView();
}

void TM_OnButtonPress(bool hemisphere) {
    TM_instance[hemisphere].OnButtonPress();
}

void TM_OnEncoderMove(bool hemisphere, int direction) {
    TM_instance[hemisphere].OnEncoderMove(direction);
}

void TM_ToggleHelpScreen(bool hemisphere) {
    TM_instance[hemisphere].HelpScreen();
}

uint64_t TM_OnDataRequest(bool hemisphere) {
    return TM_instance[hemisphere].OnDataRequest();
}

void TM_OnDataReceive(bool hemisphere, uint64_t data) {
    TM_instance[hemisphere].OnDataReceive(data);
}
