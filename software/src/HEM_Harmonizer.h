// Harmonizer for o_c hemisphere by Karol Firmanty.
// Outputs chord voicing based on user selected scale and CV input.
//
// Based on DualQuant, Copyright 2018 Jason Justian
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

#include <Arduino.h>
#include "OC_core.h"
#include "HemisphereApplet.h"
#include "braids_quantizer.h"
#include "braids_quantizer_scales.h"
#include "OC_scales.h"

class Harmonizer : public HemisphereApplet {
  public:

    const char* applet_name() { // Maximum 10 characters
      return "Harmonizer";
    }

    void Start() {
      cursor = 0;
      quantizer.Init();
      scale = 5;
      scale_data = OC::Scales::GetScale(scale);
      quantizer.Configure(scale_data, 0xffff);
      last_note[0] = 0;
      last_note[1] = 0;
    }

    void Controller() {
      int32_t pitch = In(0);
      int32_t quantized = quantizer.Process(pitch, root << 7, 0);
      if (last_quantized_input != quantized) {
        last_quantized_input = quantized;
        int scale_step = FindScaleStep(quantized);

        ForEachChannel(ch)
        {
          int32_t offset_voltage = FindOffsetVoltage(scale_step, offset[ch]);
          last_note[ch] = quantized + offset_voltage;
        }
      }
      ForEachChannel(ch)
      {
        Out(ch, last_note[ch]);
      }
    }

    void View() {
      gfxHeader(applet_name());
      DrawSelector();
    }

    void OnButtonPress() {
      if (++cursor > 3) cursor = 0;
      ResetCursor();
    }

    void OnEncoderMove(int direction) {
      last_quantized_input = -1; //invalidate last_quantized_input to force refresh CV outs
      switch (cursor) {
        case 0:
          scale += direction;
          if (scale >= OC::Scales::NUM_SCALES) scale = 0;
          if (scale < 0) scale = OC::Scales::NUM_SCALES - 1;
          scale_data = OC::Scales::GetScale(scale);
          quantizer.Configure(scale_data, 0xffff);
          ForEachChannel(ch)
          {
            offset[ch] = constrain(offset[ch], 0, scale_data.num_notes);
          }
          break;
        case 1:
          root = constrain(root + direction, 0, 11);
          break;
        case 2:
          offset[0] = constrain(offset[0] + direction, 0, scale_data.num_notes);
          break;
        case 3:
          offset[1] = constrain(offset[1] + direction, 0, scale_data.num_notes);
          break;
      }
    }

    uint32_t OnDataRequest() {
      uint32_t data = 0;
      Pack(data, PackLocation {0, 8}, scale);
      Pack(data, PackLocation {8, 4}, root);
      ForEachChannel(ch)
      {
        Pack(data, PackLocation {12 + 4 * ch, 4}, offset[ch]);
      }
      return data;
    }

    void OnDataReceive(uint32_t data) {
      scale = Unpack(data, PackLocation {0, 8});
      root = Unpack(data, PackLocation {8, 4});

      root = constrain(root, 0, 11);
      scale_data = OC::Scales::GetScale(scale);
      ForEachChannel(ch)
      {
        offset[ch] = Unpack(data, PackLocation {12 + 4 * ch, 4});
        offset[ch] = constrain(root, 0, scale_data.num_notes);
      }
      quantizer.Configure(scale_data, 0xffff);
    }

  protected:
    /* Set help text. Each help section can have up to 18 characters. Be concise! */
    void SetHelp() {
      //                               "------------------" <-- Size Guide
      help[HEMISPHERE_HELP_DIGITALS] = "";
      help[HEMISPHERE_HELP_CVS] =      "CV 1=Pitch";
      help[HEMISPHERE_HELP_OUTS] =     "OUT A=V2 B=V3";
      help[HEMISPHERE_HELP_ENCODER] =  "Scale/Root/Offset";
      //                               "------------------" <-- Size Guide
    }

  private:
    braids::Quantizer quantizer;
    int last_note[2];
    int last_quantized_input;
    int offset[2];
    int cursor;

    // Settings
    int scale;
    uint8_t root;
    braids::Scale scale_data;

    void DrawSelector()
    {
      // Draw settings
      gfxPrint(0, 15, OC::scale_names_short[scale]);
      gfxPrint(10, 25, OC::Strings::note_names_unpadded[root]);

      // Draw cursor
      switch (cursor) {
        case 0:
          gfxCursor(0, 23, 30);
          break;
        case 1:
          gfxCursor(10, 33, 12);
          break;
        case 2:
          gfxCursor(31, 23, 12);
          break;
        case 3:
          gfxCursor(31, 33, 12);
          break;
      }

      ForEachChannel(ch)
      {
        // Draw offsets
        gfxPrint(31, 15 + 10 * ch, offset[ch]);

        // Print semitones
        int semitone = (last_note[ch] / 128) % 12;
        gfxPrint(10, 41 + (10 * ch), semitone);
      }
    }

    int FindScaleStep(int32_t quantized_pitch) {
      int16_t base_pitch = quantized_pitch % scale_data.span;
      int index = 0;
      for (int i = 0; i < scale_data.num_notes; i++) {
        if (scale_data.notes[i] == base_pitch) {
          index = i;
          break;
        }
      }
      return index;
    }

    int32_t FindOffsetVoltage(int scale_step, int offset) {
      int16_t base_pitch = scale_data.notes[scale_step];
      int step = scale_step + offset;
      if (step < scale_data.num_notes) {
        return scale_data.notes[step] - base_pitch;
      } else {
        return (scale_data.notes[step % scale_data.num_notes] + scale_data.span) - base_pitch;
      }
    }
};


////////////////////////////////////////////////////////////////////////////////
//// Hemisphere Applet Functions
///
///  Once you run the find-and-replace to make these refer to Harmonizer,
///  it's usually not necessary to do anything with these functions. You
///  should prefer to handle things in the HemisphereApplet child class
///  above.
////////////////////////////////////////////////////////////////////////////////
Harmonizer Harmonizer_instance[2];

void Harmonizer_Start(bool hemisphere) {
  Harmonizer_instance[hemisphere].BaseStart(hemisphere);
}

void Harmonizer_Controller(bool hemisphere, bool forwarding) {
  Harmonizer_instance[hemisphere].BaseController(forwarding);
}

void Harmonizer_View(bool hemisphere) {
  Harmonizer_instance[hemisphere].BaseView();
}

void Harmonizer_OnButtonPress(bool hemisphere) {
  Harmonizer_instance[hemisphere].OnButtonPress();
}

void Harmonizer_OnEncoderMove(bool hemisphere, int direction) {
  Harmonizer_instance[hemisphere].OnEncoderMove(direction);
}

void Harmonizer_ToggleHelpScreen(bool hemisphere) {
  Harmonizer_instance[hemisphere].HelpScreen();
}

uint32_t Harmonizer_OnDataRequest(bool hemisphere) {
  return Harmonizer_instance[hemisphere].OnDataRequest();
}

void Harmonizer_OnDataReceive(bool hemisphere, uint32_t data) {
  Harmonizer_instance[hemisphere].OnDataReceive(data);
}
