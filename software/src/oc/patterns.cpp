#include "oc/patterns.h"
#include "oc/patterns_presets.h"

namespace oc {

    Pattern user_patterns[Patterns::PATTERN_USER_ALL];
    Pattern dummy_pattern;

    /*static*/
    const int Patterns::NUM_PATTERNS = oc::Patterns::PATTERN_USER_LAST; // = 4

    /*static*/
    // 
    void Patterns::Init() {
      for (size_t i = 0; i < oc::Patterns::PATTERN_USER_ALL ; ++i)
        memcpy(&user_patterns[i], &oc::patterns[0], sizeof(Pattern));
    }
    
    const char* const pattern_names_short[] = {
        "SEQ-1",
        "SEQ-2",
        "SEQ-3",
        "SEQ-4",
        "DEFAULT"
    };

    const char* const pattern_names[] = {
        "User-sequence 1",
        "User-sequence 2",
        "User-sequence 3",
        "User-sequence 4",
        "DEFAULT"
    }; 
} // namespace oc
