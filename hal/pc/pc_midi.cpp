// ─────────────────────────────────────────────────────────
// FILE: hal/pc/pc_midi.cpp
// BRIEF: RtMidi USB MIDI input implementation
// ─────────────────────────────────────────────────────────
#include "pc_midi.h"
#include <iostream>

PcMidi::PcMidi(MidiEventQueue& queue) : queue_(queue) {}

PcMidi::~PcMidi() { close(); }

bool PcMidi::open(int portIndex) {
    try {
        const unsigned int count = midiIn_.getPortCount();
        if (count == 0) {
            lastError_ = "No MIDI input ports found";
            return false;
        }

        const unsigned int idx = (portIndex < 0 ||
                                   portIndex >= static_cast<int>(count))
                               ? 0u
                               : static_cast<unsigned int>(portIndex);

        midiIn_.openPort(idx);
        midiIn_.setCallback(&PcMidi::midiCallback,
                            static_cast<void*>(this));
        midiIn_.ignoreTypes(true, false, true);  // ignore sysex, active sensing
        open_ = true;
    } catch (RtMidiError& e) {
        lastError_ = e.getMessage();
        return false;
    }
    return true;
}

void PcMidi::close() {
    if (open_) {
        midiIn_.closePort();
        open_ = false;
    }
}

std::vector<std::string> PcMidi::listPorts() {
    std::vector<std::string> ports;
    const unsigned int count = midiIn_.getPortCount();
    for (unsigned int i = 0; i < count; ++i)
        ports.push_back(midiIn_.getPortName(i));
    return ports;
}

// ─────────────────────────────────────────────────────────
// MIDI CALLBACK (RtMidi thread)
// ─────────────────────────────────────────────────────────

void PcMidi::midiCallback(double /*deltaTime*/,
                           std::vector<unsigned char>* msg,
                           void*  userData) {
    auto* self = static_cast<PcMidi*>(userData);
    if (!msg || msg->empty()) return;

    MidiEvent ev = self->parseMessage(*msg);
    if (ev.type != MidiEvent::Type::Invalid)
        self->queue_.push(ev);
}

MidiEvent PcMidi::parseMessage(
        const std::vector<unsigned char>& msg) const {
    MidiEvent ev;
    if (msg.empty()) return ev;

    const uint8_t status  = msg[0];
    const uint8_t type    = status & 0xF0;
    const uint8_t channel = status & 0x0F;
    ev.channel = channel;

    switch (type) {
        case 0x90:
            if (msg.size() >= 3) {
                ev.type  = MidiEvent::Type::NoteOn;
                ev.data1 = msg[1];
                ev.data2 = msg[2];
            }
            break;
        case 0x80:
            if (msg.size() >= 3) {
                ev.type  = MidiEvent::Type::NoteOff;
                ev.data1 = msg[1];
                ev.data2 = 0;
            }
            break;
        case 0xB0:
            if (msg.size() >= 3) {
                ev.type  = MidiEvent::Type::ControlChange;
                ev.data1 = msg[1];
                ev.data2 = msg[2];
            }
            break;
        case 0xE0:
            if (msg.size() >= 3) {
                ev.type      = MidiEvent::Type::PitchBend;
                const int raw = (msg[2] << 7) | msg[1];
                ev.pitchBend  = static_cast<int16_t>(raw - 8192);
            }
            break;
        case 0xD0:
            if (msg.size() >= 2) {
                ev.type  = MidiEvent::Type::Aftertouch;
                ev.data1 = msg[1];
            }
            break;
        case 0xC0:
            if (msg.size() >= 2) {
                ev.type  = MidiEvent::Type::ProgramChange;
                ev.data1 = msg[1];
            }
            break;
        case 0xF0:
            if (status == 0xFA)      ev.type = MidiEvent::Type::Start;
            else if (status == 0xFC) ev.type = MidiEvent::Type::Stop;
            else if (status == 0xFB) ev.type = MidiEvent::Type::Continue;
            break;
        default: break;
    }
    return ev;
}
