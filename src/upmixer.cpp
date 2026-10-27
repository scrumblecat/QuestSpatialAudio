#include "qsa/upmixer.hpp"
#include <algorithm>
#include <cmath>

namespace qsa {
void StereoUpmixer::Allpass::SetLength(std::size_t length) noexcept {
    length_ = std::clamp(length, std::size_t(1), delay_.size());
    Reset();
}
void StereoUpmixer::Allpass::Reset() noexcept { delay_.fill(0); cursor_ = 0; }
float StereoUpmixer::Allpass::Process(float input) noexcept {
    // Schroeder allpass: unit magnitude, different phase/delay for each rear.
    const float output = delay_[cursor_] - 0.5f * input;
    delay_[cursor_] = input + 0.5f * output;
    cursor_ = (cursor_ + 1) % length_;
    return output;
}

void StereoUpmixer::Configure(int rate) noexcept {
    analysisSmoothing_ = 1.0f - std::exp(-1.0f / (0.020f * rate));
    steeringSmoothing_ = 1.0f - std::exp(-1.0f / (0.030f * rate));
    constexpr std::array<int, 4> delays{167, 307, 229, 419};
    for (std::size_t i = 0; i < delays.size(); ++i)
        decorrelators_[i].SetLength(std::size_t(std::lround(delays[i] * rate / 48000.0f)));
    Reset();
}

void StereoUpmixer::Reset() noexcept {
    leftPower_ = rightPower_ = covariance_ = fromLeft_ = fromRight_ = 0;
    for (auto& filter : decorrelators_) filter.Reset();
}

SourceFrame StereoUpmixer::Process(float l, float r, float surroundAmount) noexcept {
    leftPower_ += analysisSmoothing_ * (l * l - leftPower_);
    rightPower_ += analysisSmoothing_ * (r * r - rightPower_);
    covariance_ += analysisSmoothing_ * (l * r - covariance_);
    // Positive shared energy is a center candidate. Negative/uncorrelated
    // energy stays in the lateral channels. Smooth steering independently of
    // the analysis so music cannot make its apparent position chatter.
    const float common = std::max(0.0f, covariance_);
    const float targetL = std::clamp(common / (leftPower_ + 1e-12f), 0.0f, 1.0f);
    const float targetR = std::clamp(common / (rightPower_ + 1e-12f), 0.0f, 1.0f);
    fromLeft_ += steeringSmoothing_ * (targetL - fromLeft_);
    fromRight_ += steeringSmoothing_ * (targetR - fromRight_);
    const float center = 0.5f * (fromLeft_ * l + fromRight_ * r);
    const float side = 0.5f * (l - r);
    const float ambientL = decorrelators_[1].Process(decorrelators_[0].Process(side));
    const float ambientR = decorrelators_[3].Process(decorrelators_[2].Process(-side));
    return {center, l - center, r - center, surroundAmount * ambientL, surroundAmount * ambientR};
}
}
