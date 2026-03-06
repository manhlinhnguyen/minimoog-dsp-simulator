// ─────────────────────────────────────────────────────────
// FILE: core/util/spsc_queue.h
// BRIEF: Lock-free Single-Producer Single-Consumer ring buffer
// ─────────────────────────────────────────────────────────
#pragma once
#include <atomic>
#include <array>
#include <cstddef>

// Single-Producer Single-Consumer lock-free queue.
// Producer: MIDI/UI thread  |  Consumer: Audio thread
// Size N MUST be a power of 2.

template<typename T, size_t N>
class SPSCQueue {
    static_assert((N & (N - 1)) == 0, "N must be power of 2");

public:
    // [RT-SAFE] — called from producer thread only
    bool push(const T& item) noexcept {
        const size_t h    = head_.load(std::memory_order_relaxed);
        const size_t next = (h + 1) & (N - 1);
        if (next == tail_.load(std::memory_order_acquire))
            return false;  // full
        buf_[h] = item;
        head_.store(next, std::memory_order_release);
        return true;
    }

    // [RT-SAFE] — called from consumer thread only
    bool pop(T& item) noexcept {
        const size_t t = tail_.load(std::memory_order_relaxed);
        if (t == head_.load(std::memory_order_acquire))
            return false;  // empty
        item = buf_[t];
        tail_.store((t + 1) & (N - 1), std::memory_order_release);
        return true;
    }

    // [RT-SAFE]
    bool isEmpty() const noexcept {
        return tail_.load(std::memory_order_acquire)
            == head_.load(std::memory_order_acquire);
    }

private:
    std::array<T, N>            buf_;
    alignas(64) std::atomic<size_t> head_{0};  // cache line separation
    alignas(64) std::atomic<size_t> tail_{0};
};
