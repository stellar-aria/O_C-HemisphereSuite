class GatedVCA : public HemisphereApplet {
public:

    const char* applet_name() { // Maximum 10 characters
        return "Gated VCA";
    }

    void Start() {
        amp_offset_pct = 0;
        amp_offset_cv = 0;
    }

    void Controller() {
        int signal = In(0);
        int amplitude = In(1);
        int output = ProportionCV(amplitude, signal);
        output += amp_offset_cv;
        output = constrain(output, -HEMISPHERE_MAX_CV, HEMISPHERE_MAX_CV);

        Out(1, output); // Regular VCA output on B
        if (Gate(0)) Out(0, output); // Gated VCA output on A
        else Out(0, 0);
    }

    void View() {
        gfxHeader(applet_name());
        DrawInterface();
    }

    void ScreensaverView() {
        DrawInterface();
    }

    void OnButtonPress() {
    }

    void OnEncoderMove(int direction) {
        amp_offset_pct = constrain(amp_offset_pct += direction, 0, 100);
        amp_offset_cv = Proportion(amp_offset_pct, 100, HEMISPHERE_MAX_CV);
    }

    uint32_t OnDataRequest() {
        uint32_t data = 0;
        return data;
    }

    void OnDataReceive(uint32_t data) {
    }

protected:
    void SetHelp() {
        help[HEMISPHERE_HELP_DIGITALS] = "1=Gate Out A";
        help[HEMISPHERE_HELP_CVS] = "1=CV signal 2=Amp";
        help[HEMISPHERE_HELP_OUTS] = "A=Gated out B=Out";
        help[HEMISPHERE_HELP_ENCODER] = "T=Amp CV Offset";
    }
    
private:
    int amp_offset_pct; // Offset as percentage of max cv
    int amp_offset_cv; // Raw CV offset; calculated on encoder move

    void DrawInterface() {
        gfxPrint(0, 15, "Offset:");
        gfxPrint(amp_offset_pct);
        gfxSkyline();
    }
};


////////////////////////////////////////////////////////////////////////////////
//// Hemisphere Applet Functions
///
///  Once you run the find-and-replace to make these refer to GatedVCA,
///  it's usually not necessary to do anything with these functions. You
///  should prefer to handle things in the HemisphereApplet child class
///  above.
////////////////////////////////////////////////////////////////////////////////
GatedVCA GatedVCA_instance[2];

void GatedVCA_Start(int hemisphere) {
    GatedVCA_instance[hemisphere].BaseStart(hemisphere);
}

void GatedVCA_Controller(int hemisphere, bool forwarding) {
    GatedVCA_instance[hemisphere].BaseController(forwarding);
}

void GatedVCA_View(int hemisphere) {
    GatedVCA_instance[hemisphere].BaseView();
}

void GatedVCA_Screensaver(int hemisphere) {
    GatedVCA_instance[hemisphere].BaseScreensaverView();
}

void GatedVCA_OnButtonPress(int hemisphere) {
    GatedVCA_instance[hemisphere].OnButtonPress();
}

void GatedVCA_OnEncoderMove(int hemisphere, int direction) {
    GatedVCA_instance[hemisphere].OnEncoderMove(direction);
}

void GatedVCA_ToggleHelpScreen(int hemisphere) {
    GatedVCA_instance[hemisphere].HelpScreen();
}

uint32_t GatedVCA_OnDataRequest(int hemisphere) {
    return GatedVCA_instance[hemisphere].OnDataRequest();
}

void GatedVCA_OnDataReceive(int hemisphere, uint32_t data) {
    GatedVCA_instance[hemisphere].OnDataReceive(data);
}