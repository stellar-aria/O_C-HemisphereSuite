// Copyright (c) 2018, Jason Justian
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// See https://www.pjrc.com/teensy/td_midi.html

#include "HEMISPHERE.hpp"
#include "hemisphere/applet_base.hpp"
#include "sky/midi.hpp"

using namespace skyline;
using namespace hemisphere;

constexpr int MIDI_CLOCK_DIVISOR = 12;

struct MIDILogEntry {
  int message;
  int data1;
  int data2;
};

class hMIDIIn : public AppletBase {
 public:
  // The functions available for each output
  enum hMIDIFunctions {
    MIDI_NOTE_OUT,
    MIDI_TRIG_OUT,
    MIDI_GATE_OUT,
    MIDI_VEL_OUT,
    MIDI_CC_OUT,
    MIDI_AT_OUT,
    MIDI_PB_OUT,
    MIDI_CLOCK_OUT,
    MIDI_START_OUT,

    MIDI_MAX_FUNCTION = MIDI_START_OUT
  };
  const char* fn_name[MIDI_MAX_FUNCTION + 1] = {
      "Note#", "Trig", "Gate", "Veloc", "Mod", "Aft", "Bend", "Clock", "Start"};

  const char* applet_name() { return "MIDIIn"; }

  void Start() {
    first_note = -1;
    channel = 0;  // Default channel 1

    ForEachChannel(ch) {
      function[ch] = ch * 2;
      Out(ch, 0);
    }

    log_index = 0;
    clock_count = 0;
  }

  void Controller() {
    using namespace daisy;
    usbMIDI.Listen();
    while (usbMIDI.HasEvents()) {
      MidiEvent msg  = usbMIDI.PopEvent();

    if (msg.type == SystemCommon && msg.sc_type == SystemExclusive) {
        ReceiveManagerSysEx();
        continue;
      }

      // Listen for incoming clock
      if (msg.type == SystemRealTime && msg.srt_type == TimingClock) {
        if (++clock_count == 1) {
          ForEachChannel(ch) {
            if (function[ch] == MIDI_CLOCK_OUT) {
              ClockOut(ch);
            }
          }
        }
        if (clock_count == MIDI_CLOCK_DIVISOR) clock_count = 0;
        continue;
      }

      if (msg.type == SystemRealTime && msg.srt_type == daisy::Start) {
        ForEachChannel(ch) {
          if (function[ch] == MIDI_START_OUT) {
            ClockOut(ch);
          }
        }

        UpdateLog(msg.type, msg.data[0], msg.data[1]);
        continue;
      }

      // all other messages are filtered by MIDI channel
      if (msg.channel == channel) {
        last_tick = oc::core::ticks;
        bool log_this = false;

        if (msg.type == NoteOn) {  // Note on
          // only track the most recent note
          first_note = msg.data[0];

          // Should this message go out on any channel?
          ForEachChannel(ch) {
            if (function[ch] == MIDI_NOTE_OUT)
              Out(ch, MIDIQuantizer::CV(msg.data[0]));

            if (function[ch] == MIDI_TRIG_OUT) ClockOut(ch);

            if (function[ch] == MIDI_GATE_OUT) GateOut(ch, 1);

            if (function[ch] == MIDI_VEL_OUT)
              Out(ch, Proportion(msg.data[1], 127, HEMISPHERE_MAX_CV));
          }

          log_this = 1;  // Log all MIDI notes. Other stuff is conditional.
        }

        // Note off - only for most recent note
        if (msg.type == NoteOff && msg.data[0] == first_note) {
          first_note = -1;

          // Should this message go out on any channel?
          ForEachChannel(ch) {
            if (function[ch] == MIDI_GATE_OUT) {
              GateOut(ch, 0);
              log_this = 1;
            }
          }
        }

        if (msg.type == ControlChange) {  // Modulation wheel
          ForEachChannel(ch) {
            if (function[ch] == MIDI_CC_OUT && msg.data[0] == 1) {
              Out(ch, Proportion(msg.data[1], 127, HEMISPHERE_MAX_CV));
              log_this = 1;
            }
          }
        }

        if (msg.type == ChannelPressure) {  // Aftertouch
          ForEachChannel(ch) {
            if (function[ch] == MIDI_AT_OUT) {
              Out(ch, Proportion(msg.data[0], 127, HEMISPHERE_MAX_CV));
              log_this = 1;
            }
          }
        }

        if (msg.type == PitchBend) {  // Pitch Bend
          ForEachChannel(ch) {
            if (function[ch] == MIDI_PB_OUT) {
              int data = (msg.data[1] << 7) + msg.data[0] - 8192;
              Out(ch, Proportion(data, 0x7fff, HEMISPHERE_3V_CV));
              log_this = 1;
            }
          }
        }

        if (log_this) UpdateLog(msg.type, msg.data[0], msg.data[1]);
      }
    }  // while
  }

  void View() {
    gfxHeader(applet_name());
    DrawMonitor();
    if (cursor == 3)
      DrawLog();
    else
      DrawSelector();
  }

  void OnButtonPress() { CursorAction(cursor, 3); }

  void OnEncoderMove(int direction) {
    if (!EditMode()) {
      MoveCursor(cursor, direction, 3);
      return;
    }

    if (cursor == 3) return;
    if (cursor == 0)
      channel = constrain(channel + direction, 0, 15);
    else {
      int ch = cursor - 1;
      function[ch] = constrain(function[ch] + direction, 0, MIDI_MAX_FUNCTION);
      clock_count = 0;
    }
    ResetCursor();
  }

