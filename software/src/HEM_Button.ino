// Copyright (c) 2022, Jason Justian
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

class Button : public HemisphereApplet {
public:

    const char* applet_name() { // Maximum 10 characters
        return "Button2";
    }

	/* Run when the Applet is selected */
    void Start() {
        trigger_countdown = 0;
    }

	/* Run during the interrupt service routine, 16667 times per second */
    void Controller() {
        ForEachChannel(ch) {
            // Check physical trigger input to emulate button press (ignore forwarding)
            if (Clock(ch, 1)) PressButton(ch);

            // Handle output if triggered
            if (trigger_out[ch]) {
                if (gate_mode[ch])
                    GateOut(ch, toggle_st[ch]);
                else
                    ClockOut(ch);

                trigger_out[ch] = 0; // Clear trigger queue
            }
        }
        if (trigger_countdown) trigger_countdown--;
    }

	/* Draw the screen */
    void View() {
        gfxHeader(applet_name());
        DrawIndicator();
    }

	/* Called when the encoder button for this hemisphere is pressed */
    void OnButtonPress() {
        PressButton(channel);
        trigger_countdown = 1667; // Trigger display countdown
    }

	/* Called when the encoder for this hemisphere is rotated
	 * direction 1 is clockwise
	 * direction -1 is counterclockwise
	 */
    void OnEncoderMove(int direction) {
        if (direction > 0) // CW toggles which channel
            channel = 1 - channel;
        else { // CCW toggles between Trig/Gate output
            // retrigger if switching from a Gate On to Trig mode
            trigger_out[channel] = gate_mode[channel] && toggle_st[channel];

            gate_mode[channel] = 1 - gate_mode[channel];
            toggle_st[channel] = 0; // reset Gate to off
        }
    }
        
    /* No state data for this applet
     */
    uint64_t OnDataRequest() {
        uint64_t data = 0;
        return data;
    }

    /* No state data for this applet
     */
    void OnDataReceive(uint64_t data) {
        return;
    }

protected:
    /* Set help text. Each help section can have up to 18 characters. Be concise! */
    void SetHelp() {
        //                               "------------------" <-- Size Guide
        help[HEMISPHERE_HELP_DIGITALS] = "1,2=Press Button";
        help[HEMISPHERE_HELP_CVS]      = "";
        help[HEMISPHERE_HELP_OUTS]     = "A,B=Trig/Gate Out";
        help[HEMISPHERE_HELP_ENCODER]  = "T=Config P=Button";
        //                               "------------------" <-- Size Guide
    }
    
private:
    bool trigger_out[2] = {0,0}; // Trigger output queue (output A/C)
    bool toggle_st[2] = {0,0}; // Toggle state (output B/D)
    int trigger_countdown; // For momentary display of trigger output
    bool channel = 0; // selected channel, 0=(output A/C) 1=(output B/D)
    bool gate_mode[2] = {0,1}; // mode flag for each channel, 0=trig, 1=gate toggle

    void DrawIndicator()
    {
        // Guide text
        gfxIcon(1, 15, ROTATE_R_ICON);
        gfxPrint(10, 15, "Channel");
        gfxIcon(1, 24, ROTATE_L_ICON);
        gfxPrint(10, 24, "Mode");

        if (trigger_countdown) gfxFrame(9 + 32*channel, 42, 13, 13); // momentary manual trigger indicator

        gfxIcon(12 + channel*32, 35, DOWN_BTN_ICON); // channel selector

        ForEachChannel(ch) {
            if (!gate_mode[ch])
                gfxIcon(12 + ch*32, 45, CLOCK_ICON);
            else
                gfxIcon(12 + ch*32, 45, toggle_st[ch] ? CLOSED_ICON : OPEN_ICON);
        }
    }

    void PressButton(bool ch = 0)
    {
        toggle_st[ch] = 1 - toggle_st[ch]; // Alternate toggle state when pressed
        trigger_out[ch] = 1; // Set trigger queue
    }
};


////////////////////////////////////////////////////////////////////////////////
//// Hemisphere Applet Functions
///
///  Once you run the find-and-replace to make these refer to Button,
///  it's usually not necessary to do anything with these functions. You
///  should prefer to handle things in the HemisphereApplet child class
///  above.
////////////////////////////////////////////////////////////////////////////////
Button Button_instance[2];

void Button_Start(bool hemisphere) {Button_instance[hemisphere].BaseStart(hemisphere);}
void Button_Controller(bool hemisphere, bool forwarding) {Button_instance[hemisphere].BaseController(forwarding);}
void Button_View(bool hemisphere) {Button_instance[hemisphere].BaseView();}
void Button_OnButtonPress(bool hemisphere) {Button_instance[hemisphere].OnButtonPress();}
void Button_OnEncoderMove(bool hemisphere, int direction) {Button_instance[hemisphere].OnEncoderMove(direction);}
void Button_ToggleHelpScreen(bool hemisphere) {Button_instance[hemisphere].HelpScreen();}
uint64_t Button_OnDataRequest(bool hemisphere) {return Button_instance[hemisphere].OnDataRequest();}
void Button_OnDataReceive(bool hemisphere, uint64_t data) {Button_instance[hemisphere].OnDataReceive(data);}
