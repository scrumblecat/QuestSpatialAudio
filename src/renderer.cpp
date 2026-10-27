#include "qsa/renderer.hpp"
#include <algorithm>
#include <cmath>

namespace qsa {
std::uint16_t FindResponse(Direction d) noexcept {
    if (!std::isfinite(d.x) || !std::isfinite(d.y) || !std::isfinite(d.z) ||
        d.x * d.x + d.y * d.y + d.z * d.z < 1e-8f) d = {0, 0, 1};
    const bool mirrored = d.x < 0;
    d.x = std::abs(d.x);
    float best = -1e30f;
    std::uint16_t index = 0;
    for (std::uint16_t i = 0; i < kResponseCount; ++i) {
        const auto& v = kHrtfResponses[i].direction;
        const float score = d.x * v.x + d.y * v.y + d.z * v.z;
        if (score > best) { best = score; index = i; }
    }
    return index | (mirrored ? 0x8000u : 0u);
}

Renderer::Renderer() noexcept { SetDirections(kStageScene); }

void Renderer::SetDirections(const Scene& scene) noexcept {
    // Five 10-bit keys fit in one lock-free atomic publication, so every
    // speaker uses the same head pose even if an audio callback interrupts us.
    std::uint64_t pose = 0;
    for (std::size_t s = 0; s < SourceCount; ++s) {
        const auto response = FindResponse(scene[s]);
        const auto key = (response & 0x1ffu) | ((response & 0x8000u) ? 0x200u : 0u);
        pose |= std::uint64_t(key) << (10 * s);
    }
    pose_.store(pose, std::memory_order_relaxed);
}

void Renderer::SetGain(float gain) noexcept {
    gain_.store(std::isfinite(gain) ? std::clamp(gain, 0.0f, 1.0f) : 0.5f, std::memory_order_relaxed);
}

void Renderer::SetSurroundAmount(float amount) noexcept {
    surroundAmount_.store(std::isfinite(amount) ? std::clamp(amount, 0.0f, 0.6f) : 0.3f, std::memory_order_relaxed);
}

void Renderer::LoadKernels(std::uint64_t pose, int rate) noexcept {
    taps_ = rate == 44100 ? 128 : kMaxTaps;
    for (std::size_t speaker = 0; speaker < SourceCount; ++speaker) {
        const std::uint16_t key = (pose >> (speaker * 10)) & 0x3ffu;
        const auto& response = kHrtfResponses[key & 0x1ffu];
        const bool mirror = key & 0x200u;
        const auto* l = rate == 44100 ? response.left44100 : response.left48000;
        const auto* r = rate == 44100 ? response.right44100 : response.right48000;
        for (std::size_t t = 0; t < kMaxTaps; ++t) {
            target_[speaker * 2][t] = t < taps_ ? (mirror ? r[t] : l[t]) / 32768.0f : 0;
            target_[speaker * 2 + 1][t] = t < taps_ ? (mirror ? l[t] : r[t]) / 32768.0f : 0;
        }
    }
    activePose_ = pose;
    activeRate_ = rate;
}

void Renderer::Process(float* data, std::size_t count, int channels) noexcept {
    blocks_.fetch_add(1, std::memory_order_relaxed);
    channels_.store(channels, std::memory_order_relaxed);
    const int rate = rate_.load(std::memory_order_relaxed);
    if (!data || count == 0 || channels != 2 || count % 2 ||
        (rate != 44100 && rate != 48000) || !enabled_.load(std::memory_order_relaxed)) {
        // Bypass is bit-exact. Never replay old history when enabled again.
        primed_ = false;
        return;
    }
    const auto pose = pose_.load(std::memory_order_relaxed);
    const auto reset = reset_.load(std::memory_order_relaxed);
    if (reset != activeReset_ || rate != activeRate_) primed_ = false;
    if (pose != activePose_ || rate != activeRate_) LoadKernels(pose, rate);
    if (!primed_) {
        current_ = target_;
        for (auto& history : history_) history.fill(0);
        upmixer_.Configure(rate);
        cursor_ = 0;
        activeReset_ = reset;
        primed_ = true;
    }
    // Exponential coefficient smoothing across callbacks (~8 ms) avoids clicks
    // at measurement boundaries, even for buffers shorter than a game frame.
    const float smoothing = 1.0f - std::exp(-1.0f / (0.008f * rate));
    const float gain = gain_.load(std::memory_order_relaxed);
    const float surround = surroundAmount_.load(std::memory_order_relaxed);
    std::uint32_t limited = 0;
    for (std::size_t frame = 0; frame < count; frame += 2) {
        const float inL = std::isfinite(data[frame]) ? data[frame] : 0.0f;
        const float inR = std::isfinite(data[frame + 1]) ? data[frame + 1] : 0.0f;
        const auto sources = upmixer_.Process(inL, inR, surround);
        for (std::size_t s = 0; s < SourceCount; ++s)
            history_[s][cursor_] = history_[s][cursor_ + kMaxTaps] = sources[s];
        float outL = 0, outR = 0;
        for (std::size_t t = 0; t < taps_; ++t) {
            for (std::size_t s = 0; s < SourceCount; ++s) {
                auto& l = current_[2 * s][t];
                auto& r = current_[2 * s + 1][t];
                l += smoothing * (target_[2 * s][t] - l);
                r += smoothing * (target_[2 * s + 1][t] - r);
                const float sample = history_[s][cursor_ + t];
                outL += sample * l;
                outR += sample * r;
            }
        }
        outL *= gain;
        outR *= gain;
        limited += std::abs(outL) > 1.0f;
        limited += std::abs(outR) > 1.0f;
        data[frame] = std::clamp(outL, -1.0f, 1.0f);
        data[frame + 1] = std::clamp(outR, -1.0f, 1.0f);
        cursor_ = cursor_ ? cursor_ - 1 : kMaxTaps - 1;
    }
    limited_.fetch_add(limited, std::memory_order_relaxed);
}
}
