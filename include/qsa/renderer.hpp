#pragma once
#include "qsa/hrtf_data.hpp"
#include "qsa/upmixer.hpp"
#include <array>
#include <atomic>
#include <cstdint>

namespace qsa {
// One main-thread producer; one Unity audio-thread consumer. The callback never
// allocates, locks, logs, calls Unity, or changes the game's playback clock.
class Renderer {
public:
    Renderer() noexcept;
    void SetDirections(const Scene& scene) noexcept;
    void SetEnabled(bool enabled) noexcept { enabled_.store(enabled, std::memory_order_relaxed); }
    void SetSampleRate(int rate) noexcept { rate_.store(rate, std::memory_order_relaxed); }
    void SetGain(float gain) noexcept;
    void SetSurroundAmount(float amount) noexcept;
    void Reset() noexcept { reset_.fetch_add(1, std::memory_order_relaxed); }
    void Process(float* interleaved, std::size_t sampleCount, int channels) noexcept;
    std::uint32_t Blocks() const noexcept { return blocks_.load(std::memory_order_relaxed); }
    std::uint32_t LimitedSamples() const noexcept { return limited_.load(std::memory_order_relaxed); }
    int LastChannels() const noexcept { return channels_.load(std::memory_order_relaxed); }

private:
    using Kernel = std::array<float, kMaxTaps>;
    void LoadKernels(std::uint64_t pose, int rate) noexcept;
    std::atomic<std::uint64_t> pose_{0};
    std::atomic<int> rate_{48000};
    std::atomic<bool> enabled_{true};
    std::atomic<float> gain_{0.5f};
    std::atomic<float> surroundAmount_{0.3f};
    std::atomic<std::uint32_t> reset_{0}, blocks_{0}, limited_{0};
    std::atomic<int> channels_{0};
    std::array<Kernel, 2 * SourceCount> current_{}, target_{};
    // Doubled circular histories keep the FIR dot products contiguous for SIMD.
    std::array<std::array<float, 2 * kMaxTaps>, SourceCount> history_{};
    StereoUpmixer upmixer_;
    std::uint64_t activePose_{~std::uint64_t(0)};
    std::uint32_t activeReset_{0};
    int activeRate_{0};
    std::size_t cursor_{0}, taps_{kMaxTaps};
    bool primed_{false};
};
static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
static_assert(std::atomic<float>::is_always_lock_free);
}
