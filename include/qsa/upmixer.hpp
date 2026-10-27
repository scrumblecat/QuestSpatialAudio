#pragma once
#include "qsa/hrtf_data.hpp"
#include <array>
#include <cstddef>

namespace qsa {
// These are mix components, not isolated instruments. Channel order is shared
// by the main-thread pose publisher and the audio-thread binaural renderer.
enum Source : std::size_t { Center, Left, Right, SurroundLeft, SurroundRight, SourceCount };
static_assert(kResponseCount <= 512 && SourceCount * 10 <= 64);
using Scene = std::array<Direction, SourceCount>;
using SourceFrame = std::array<float, SourceCount>;
inline constexpr Scene kStageScene{{
    {0, 0, 1}, {-1, 0, 0}, {1, 0, 0},
    {-0.70710678f, 0, -0.70710678f}, {0.70710678f, 0, -0.70710678f}
}};

// Low-latency broadband center extraction plus decorrelated stereo-difference
// ambience. The direct signal is never delayed by a window or a block queue.
class StereoUpmixer {
public:
    void Configure(int sampleRate) noexcept;
    void Reset() noexcept;
    SourceFrame Process(float left, float right, float surroundAmount) noexcept;

private:
    class Allpass {
    public:
        void SetLength(std::size_t length) noexcept;
        void Reset() noexcept;
        float Process(float input) noexcept;
    private:
        std::array<float, 512> delay_{};
        std::size_t cursor_ = 0, length_ = 167;
    };
    float leftPower_ = 0, rightPower_ = 0, covariance_ = 0;
    float analysisSmoothing_ = 0.001f, steeringSmoothing_ = 0.001f;
    float fromLeft_ = 0, fromRight_ = 0;
    std::array<Allpass, 4> decorrelators_{};
};
}
