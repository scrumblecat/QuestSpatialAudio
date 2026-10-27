#include "music_filter.hpp"
#include "UnityEngine/AudioSettings.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/Time.hpp"
#include "UnityEngine/Vector3.hpp"
#include <cmath>

DEFINE_TYPE(QuestSpatialAudio, MusicFilter);

void QuestSpatialAudio::MusicFilter::Initialize(UnityEngine::Transform* h, UnityEngine::Transform* o, qsa::Settings settings) {
    head = h;
    origin = o;
    renderer.SetSampleRate(UnityEngine::AudioSettings::get_outputSampleRate());
    renderer.SetGain(settings.outputGain);
    renderer.SetSurroundAmount(settings.surroundAmount);
    renderer.SetEnabled(settings.enabled);
    LateUpdate();
    Log.info("Attached stereo surround upmixer: sampleRate={}, surroundAmount={}, gain={}",
        UnityEngine::AudioSettings::get_outputSampleRate(), settings.surroundAmount, settings.outputGain);
}

void QuestSpatialAudio::MusicFilter::LateUpdate() {
    if (!head || !origin || !UnityEngine::Object::IsNativeObjectAlive(head) || !UnityEngine::Object::IsNativeObjectAlive(origin)) {
        renderer.SetEnabled(false);
        return;
    }
    // Use the play-space basis, never the current gaze direction. Only rotation
    // contributes: dodging does not change loudness, pitch, or speaker distance.
    qsa::Scene scene;
    for (std::size_t s = 0; s < qsa::SourceCount; ++s) {
        const auto& v = qsa::kStageScene[s];
        const auto local = head->InverseTransformDirection(origin->TransformDirection(UnityEngine::Vector3(v.x, v.y, v.z)));
        scene[s] = {local.x, local.y, local.z};
    }
    renderer.SetDirections(scene);
    renderer.SetSampleRate(UnityEngine::AudioSettings::get_outputSampleRate());
    diagnosticTime += UnityEngine::Time::get_unscaledDeltaTime();
    if (!reported && diagnosticTime > 3.0f) {
        Log.info("Audio callback diagnostic: blocks={}, channels={}, limitedSamples={}",
            renderer.Blocks(), renderer.LastChannels(), renderer.LimitedSamples());
        if (renderer.Blocks() == 0) Log.warn("No audio callbacks yet; verify during song playback.");
        if (renderer.LastChannels() != 2 && renderer.Blocks() > 0) Log.warn("Non-stereo audio bypassed unchanged.");
        reported = true;
    }
}

void QuestSpatialAudio::MusicFilter::OnAudioFilterRead(ArrayW<float> data, int channels) {
    renderer.Process(data.begin(), data.size(), channels);
}

void QuestSpatialAudio::MusicFilter::OnDisable() { renderer.Reset(); }

void QuestSpatialAudio::MusicFilter::OnDestroy() {
    Log.info("Music filter finished: blocks={}, channels={}, limitedSamples={}",
        renderer.Blocks(), renderer.LastChannels(), renderer.LimitedSamples());
    // All native state is inline and trivially destructible. No state is freed
    // while an audio callback could still hold a pointer to it.
}