  uint64_t OnDataRequest() {
    uint64_t data = 0;
    Pack(data, PackLocation{0, 8}, channel);
    Pack(data, PackLocation{8, 3}, function[0]);
    Pack(data, PackLocation{11, 3}, function[1]);
    return data;
  }

  void OnDataReceive(uint64_t data) {
    channel = Unpack(data, PackLocation{0, 8});
    function[0] = Unpack(data, PackLocation{8, 3});
    function[1] = Unpack(data, PackLocation{11, 3});
  }

 protected:
  void SetHelp() {
    //                               "------------------" <-- Size Guide
    help[HEMISPHERE_HELP_DIGITALS] = "";
    help[HEMISPHERE_HELP_CVS] = "";
    help[HEMISPHERE_HELP_OUTS] = "Assignable";
    help[HEMISPHERE_HELP_ENCODER] = "MIDI Ch/Assign/Log";
    //                               "------------------" <-- Size Guide
  }

 private:
  // Settings
  int channel;      // MIDI channel number
  int function[2];  // Function for each channel

  // Housekeeping
  int cursor;           // 0=MIDI channel, 1=A/C function, 2=B/D function
  int last_tick;        // Tick of last received message
  int first_note;       // First note received, for awaiting Note Off
  uint8_t clock_count;  // MIDI clock counter (24ppqn)

  // Logging
  MIDILogEntry log[7];
  int log_index;

  void UpdateLog(int message, int data1, int data2) {
    log[log_index++] = {message, data1, data2};
    if (log_index == 7) {
      for (int i = 0; i < 6; i++) {
        memcpy(&log[i], &log[i + 1], sizeof(log[i + 1]));
      }
      log_index--;
    }
  }

  void DrawMonitor() {
    if (oc::core::ticks - last_tick < 4000) {
      gfxBitmap(46, 1, 8, MIDI_ICON);
    }
  }

  void DrawSelector() {
    // MIDI Channel
    gfxPrint(1, 15, "Ch:");
    gfxPrint(24, 15, channel + 1);

    // Output 1 function
    if (hemisphere == 0)
      gfxPrint(1, 25, "A :");
    else
      gfxPrint(1, 25, "C :");
    gfxPrint(24, 25, fn_name[function[0]]);

    // Output 2 function
    if (hemisphere == 0)
      gfxPrint(1, 35, "B :");
    else
      gfxPrint(1, 35, "D :");
    gfxPrint(24, 35, fn_name[function[1]]);

    // Cursor
    gfxCursor(24, 23 + (cursor * 10), 39);

    // Last log entry
    if (log_index > 0) {
      log_entry(56, log_index - 1);
    }
    gfxInvert(0, 55, 63, 9);
  }

  void DrawLog() {
    if (log_index) {
      for (int i = 0; i < log_index; i++) {
        log_entry(15 + (i * 8), i);
      }
    }
  }

  void log_entry(int y, int index) {
    switch (log[index].message) {
      case MIDI::NOTE_ON:
        gfxBitmap(1, y, 8, NOTE_ICON);
        gfxPrint(10, y, midi_note_numbers[log[index].data1]);
        gfxPrint(40, y, log[index].data2);
        break;

      case MIDI::NOTE_OFF:
        gfxPrint(1, y, "-");
        gfxPrint(10, y, midi_note_numbers[log[index].data1]);
        break;

      case MIDI::CONTROL_CHANGE:
        gfxBitmap(1, y, 8, MOD_ICON);
        gfxPrint(10, y, log[index].data2);
        break;

      case MIDI::CHANNEL_PRESSURE:
        gfxBitmap(1, y, 8, AFTERTOUCH_ICON);
        gfxPrint(10, y, log[index].data1);
        break;

      case MIDI::PITCH_BEND: {
        int data = (log[index].data2 << 7) + log[index].data1 - 8192;
        gfxBitmap(1, y, 8, BEND_ICON);
        gfxPrint(10, y, data);
        break;
      }

      default:
        gfxPrint(1, y, "?");
        gfxPrint(10, y, log[index].data1);
        gfxPrint(" ");
        gfxPrint(log[index].data2);
        break;
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
//// Hemisphere Applet Functions
///
///  Once you run the find-and-replace to make these refer to hMIDIIn,
///  it's usually not necessary to do anything with these functions. You
///  should prefer to handle things in the HemisphereApplet child class
///  above.
////////////////////////////////////////////////////////////////////////////////
hMIDIIn hMIDIIn_instance[2];

void hMIDIIn_Start(bool hemisphere) {
  hMIDIIn_instance[hemisphere].BaseStart(hemisphere);
}

void hMIDIIn_Controller(bool hemisphere, bool forwarding) {
  hMIDIIn_instance[hemisphere].BaseController(forwarding);
}

void hMIDIIn_View(bool hemisphere) { hMIDIIn_instance[hemisphere].BaseView(); }

void hMIDIIn_OnButtonPress(bool hemisphere) {
  hMIDIIn_instance[hemisphere].OnButtonPress();
}

void hMIDIIn_OnEncoderMove(bool hemisphere, int direction) {
  hMIDIIn_instance[hemisphere].OnEncoderMove(direction);
}

void hMIDIIn_ToggleHelpScreen(bool hemisphere) {
  hMIDIIn_instance[hemisphere].HelpScreen();
}

uint64_t hMIDIIn_OnDataRequest(bool hemisphere) {
  return hMIDIIn_instance[hemisphere].OnDataRequest();
}

void hMIDIIn_OnDataReceive(bool hemisphere, uint64_t data) {
  hMIDIIn_instance[hemisphere].OnDataReceive(data);
}
