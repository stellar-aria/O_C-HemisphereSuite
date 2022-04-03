// Categories*:
// 0x01 = Modulator
// 0x02 = Sequencer
// 0x04 = Clocking
// 0x08 = Quantizer
// 0x10 = Utility
// 0x20 = MIDI
// 0x40 = Logic
// 0x80 = Other
//
// * Category filtering is deprecated at 1.8, but I'm leaving the per-applet categorization
// alone to avoid breaking forked codebases by other developers.

#include "HEM_ADSREG.h"
#include "HEM_ADEG.h"
#include "HEM_AnnularFusion.h"
#include "HEM_ASR.h"
#include "HEM_AttenuateOffset.h"
// #include "HEM_Binary.h"
// #include "HEM_BootsNCat.h"
#include "HEM_Brancher.h"
#include "HEM_Burst.h"
#include "HEM_Calculate.h"
#include "HEM_Carpeggio.h"
#include "HEM_ClockDivider.h"
#include "HEM_ClockSkip.h"
#include "HEM_Compare.h"
// #include "HEM_CVRecV2.h"
// #include "HEM_DrCrusher.h"
#include "HEM_DualQuant.h"
#include "HEM_EnigmaJr.h"
#include "HEM_EnvFollow.h"
#include "HEM_GateDelay.h"
// #include "HEM_GatedVCA.h"
// #include "HEM_LoFiPCM.h"
#include "HEM_Logic.h"
// #include "HEM_LowerRenz.h"
#include "HEM_Metronome.h"
#include "HEM_hMIDIIn.h"
#include "HEM_hMIDIOut.h"
// #include "HEM_MixerBal.h"
// #include "HEM_Palimpsest.h"
#include "HEM_RndWalk.h"
// #include "HEM_RunglBook.h"
#include "HEM_ScaleDuet.h"
#include "HEM_Schmitt.h"
#include "HEM_Scope.h"
#include "HEM_Sequence5.h"
#include "HEM_ShiftGate.h"
#include "HEM_TM.h"
#include "HEM_Shuffle.h"
#include "HEM_SkewedLFO.h"
#include "HEM_Slew.h"
// #include "HEM_Squanch.h"
#include "HEM_Switch.h"
// #include "HEM_TLNeuron.h"
// #include "HEM_Trending.h"
#include "HEM_TrigSeq.h"
#include "HEM_TrigSeq16.h"
#include "HEM_Tuner.h"
// #include "HEM_VectorEG.h"
// #include "HEM_VectorLFO.h"
// #include "HEM_VectorMod.h"
// #include "HEM_VectorMorph.h"
#include "HEM_Voltage.h"

#include "HEM_Stairs.h"
#include "HEM_TB3PO.h"

#include "HEM_DrumMap.h"
#include "HEM_Shredder.h"

#define HEMISPHERE_AVAILABLE_APPLETS 39 //51

//////////////////  id  cat   class name
#define HEMISPHERE_APPLETS { \
    DECLARE_APPLET(  8, 0x01, ADSREG), \
    DECLARE_APPLET( 34, 0x01, ADEG), \
    DECLARE_APPLET( 15, 0x02, AnnularFusion), \
    DECLARE_APPLET( 47, 0x09, ASR), \
    DECLARE_APPLET( 56, 0x10, AttenuateOffset), \
    DECLARE_APPLET(  4, 0x14, Brancher), \
    DECLARE_APPLET( 31, 0x04, Burst), \
    DECLARE_APPLET( 12, 0x10, Calculate),\
    DECLARE_APPLET( 32, 0x0a, Carpeggio), \
    DECLARE_APPLET(  6, 0x04, ClockDivider), \
    DECLARE_APPLET( 28, 0x04, ClockSkip), \
    DECLARE_APPLET( 30, 0x10, Compare), \
    DECLARE_APPLET(  9, 0x08, DualQuant), \
    DECLARE_APPLET( 45, 0x02, EnigmaJr), \
    DECLARE_APPLET( 42, 0x11, EnvFollow), \
    DECLARE_APPLET( 29, 0x04, GateDelay), \
    DECLARE_APPLET( 10, 0x44, Logic), \
    DECLARE_APPLET( 50, 0x04, Metronome), \
    DECLARE_APPLET(150, 0x20, hMIDIIn), \
    DECLARE_APPLET( 27, 0x20, hMIDIOut), \
    DECLARE_APPLET( 44, 0x01, RndWalk), \
    DECLARE_APPLET( 26, 0x08, ScaleDuet), \
    DECLARE_APPLET( 40, 0x40, Schmitt), \
    DECLARE_APPLET( 23, 0x80, Scope), \
    DECLARE_APPLET( 14, 0x02, Sequence5), \
    DECLARE_APPLET( 48, 0x45, ShiftGate), \
    DECLARE_APPLET( 18, 0x02, TM), \
    DECLARE_APPLET( 36, 0x04, Shuffle), \
    DECLARE_APPLET(  7, 0x01, SkewedLFO), \
    DECLARE_APPLET( 19, 0x01, Slew), \
    DECLARE_APPLET(  3, 0x10, Switch), \
    DECLARE_APPLET( 57, 0x01, Stairs), \
    DECLARE_APPLET( 58, 0x01, TB_3PO), \
    DECLARE_APPLET( 11, 0x06, TrigSeq), \
    DECLARE_APPLET( 25, 0x06, TrigSeq16), \
    DECLARE_APPLET( 52, 0x01, Shredder), \
    DECLARE_APPLET( 49, 0x01, DrumMap), \
    DECLARE_APPLET( 39, 0x80, Tuner), \
    DECLARE_APPLET( 43, 0x10, Voltage), \
}
/*
    // DECLARE_APPLET( 52, 0x01, VectorEG), \
    // DECLARE_APPLET( 49, 0x01, VectorLFO), \
    // DECLARE_APPLET( 53, 0x01, VectorMod), \
    // DECLARE_APPLET( 54, 0x01, VectorMorph), \
    // DECLARE_APPLET( 13, 0x40, TLNeuron), \
    DECLARE_APPLET( 44, 0x01, RunglBook), \
    DECLARE_APPLET( 24, 0x02, CVRecV2), \
    DECLARE_APPLET( 55, 0x80, DrCrusher), \
    DECLARE_APPLET( 16, 0x80, LoFiPCM), \
    DECLARE_APPLET( 21, 0x01, LowerRenz), \
    DECLARE_APPLET( 17, 0x50, GatedVCA), \
    DECLARE_APPLET( 51, 0x80, BootsNCat), \
    DECLARE_APPLET( 41, 0x41, Binary), \
    DECLARE_APPLET( 37, 0x40, Trending), \
    DECLARE_APPLET( 20, 0x02, Palimpsest), \
    DECLARE_APPLET( 46, 0x08, Squanch), \
    DECLARE_APPLET( 33, 0x10, MixerBal), \
    DECLARE_APPLET(127, 0x80, DIAGNOSTIC), \
*/
