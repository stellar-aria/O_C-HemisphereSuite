// Copyright (c) 2018, Jason Justian
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

#define HEM_ENV_FOLLOWER_SAMPLES 166
#define HEM_ENV_FOLLOWER_MAXSPEED 16

class EnvFollow : public HemisphereApplet {
public:

    enum EnvFollowCursor {
        MODE_1, MODE_2,
        GAIN_1, GAIN_2,
        SPEED,
        MAX_CURSOR = SPEED
    };

    const char* applet_name() {
        return "EnvFollow";
    }

    void Start() {
        ForEachChannel(ch)
        {
            max[ch] = 0;
            gain[ch] = 10;
            duck[ch] = ch; // Default: one of each
        }
        countdown = HEM_ENV_FOLLOWER_SAMPLES;
    }

    void Controller() {
        if (--countdown == 0) {
            ForEachChannel(ch)
            {
                target[ch] = max[ch] * gain[ch];
                if (duck[ch]) target[ch] = HEMISPHERE_MAX_CV - target[ch]; // Handle ducking channel(s)
                target[ch] = constrain(target[ch], 0, HEMISPHERE_MAX_CV);
                max[ch] = 0;
            }
            countdown = HEM_ENV_FOLLOWER_SAMPLES;
        }

        ForEachChannel(ch)
        {
            int v = abs(In(ch));
            if (v > max[ch]) max[ch] = v;
            if (target[ch] > signal[ch]) {
                signal[ch] += speed;
                if (signal[ch] > target[ch]) signal[ch] = target[ch];
            }
            else if (target[ch] < signal[ch]) {
                signal[ch] -= speed;
                if (signal[ch] < target[ch]) signal[ch] = target[ch];
            }
            Out(ch, signal[ch]);
        }
    }

    void View() {
        gfxHeader(applet_name());
        DrawInterface();
        gfxSkyline();
    }

    void OnButtonPress() {
        CursorAction(cursor, MAX_CURSOR);
    }

    void OnEncoderMove(int direction) {
        if (!EditMode()) {
            MoveCursor(cursor, direction, MAX_CURSOR);
            return;
        }

        switch (cursor) {
        case GAIN_1:
        case GAIN_2:
            gain[cursor - GAIN_1] = constrain(gain[cursor - GAIN_1] + direction, 1, 31);
            break;

        case MODE_1:
        case MODE_2:
            duck[cursor] = 1 - duck[cursor];
            break;

        case SPEED:
            speed = constrain(speed + direction, 1, HEM_ENV_FOLLOWER_MAXSPEED);
            break;
        }
        ResetCursor();
    }
        
    uint64_t OnDataRequest() {
        uint64_t data = 0;
        Pack(data, PackLocation {0,5}, gain[0]);
        Pack(data, PackLocation {5,5}, gain[1]);
        Pack(data, PackLocation {10,1}, duck[0]);
        Pack(data, PackLocation {11,1}, duck[1]);
        Pack(data, PackLocation {12,4}, speed - 1);
        return data;
    }

    void OnDataReceive(uint64_t data) {
        gain[0] = Unpack(data, PackLocation {0,5});
        gain[1] = Unpack(data, PackLocation {5,5});
        duck[0] = Unpack(data, PackLocation {10,1});
        duck[1] = Unpack(data, PackLocation {11,1});
        speed = Unpack(data, PackLocation {12,4}) + 1;
        speed = constrain(speed, 1, HEM_ENV_FOLLOWER_MAXSPEED);
    }

protected:
    void SetHelp() {
        //                               "------------------" <-- Size Guide
        help[HEMISPHERE_HELP_DIGITALS] = "";
        help[HEMISPHERE_HELP_CVS]      = "Inputs 1,2";
        help[HEMISPHERE_HELP_OUTS]     = "Follow/Duck";
        help[HEMISPHERE_HELP_ENCODER]  = "Gain/Assign/Speed";
        //                               "------------------" <-- Size Guide
    }
    
private:
    int cursor;
    int max[2];
    uint8_t countdown;
    int signal[2];
    int target[2];

    // Setting
    uint8_t gain[2];
    bool duck[2]; // Choose between follow and duck per channel
    int speed = 1; // attack/release rate

    void DrawInterface() {
        ForEachChannel(ch)
        {
            // Duck
            gfxPrint(1 + (38 * ch), 15, duck[ch] ? "Duck" : "Foll");

            // Gain
            gfxFrame(32 * ch, 25, gain[ch], 3);

        }

        switch (cursor) {
        case MODE_1:
        case MODE_2:
            gfxCursor(1 + (38 * cursor), 23, 24);
            break;

        case GAIN_1:
        case GAIN_2:
            gfxCursor(32 * (cursor - GAIN_1), 29, gain[cursor - GAIN_1], 5);
            break;
        
        case SPEED:
            gfxIcon(20, 31, GAUGE_ICON);
            gfxPrint(28, 31, speed);
            gfxCursor(28, 39, 14);
            break;

        default: break;
        }
    }

};


////////////////////////////////////////////////////////////////////////////////
//// Hemisphere Applet Functions
///
///  Once you run the find-and-replace to make these refer to EnvFollow,
///  it's usually not necessary to do anything with these functions. You
///  should prefer to handle things in the HemisphereApplet child class
///  above.
////////////////////////////////////////////////////////////////////////////////
EnvFollow EnvFollow_instance[2];

void EnvFollow_Start(bool hemisphere) {EnvFollow_instance[hemisphere].BaseStart(hemisphere);}
void EnvFollow_Controller(bool hemisphere, bool forwarding) {EnvFollow_instance[hemisphere].BaseController(forwarding);}
void EnvFollow_View(bool hemisphere) {EnvFollow_instance[hemisphere].BaseView();}
void EnvFollow_OnButtonPress(bool hemisphere) {EnvFollow_instance[hemisphere].OnButtonPress();}
void EnvFollow_OnEncoderMove(bool hemisphere, int direction) {EnvFollow_instance[hemisphere].OnEncoderMove(direction);}
void EnvFollow_ToggleHelpScreen(bool hemisphere) {EnvFollow_instance[hemisphere].HelpScreen();}
uint64_t EnvFollow_OnDataRequest(bool hemisphere) {return EnvFollow_instance[hemisphere].OnDataRequest();}
void EnvFollow_OnDataReceive(bool hemisphere, uint64_t data) {EnvFollow_instance[hemisphere].OnDataReceive(data);}
