// ─────────────────────────────────────────────────────────
// FILE: hal/pc/pc_midi.h
// BRIEF: USB MIDI input via RtMidi
// ─────────────────────────────────────────────────────────
#pragma once
#include "RtMidi.h"
#include "shared/types.h"
#include "shared/interfaces.h"
#include "core/util/spsc_queue.h"
#include <string>
#include <vector>

class PcMidi {
public:
    explicit PcMidi(MidiEventQueue& queue);
    ~PcMidi();

    bool open(int portIndex = -1);   // -1 = first available
    void close();

    std::vector<std::string> listPorts();
    bool        isOpen()       const { return open_; }
    std::string getLastError() const { return lastError_; }

private:
    MidiEventQueue& queue_;
    RtMidiIn        midiIn_;
    bool            open_      = false;
    std::string     lastError_;

    static void midiCallback(double                     deltaTime,
                             std::vector<unsigned char>* msg,
                             void*                       userData);

    MidiEvent parseMessage(const std::vector<unsigned char>& msg) const;
};
