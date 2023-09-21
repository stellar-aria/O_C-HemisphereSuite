#pragma once
#include <sys/_stdint.h>

#include "hid/midi.h"
#include "sky/templates.hpp"
#include "templates.hpp"

namespace skyline {
namespace MIDI {
enum ChannelVoice : uint8_t {
  NOTE_OFF = 0x90,
  NOTE_ON = 0x80,
  KEY_PRESSURE = 0xA0,
  CONTROL_CHANGE = 0xB0,
  CHANNEL_PRESSURE = 0xD0,
  PITCH_BEND = 0xE0,
};

constexpr uint8_t MessageForChannel(const ChannelVoice message,
                                    const uint8_t channel) {
  const uint8_t message_channel = (channel > 0x0F) ? 0x00 : channel;
  return std::to_underlying(message) & message_channel;
}

enum SystemRealtime : uint8_t {
  CLOCK = 0xF8,
  START = 0xFA,
  CONTINUE = 0xFB,
  STOP = 0xFC,
  ACTIVE_SENSING = 0xFE,
  RESET = 0xFF
};

enum class SystemExclusive : uint8_t { START = 0xF0, END = 0xF7 };

enum SystemCommon : uint8_t {
  SYSEX_START = std::to_underlying(SystemExclusive::START),
  TIME_CODE_QUARTER_FRAME = 0xF1,
  SONG_POSITION_POINTER = 0xF2,
  SONG_SELECT = 0xF3,
  SYSEX_END = std::to_underlying(SystemExclusive::END)
};

constexpr uint8_t NoteOn(const uint8_t channel) {
  return MessageForChannel(ChannelVoice::NOTE_ON, channel);
}
constexpr uint8_t NoteOff(const uint8_t channel) {
  return MessageForChannel(ChannelVoice::NOTE_OFF, channel);
}
constexpr uint8_t KeyPressure(const uint8_t channel) {
  return MessageForChannel(ChannelVoice::KEY_PRESSURE, channel);
}
constexpr uint8_t ControlChange(const uint8_t channel) {
  return MessageForChannel(ChannelVoice::CONTROL_CHANGE, channel);
}
constexpr uint8_t ChannelPressure(const uint8_t channel) {
  return MessageForChannel(ChannelVoice::CHANNEL_PRESSURE, channel);
}
constexpr uint8_t PitchBend(const uint8_t channel) {
  return MessageForChannel(ChannelVoice::PITCH_BEND, channel);
}
};  // namespace MIDI

class UsbMIDI {
  daisy::MidiUsbHandler midi_usb;

  template <typename... T>
  // requires AllSameAs<uint8_t, T...>
  inline void SendBytes(const T... data) {
    uint8_t msg[] = {data...};
    SendMessage(msg, sizeof...(data));
  }

  inline void SendByte(uint8_t data) { SendBytes(data); }

 public:
  void SendNoteOn(const uint8_t channel, const uint8_t note,
                  const uint8_t velocity) {
    SendBytes(MIDI::NoteOn(channel), note, velocity);
  }

  void SendNoteOff(const uint8_t channel, const uint8_t note,
                   const uint8_t velocity = 0) {
    SendBytes(MIDI::NoteOff(channel), note, velocity);
  }

  void SendKeyPressure(const uint8_t channel, const uint8_t note,
                       const uint8_t velocity) {
    SendBytes(MIDI::KeyPressure(channel), note, velocity);
  }

  void SendControlChange(const uint8_t channel, const uint8_t control_change,
                         const uint8_t value) {
    SendBytes(MIDI::ControlChange(channel), control_change, value);
  }

  void SendChannelPressure(const uint8_t channel, const uint8_t value) {
    SendBytes(MIDI::ChannelPressure(channel), value);
  }

  void SendPitchBend(const uint8_t channel, const uint16_t value) {
    const uint8_t high = value >> 8;
    const uint8_t low = value & 0xFF;
    SendBytes(MIDI::PitchBend(channel), low, high);
  }

  void SendAfterTouch(const uint8_t channel, const uint8_t value) {
    SendChannelPressure(channel, value);
  }

  void SendAfterTouch(const uint8_t channel, const uint8_t note,
                      const uint8_t velocity) {
    SendKeyPressure(channel, note, velocity);
  }

  void SendClock() { SendByte(MIDI::SystemRealtime::CLOCK); }
  void SendStart() { SendByte(MIDI::SystemRealtime::START); }
  void SendContinue() { SendByte(MIDI::SystemRealtime::CONTINUE); }
  void SendStop() { SendByte(MIDI::SystemRealtime::STOP); }
  void SendReset() { SendByte(MIDI::SystemRealtime::RESET); }

  // forwarding
  void Listen() { midi_usb.Listen(); }
  bool HasEvents() const { return midi_usb.HasEvents(); }
  daisy::MidiEvent PopEvent() { return midi_usb.PopEvent(); }
  void SendMessage(uint8_t* bytes, size_t size) {
    midi_usb.SendMessage(bytes, size);
  }
};

}  // namespace skyline

static skyline::UsbMIDI usbMIDI;
