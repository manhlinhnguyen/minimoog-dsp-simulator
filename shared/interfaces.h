// ─────────────────────────────────────────────────────────
// FILE: shared/interfaces.h
// BRIEF: AtomicParamStore, MidiEventQueue, IAudioProcessor
// ─────────────────────────────────────────────────────────
#pragma once
#include "types.h"
#include "params.h"
#include "core/util/spsc_queue.h"
#include <atomic>
#include <array>

// ════════════════════════════════════════════════════════
// ATOMIC PARAMETER STORE
// Thread-safe: UI thread writes, Audio thread reads.
// Uses std::atomic<float> with memory_order_relaxed —
// no mutex, no blocking.
// ════════════════════════════════════════════════════════

class AtomicParamStore {
public:
    AtomicParamStore() {
        for (int i = 0; i < PARAM_COUNT; ++i)
            values_[i].store(PARAM_META[i].defaultVal,
                             std::memory_order_relaxed);
    }

    // [RT-SAFE] — callable from any thread
    float get(int id) const noexcept {
        return values_[id].load(std::memory_order_relaxed);
    }

    void set(int id, float val) noexcept {
        values_[id].store(val, std::memory_order_relaxed);
    }

    // [RT-SAFE] — batch read into plain array (once per block)
    void snapshot(float out[PARAM_COUNT]) const noexcept {
        for (int i = 0; i < PARAM_COUNT; ++i)
            out[i] = values_[i].load(std::memory_order_relaxed);
    }

    void resetToDefaults() noexcept {
        for (int i = 0; i < PARAM_COUNT; ++i)
            values_[i].store(PARAM_META[i].defaultVal,
                             std::memory_order_relaxed);
    }

private:
    std::atomic<float> values_[PARAM_COUNT];
    static_assert(std::atomic<float>::is_always_lock_free,
                  "atomic<float> must be lock-free on this platform");
};

// ════════════════════════════════════════════════════════
// MIDI EVENT QUEUE — SPSC lock-free ring buffer
// Producer: UI/MIDI thread  |  Consumer: Audio thread
// ════════════════════════════════════════════════════════

using MidiEventQueue = SPSCQueue<MidiEvent, 256>;

// ════════════════════════════════════════════════════════
// AUDIO PROCESSOR INTERFACE
// ════════════════════════════════════════════════════════

class IAudioProcessor {
public:
    virtual ~IAudioProcessor() = default;

    // [RT-SAFE] — called by audio backend every block.
    // MUST NOT allocate, lock, or throw.
    virtual void processBlock(sample_t* outL,
                              sample_t* outR,
                              int nFrames) noexcept = 0;

    virtual void setSampleRate(float sr) = 0;
    virtual void setBlockSize(int bs)    = 0;
};
