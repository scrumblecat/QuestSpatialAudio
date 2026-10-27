#pragma once
#include <cstddef>
#include <cstdint>

namespace qsa {
struct Direction { float x, y, z; }; // right, up, forward in listener space
inline constexpr std::size_t kResponseCount = 368;
inline constexpr std::size_t kMaxTaps = 140;
struct HrtfResponse {
    Direction direction;
    std::int16_t left44100[128], right44100[128];
    std::int16_t left48000[kMaxTaps], right48000[kMaxTaps];
};
extern const HrtfResponse kHrtfResponses[kResponseCount];

// Bit 15 mirrors the measured right hemisphere, swapping ears.
std::uint16_t FindResponse(Direction direction) noexcept;
}
