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

class ClockSetup : public HemisphereApplet {
public:

    enum ClockSetupCursor {
        PLAY_STOP,
        FORWARDING,
        EXT_PPQN,
        TEMPO,
        MULT1,
        MULT2,
        MULT3,
        MULT4,
        TRIG1,
        TRIG2,
        TRIG3,
        TRIG4,
        LAST_SETTING = TRIG4
    };

    const char* applet_name() {
        return "ClockSet";
    }

    void Start() { }

    // The ClockSetup controller handles MIDI Clock and Transport Start/Stop
    void Controller() {
        if (start_q){
            start_q = 0;
            usbMIDI.sendRealTime(usbMIDI.Start);
        }
        if (stop_q){
            stop_q = 0;
            usbMIDI.sendRealTime(usbMIDI.Stop);
        }
        if (clock_m->IsRunning() && clock_m->MIDITock()) usbMIDI.sendRealTime(usbMIDI.Clock);

        // 4 internal clock flashers
        for (int i = 0; i < 4; ++i) {
            if (clock_m->Tock(i))
                flash_ticker[i] = HEMISPHERE_PULSE_ANIMATION_TIME;
            else if (flash_ticker[i])
                --flash_ticker[i];
        }

        if (button_ticker) --button_ticker;
    }

    void View() {
        DrawInterface();
    }

    void OnButtonPress() {
        if (!EditMode()) { // special cases for toggle buttons
            if (cursor == PLAY_STOP) PlayStop();
            else if (cursor == FORWARDING) clock_m->ToggleForwarding();
            else if (cursor >= TRIG1) {
                clock_m->Boop(cursor-TRIG1);
                button_ticker = HEMISPHERE_PULSE_ANIMATION_TIME_LONG;
            }
            else CursorAction(cursor, LAST_SETTING);
        }
        else CursorAction(cursor, LAST_SETTING);

        if (cursor == TEMPO) {
            // Tap Tempo detection
            if (last_tap_tick) {
                tap_time[taps] = OC::CORE::ticks - last_tap_tick;

                if (tap_time[taps] > CLOCK_TICKS_MAX) {
                    taps = 0;
                    last_tap_tick = 0;
                }
                else if (++taps == NR_OF_TAPS)
                    clock_m->SetTempoFromTaps(tap_time, taps);

                taps %= NR_OF_TAPS;
            }
            last_tap_tick = OC::CORE::ticks;
        }
    }

    void OnEncoderMove(int direction) {
        taps = 0;
        last_tap_tick = 0;
        if (!EditMode()) {
            MoveCursor(cursor, direction, LAST_SETTING);
            return;
        }

        switch ((ClockSetupCursor)cursor) {
        case PLAY_STOP:
            PlayStop();
            break;

        case FORWARDING:
            clock_m->ToggleForwarding();
            break;

        case TRIG1:
        case TRIG2:
        case TRIG3:
        case TRIG4:
            clock_m->Boop(cursor-TRIG1);
            button_ticker = HEMISPHERE_PULSE_ANIMATION_TIME_LONG;
            break;

        case EXT_PPQN:
            clock_m->SetClockPPQN(clock_m->GetClockPPQN() + direction);
            break;
        case TEMPO:
            clock_m->SetTempoBPM(clock_m->GetTempo() + direction);
            break;

        case MULT1:
        case MULT2:
        case MULT3:
        case MULT4:
            clock_m->SetMultiply(clock_m->GetMultiply(cursor - MULT1) + direction, cursor - MULT1);
            break;

        default: break;
        }
    }

    uint64_t OnDataRequest() {
        uint64_t data = 0;
        Pack(data, PackLocation { 0, 1 }, clock_m->IsRunning() || clock_m->IsPaused());
        Pack(data, PackLocation { 1, 1 }, clock_m->IsForwarded());
        Pack(data, PackLocation { 2, 9 }, clock_m->GetTempo());
        Pack(data, PackLocation { 11, 5 }, clock_m->GetClockPPQN());
        for (size_t i = 0; i < 4; ++i) {
            Pack(data, PackLocation { 16+i*6, 6 }, clock_m->GetMultiply(i)+32);
        }

        // other config settings are kept here as well, it's convenient
        Pack(data, PackLocation { 50, 2 }, HemisphereApplet::modal_edit_mode);
        Pack(data, PackLocation { 52, 7 }, HemisphereApplet::trig_length);

        return data;
    }

    void OnDataReceive(uint64_t data) {
        /* leave Play state unchanged - Start/Stop will always be manual
        if (Unpack(data, PackLocation { 0, 1 })) {
            clock_m->Start(1); // start paused
        } else {
            clock_m->Stop();
        }
        */
        clock_m->SetForwarding(Unpack(data, PackLocation { 1, 1 }));
        clock_m->SetTempoBPM(Unpack(data, PackLocation { 2, 9 }));
        clock_m->SetClockPPQN(Unpack(data, PackLocation { 11, 5 }));
        for (size_t i = 0; i < 4; ++i) {
            clock_m->SetMultiply(Unpack(data, PackLocation { 16+i*6, 6 })-32, i);
        }

        HemisphereApplet::modal_edit_mode = Unpack(data, PackLocation { 50, 2 });
        HemisphereApplet::trig_length = constrain( Unpack(data, PackLocation { 52, 7 }), 1, 127);
    }

protected:
    void SetHelp() {
        //                               "------------------" <-- Size Guide
        help[HEMISPHERE_HELP_DIGITALS] = "";
        help[HEMISPHERE_HELP_CVS]      = "";
        help[HEMISPHERE_HELP_OUTS]     = "";
        help[HEMISPHERE_HELP_ENCODER]  = "";
        //                               "------------------" <-- Size Guide
    }

private:
    int cursor; // ClockSetupCursor
    bool start_q;
    bool stop_q;
    int flash_ticker[4];
    int button_ticker;
    ClockManager *clock_m = clock_m->get();

