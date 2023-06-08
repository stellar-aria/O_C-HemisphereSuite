#include "hemisphere/preset.hpp"

using namespace hemisphere;

constexpr size_t as_idx(Setting s) { return static_cast<size_t>(s); }

int Preset::GetAppletId(int h) {
  Setting setting_idx = (h == LEFT_HEMISPHERE) ? Setting::SELECTED_LEFT_ID
                                               : Setting::SELECTED_RIGHT_ID;
  return values_[as_idx(setting_idx)];
}
void Preset::SetAppletId(int h, int id) { apply_value(h, id); }
bool Preset::is_valid() {
  return values_[as_idx(Setting::SELECTED_LEFT_ID)] != 0;
}

// restore state by setting applets and giving them data
void Preset::LoadClockData() {
  hemisphere::clock_setup_applet.OnDataReceive(
      0,
      (static_cast<uint64_t>(values_[as_idx(Setting::CLOCK_DATA4)]) << 48) |
          (static_cast<uint64_t>(values_[as_idx(Setting::CLOCK_DATA3)]) << 32) |
          (static_cast<uint64_t>(values_[as_idx(Setting::CLOCK_DATA2)]) << 16) |
          static_cast<uint64_t>(values_[as_idx(Setting::CLOCK_DATA1)]));
}
void Preset::StoreClockData() {
  uint64_t data = hemisphere::clock_setup_applet.OnDataRequest(0);
  apply_value(as_idx(Setting::CLOCK_DATA1), data & 0xffff);
  apply_value(as_idx(Setting::CLOCK_DATA2), (data >> 16) & 0xffff);
  apply_value(as_idx(Setting::CLOCK_DATA3), (data >> 32) & 0xffff);
  apply_value(as_idx(Setting::CLOCK_DATA4), (data >> 48) & 0xffff);
}

// Manually get data for one side
uint64_t Preset::GetData(int h) {
  return (static_cast<uint64_t>(values_[8 + h]) << 48) |
         (static_cast<uint64_t>(values_[6 + h]) << 32) |
         (static_cast<uint64_t>(values_[4 + h]) << 16) |
         (static_cast<uint64_t>(values_[2 + h]));
}

/* Manually store state data for one side */
void Preset::SetData(int h, uint64_t data) {
  apply_value(2 + h, data & 0xffff);
  apply_value(4 + h, (data >> 16) & 0xffff);
  apply_value(6 + h, (data >> 32) & 0xffff);
  apply_value(8 + h, (data >> 48) & 0xffff);
}

// TODO: I haven't updated the SysEx data structure here because I don't use
// it. Clock data would probably be useful if it's not too big. -NJM
void Preset::OnSendSysEx() {
  // Describe the data structure for the audience
  uint8_t V[18];
  V[0] = static_cast<uint8_t>(values_[as_idx(Setting::SELECTED_LEFT_ID)]);
  V[1] = static_cast<uint8_t>(values_[as_idx(Setting::SELECTED_RIGHT_ID)]);
  V[2] = static_cast<uint8_t>(values_[as_idx(Setting::LEFT_DATA_B1)] & 0xff);
  V[3] = static_cast<uint8_t>((values_[as_idx(Setting::LEFT_DATA_B1)] >> 8) &
                              0xff);
  V[4] = static_cast<uint8_t>(values_[as_idx(Setting::RIGHT_DATA_B1)] & 0xff);
  V[5] = static_cast<uint8_t>((values_[as_idx(Setting::RIGHT_DATA_B1)] >> 8) &
                              0xff);
  V[6] = static_cast<uint8_t>(values_[as_idx(Setting::LEFT_DATA_B2)] & 0xff);
  V[7] = static_cast<uint8_t>((values_[as_idx(Setting::LEFT_DATA_B2)] >> 8) &
                              0xff);
  V[8] = static_cast<uint8_t>(values_[as_idx(Setting::RIGHT_DATA_B2)] & 0xff);
  V[9] = static_cast<uint8_t>((values_[as_idx(Setting::RIGHT_DATA_B2)] >> 8) &
                              0xff);
  V[10] = static_cast<uint8_t>(values_[as_idx(Setting::LEFT_DATA_B3)] & 0xff);
  V[11] = static_cast<uint8_t>((values_[as_idx(Setting::LEFT_DATA_B3)] >> 8) &
                               0xff);
  V[12] = static_cast<uint8_t>(values_[as_idx(Setting::RIGHT_DATA_B3)] & 0xff);
  V[13] = static_cast<uint8_t>((values_[as_idx(Setting::RIGHT_DATA_B3)] >> 8) &
                               0xff);
  V[14] = static_cast<uint8_t>(values_[as_idx(Setting::LEFT_DATA_B4)] & 0xff);
  V[15] = static_cast<uint8_t>((values_[as_idx(Setting::LEFT_DATA_B4)] >> 8) &
                               0xff);
  V[16] = static_cast<uint8_t>(values_[as_idx(Setting::RIGHT_DATA_B4)] & 0xff);
  V[17] = static_cast<uint8_t>((values_[as_idx(Setting::RIGHT_DATA_B4)] >> 8) &
                               0xff);

  // Pack it up, ship it out
  UnpackedData unpacked;
  unpacked.set_data(18, V);
  PackedData packed = unpacked.pack();
  SendSysEx(packed, 'H');
}

void Preset::OnReceiveSysEx() {
  uint8_t V[18];
  if (ExtractSysExData(V, 'H')) {
    values_[as_idx(Setting::SELECTED_LEFT_ID)] = V[0];
    values_[as_idx(Setting::SELECTED_RIGHT_ID)] = V[1];
    values_[as_idx(Setting::LEFT_DATA_B1)] =
        (static_cast<uint16_t>(V[3]) << 8) + V[2];
    values_[as_idx(Setting::RIGHT_DATA_B1)] =
        (static_cast<uint16_t>(V[5]) << 8) + V[4];
    values_[as_idx(Setting::LEFT_DATA_B2)] =
        (static_cast<uint16_t>(V[7]) << 8) + V[6];
    values_[as_idx(Setting::RIGHT_DATA_B2)] =
        (static_cast<uint16_t>(V[9]) << 8) + V[8];
    values_[as_idx(Setting::LEFT_DATA_B3)] =
        (static_cast<uint16_t>(V[11]) << 8) + V[10];
    values_[as_idx(Setting::RIGHT_DATA_B3)] =
        (static_cast<uint16_t>(V[13]) << 8) + V[12];
    values_[as_idx(Setting::LEFT_DATA_B4)] =
        (static_cast<uint16_t>(V[15]) << 8) + V[14];
    values_[as_idx(Setting::RIGHT_DATA_B4)] =
        (static_cast<uint16_t>(V[17]) << 8) + V[16];
    // LoadClockData();
  }
}