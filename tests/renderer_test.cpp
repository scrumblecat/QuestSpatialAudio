#include "qsa/renderer.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>

static void Check(bool pass, const char* description) {
    if (!pass) { std::fprintf(stderr, "FAIL: %s\n", description); std::exit(1); }
    std::printf("PASS: %s\n", description);
}

static double Energy(const std::vector<float>& data, int ear) {
    double sum = 0;
    for (std::size_t i = ear; i < data.size(); i += 2) sum += data[i] * data[i];
    return sum;
}

static qsa::Scene TurnHead(float radians) {
    auto scene = qsa::kStageScene;
    for (auto& v : scene) {
        const auto x = v.x, z = v.z;
        v.x = std::cos(radians) * x - std::sin(radians) * z;
        v.z = std::sin(radians) * x + std::cos(radians) * z;
    }
    return scene;
}

static std::vector<float> Impulse(const qsa::Scene& scene, int channel, int rate) {
    qsa::Renderer renderer;
    renderer.SetDirections(scene);
    renderer.SetSampleRate(rate);
    renderer.SetSurroundAmount(0); // Isolate the direct localization cue.
    std::vector<float> data(2048, 0);
    data[channel] = 0.25f;
    renderer.Process(data.data(), data.size(), 2);
    return data;
}

