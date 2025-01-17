#include "oc/chords.h"
#include "oc/chords_presets.h"

namespace oc {

    Chord user_chords[Chords::CHORDS_USER_LAST];

    /*static*/
    const int Chords::NUM_CHORD_PROGRESSIONS = 0x4;
    const int Chords::NUM_CHORDS_TOTAL = oc::Chords::CHORDS_USER_LAST; // = 8
    const int Chords::NUM_CHORDS_PROPERTIES = sizeof(Chord);
    const int Chords::NUM_CHORDS = Chords::NUM_CHORDS_TOTAL / Chords::NUM_CHORD_PROGRESSIONS;

    /*static*/
    // 
    void Chords::Init() {
      for (size_t i = 0; i < oc::Chords::CHORDS_USER_LAST; ++i)
        memcpy(&user_chords[i], &oc::chords[0], sizeof(Chord));
    }

    const Chord &Chords::GetChord(int index, int progression) {

       uint8_t _index = index + progression * Chords::NUM_CHORDS;
       if (_index < CHORDS_USER_LAST) 
        return user_chords[_index];
       else
        return user_chords[0x0];
    }
} // namespace oc
