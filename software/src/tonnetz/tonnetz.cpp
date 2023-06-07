#include "tonnetz/tonnetz.h"

using namespace tonnetz;

abstract_triad tonnetz::apply_transformation(ETransformType type,
                                    const abstract_triad &source) {
  const tonnetz::transformation &t = tonnetz::transformations[type][source.mode()];

  abstract_triad result = source;
  result.change_mode();
  result.apply_offsets(t.offsets);
  result.shift_root(t.root_shift);
  return result;
}