int main() {
    for (int rate : {44100, 48000}) {
        const auto left = Impulse(qsa::kStageScene, 0, rate);
        const auto right = Impulse(qsa::kStageScene, 1, rate);
        Check(Energy(left, 0) > Energy(left, 1), "left-only music favors left ear");
        Check(Energy(right, 1) > Energy(right, 0), "right-only music favors right ear");
        bool mirrored = true;
        for (std::size_t i = 0; i < left.size(); i += 2) {
            mirrored &= std::abs(left[i] - right[i + 1]) < 1e-7f;
            mirrored &= std::abs(left[i + 1] - right[i]) < 1e-7f;
        }
        Check(mirrored, "stereo speakers are mirror symmetric");
        // Regression for the user's complaint: the old +/-30 degree speakers
        // both moved to the left. A right-side source must instead move ahead.
        const auto sceneRight = TurnHead(1.570796327f);
        Check(std::abs(sceneRight[qsa::Right].x) < 1e-6f && sceneRight[qsa::Right].z > 0.99f,
            "90-degree right turn puts the old right channel in front");
        const auto turned = Impulse(sceneRight, 1, rate);
        Check(std::abs(Energy(turned, 0) - Energy(turned, 1)) < 1e-8,
            "former right channel reaches both ears equally when facing it");
        const auto sceneLeft = TurnHead(-1.570796327f);
        const auto turnedLeft = Impulse(sceneLeft, 0, rate);
        Check(std::abs(Energy(turnedLeft, 0) - Energy(turnedLeft, 1)) < 1e-8,
            "former left channel reaches both ears equally when facing it");
        Check(sceneRight[qsa::Center].x < -0.99f && sceneRight[qsa::Left].z < -0.99f,
            "the rest of the scene rotates consistently with the right channel");
        auto allFront = qsa::kStageScene, allBack = qsa::kStageScene, allUp = qsa::kStageScene;
        allFront.fill({0, 0, 1});
        allBack.fill({0, 0, -1});
        allUp.fill({0, 1, 0});
        const auto front = Impulse(allFront, 0, rate);
        const auto back = Impulse(allBack, 0, rate);
        Check(front != back, "front and rear use distinct measured filters");
        const auto up = Impulse(allUp, 0, rate);
        Check(front != up, "elevation changes the binaural response");
        auto peak = std::max_element(front.begin(), front.end(), [](float a, float b) { return std::abs(a) < std::abs(b); });
        const double peakMs = (std::distance(front.begin(), peak) / 2) * 1000.0 / rate;
        std::printf("Front HRIR peak at %0.3f ms (%d Hz); no extra block queue\n", peakMs, rate);
        Check(peakMs < 3.0, "filter onset remains below 3 ms");
    }

    qsa::StereoUpmixer upmixer;
    upmixer.Configure(48000);
    bool reconstructs = true, noMonoRear = true;
    qsa::SourceFrame split{};
    for (int i = 0; i < 48000; ++i) {
        const float l = 0.2f * std::sin(i * 0.021f) + 0.1f * std::sin(i * 0.071f);
        const float r = 0.2f * std::sin(i * 0.021f) + 0.1f * std::cos(i * 0.043f);
        split = upmixer.Process(l, r, 0.3f);
        reconstructs &= std::abs(split[qsa::Center] + split[qsa::Left] - l) < 1e-7f;
        reconstructs &= std::abs(split[qsa::Center] + split[qsa::Right] - r) < 1e-7f;
    }
    Check(reconstructs, "center extraction preserves both original channels algebraically");
    upmixer.Reset();
    for (int i = 0; i < 48000; ++i) {
        const float mono = 0.2f * std::sin(i * 0.031f);
        split = upmixer.Process(mono, mono, 0.3f);
        noMonoRear &= split[qsa::SurroundLeft] == 0 && split[qsa::SurroundRight] == 0;
    }
    Check(noMonoRear, "mono material does not create artificial rear ambience");
    Check(std::abs(split[qsa::Left]) < 1e-4f && std::abs(split[qsa::Right]) < 1e-4f,
        "correlated center settles at the stage instead of two lateral copies");
    upmixer.Reset();
    bool noCenterStealing = true;
    double ambientDifference = 0;
    for (int i = 0; i < 4000; ++i) {
        split = upmixer.Process(0, 0.2f * std::sin(i * 0.021f), 0.3f);
        noCenterStealing &= split[qsa::Center] == 0 && split[qsa::Left] == 0;
        ambientDifference += std::abs(split[qsa::SurroundLeft] + split[qsa::SurroundRight]);
    }
    Check(noCenterStealing, "hard-panned material keeps its original side");
    Check(ambientDifference > 1, "rear ambience is decorrelated rather than a mono duplicate");
    upmixer.Reset();
    split = upmixer.Process(0, 0, 0.3f);
    Check(std::all_of(split.begin(), split.end(), [](float v) { return v == 0; }),
        "upmixer reset removes rear delay tails and center steering history");

    qsa::Renderer renderer;
    std::array<float, 16> input{0.1f,-0.2f,0.3f,-0.4f,0.5f,0.6f,-0.7f,0.8f};
    auto data = input;
    renderer.SetEnabled(false);
    renderer.Process(data.data(), data.size(), 2);
    Check(data == input, "disabled renderer bypasses bit-exactly");
    renderer.SetEnabled(true);
    renderer.Process(data.data(), data.size(), 1);
    Check(data == input, "non-stereo input bypasses bit-exactly");
    renderer.SetSampleRate(96000);
    renderer.Process(data.data(), data.size(), 2);
    Check(data == input, "unsupported sample rates bypass bit-exactly");
    renderer.SetSampleRate(48000);
    renderer.SetGain(1);
    std::vector<float> loud(4096, 100);
    renderer.Process(loud.data(), loud.size(), 2);
    Check(std::all_of(loud.begin(), loud.end(), [](float v) { return std::isfinite(v) && std::abs(v) <= 1; }), "output guard bounds extreme input");
    Check(renderer.LimitedSamples() > 0, "output guard reports limited samples");
    renderer.Reset();
    std::vector<float> silence(4096, 0);
    renderer.Process(silence.data(), silence.size(), 2);
    Check(std::all_of(silence.begin(), silence.end(), [](float v) { return v == 0; }), "seek/reset discards audio history");

    std::vector<float> signal(1024);
    auto start = std::chrono::steady_clock::now();
    double time = 0;
    bool sweepFinite = true;
    double fullMixLeft = 0, fullMixRight = 0;
    for (int block = 0; block < 1000; ++block) {
        const float a = block < 200 ? 1.570796327f : (block - 200) * 0.008f;
        renderer.SetDirections(TurnHead(a));
        for (std::size_t i = 0; i < signal.size(); i += 2, time += 1.0 / 48000) {
            signal[i] = 0.3f * std::sin(6.2831853 * 440 * time);
            signal[i + 1] = 0.3f * std::sin(6.2831853 * 660 * time);
        }
        renderer.Process(signal.data(), signal.size(), 2);
        sweepFinite &= std::all_of(signal.begin(), signal.end(), [](float v) { return std::isfinite(v); });
        if (block > 10 && block < 200) {
            fullMixLeft += Energy(signal, 0);
            fullMixRight += Energy(signal, 1);
        }
    }
    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    Check(sweepFinite, "rotation sweep stays finite");
    const double earRatio = std::min(fullMixLeft, fullMixRight) / std::max(fullMixLeft, fullMixRight);
    std::printf("Full stereo mix at 90-degree right turn: ear energy ratio %.3f\n", earRatio);
    Check(earRatio > 0.5, "stereo mix does not collapse into one ear at a 90-degree turn");
    std::printf("Benchmark: %.3f seconds to render %.3f seconds of 48k stereo audio (%.1f%% of one core)\n", elapsed, 512000.0 / 48000, elapsed / (512000.0 / 48000) * 100);
    Check(elapsed < 512000.0 / 48000, "renderer runs faster than real time on this device");
    std::puts("All DSP checks passed.");
}