    static const int NR_OF_TAPS = 3;

    int taps = 0; // tap tempo
    uint32_t tap_time[NR_OF_TAPS]; // buffer of past tap tempo measurements
    uint32_t last_tap_tick = 0;

    void PlayStop() {
        if (clock_m->IsRunning()) {
            stop_q = 1;
            clock_m->Stop();
        } else {
            start_q = clock_m->IsPaused();
            clock_m->Start( !start_q ); // stop->pause->start
        }
    }

    void DrawInterface() {
        // Header: This is sort of a faux applet, so its header
        // needs to extend across the screen
        graphics.setPrintPos(1, 2);
        graphics.print("Clock Setup");
        //gfxLine(0, 10, 62, 10);
        //gfxLine(0, 12, 62, 12);
        graphics.drawLine(0, 10, 127, 10);
        graphics.drawLine(0, 12, 127, 12);

        // Clock Source
        gfxIcon(1, 15, CLOCK_ICON);
        if (clock_m->IsRunning()) {
            gfxIcon(12, 15, PLAY_ICON);
        } else if (clock_m->IsPaused()) {
            gfxIcon(12, 15, PAUSE_ICON);
        } else {
            gfxIcon(12, 15, STOP_ICON);
        }
        gfxPrint(26, 15, "Fwd ");
        gfxIcon(50, 15, clock_m->IsForwarded() ? CHECK_ON_ICON : CHECK_OFF_ICON);

        // Input PPQN
        gfxPrint(64, 15, "PPQN x");
        gfxPrint(clock_m->GetClockPPQN());

        // Tempo
        gfxIcon(1, 26, NOTE4_ICON);
        gfxPrint(9, 26, "= ");
        gfxPrint(pad(100, clock_m->GetTempo()), clock_m->GetTempo());
        gfxPrint(" BPM");

        for (int ch=0; ch<4; ++ch) {
            int mult = clock_m->GetMultiply(ch);
            int x = ch * 32;

            // Multipliers
            gfxPrint(1 + x, 37, (mult >= 0) ? "x" : "/");
            gfxPrint( (mult >= 0) ? mult : 1 - mult );

            // Manual triggers
            gfxIcon(4 + x, 47, (button_ticker && ch == cursor-TRIG1)?BTN_ON_ICON:BTN_OFF_ICON);
            gfxIcon(4 + x, 54, DOWN_BTN_ICON);
            if (flash_ticker[ch]) gfxInvert(3 + x, 56, 9, 8);
        }

        switch ((ClockSetupCursor)cursor) {
        case PLAY_STOP: gfxFrame(11, 14, 10, 10); break;
        case FORWARDING: gfxFrame(49, 14, 10, 10); break;

        case EXT_PPQN:
            gfxCursor(100,23, 12);
            break;

        case TEMPO:
            gfxCursor(22, 34, 18);
            break;

        case MULT1:
        case MULT2:
        case MULT3:
        case MULT4:
            gfxCursor(8 + 32*(cursor-MULT1), 45, 12);
            break;

        case TRIG1:
        case TRIG2:
        case TRIG3:
        case TRIG4:
            if (0 == button_ticker)
                gfxIcon(12 + 32*(cursor-TRIG1), 49, LEFT_ICON);
            break;

        default: break;
        }
    }
};


////////////////////////////////////////////////////////////////////////////////
//// Hemisphere Applet Functions
///
///  Once you run the find-and-replace to make these refer to ClockSetup,
///  it's usually not necessary to do anything with these functions. You
///  should prefer to handle things in the HemisphereApplet child class
///  above.
////////////////////////////////////////////////////////////////////////////////
ClockSetup ClockSetup_instance[1];

void ClockSetup_Start(bool hemisphere) {ClockSetup_instance[hemisphere].BaseStart(hemisphere);}
// NJM: don't call BaseController for ClockSetup
void ClockSetup_Controller(bool hemisphere, bool forwarding) {ClockSetup_instance[hemisphere].Controller();}
void ClockSetup_View(bool hemisphere) {ClockSetup_instance[hemisphere].BaseView();}
void ClockSetup_OnButtonPress(bool hemisphere) {ClockSetup_instance[hemisphere].OnButtonPress();}
void ClockSetup_OnEncoderMove(bool hemisphere, int direction) {ClockSetup_instance[hemisphere].OnEncoderMove(direction);}
void ClockSetup_ToggleHelpScreen(bool hemisphere) {ClockSetup_instance[hemisphere].HelpScreen();}
uint64_t ClockSetup_OnDataRequest(bool hemisphere) {return ClockSetup_instance[hemisphere].OnDataRequest();}
void ClockSetup_OnDataReceive(bool hemisphere, uint64_t data) {ClockSetup_instance[hemisphere].OnDataReceive(data);}
