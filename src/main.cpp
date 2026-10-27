#include "mod.hpp"
#include "music_filter.hpp"
#include "GlobalNamespace/AudioTimeSyncController.hpp"
#include "GlobalNamespace/PlayerTransforms.hpp"
#include "UnityEngine/AudioSource.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Object.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "custom-types/shared/register.hpp"
#include <algorithm>
#include <cmath>

static modloader::ModInfo modInfo{"quest-spatial-audio", VERSION, 0};

qsa::Settings qsa::LoadSettings() {
    Configuration config(modInfo);
    config.Load();
    auto& doc = config.config;
    if (!doc.IsObject()) doc.SetObject();
    auto& alloc = doc.GetAllocator();
    if (!doc.HasMember("enabled")) doc.AddMember("enabled", true, alloc);
    // The old narrow-speaker setting is intentionally retired on upgrade.
    // Leaving a persisted 30 degree value active would reproduce the problem.
    if (doc.HasMember("speakerAngleDegrees")) doc.RemoveMember("speakerAngleDegrees");
    if (!doc.HasMember("surroundAmount")) doc.AddMember("surroundAmount", 0.3, alloc);
    if (!doc.HasMember("outputGain")) doc.AddMember("outputGain", 0.5, alloc);
    config.Write();
    Settings settings;
    if (doc["enabled"].IsBool()) settings.enabled = doc["enabled"].GetBool();
    const auto readFloat = [&](const char* key, float fallback, float lo, float hi) {
        if (!doc[key].IsNumber()) return fallback;
        const auto value = doc[key].GetFloat();
        return std::isfinite(value) ? std::clamp(value, lo, hi) : fallback;
    };
    settings.surroundAmount = readFloat("surroundAmount", 0.3f, 0, 0.6f);
    settings.outputGain = readFloat("outputGain", 0.5f, 0, 1);
    return settings;
}

static QuestSpatialAudio::MusicFilter* GetFilter(GlobalNamespace::AudioTimeSyncController* self) {
    auto source = self->____audioSource;
    if (!source) return nullptr;
    return source->get_gameObject()->GetComponent<QuestSpatialAudio::MusicFilter*>();
}

MAKE_HOOK_MATCH(MusicStart, &GlobalNamespace::AudioTimeSyncController::Start, void,
    GlobalNamespace::AudioTimeSyncController* self) {
    const auto settings = qsa::LoadSettings();
    auto source = self->____audioSource;
    if (settings.enabled && source && !GetFilter(self)) {
        auto player = UnityEngine::Object::FindObjectOfType<GlobalNamespace::PlayerTransforms*>();
        if (player && player->____headTransform && player->____originTransform) {
            // A GameObject filter affects every AudioSource on that object.
            // Decline ambiguous/shared objects rather than filtering other sounds.
            const auto sources = source->get_gameObject()->GetComponents<UnityEngine::AudioSource*>();
            if (sources.size() == 1 && source->get_spatialBlend() == 0 && !source->get_spatialize()) {
                auto filter = source->get_gameObject()->AddComponent<QuestSpatialAudio::MusicFilter*>();
                filter->Initialize(player->____headTransform.ptr(), player->____originTransform.ptr(), settings);
            } else {
                Log.warn("Music source is shared or already spatialized; leaving audio unchanged.");
            }
        } else {
            Log.warn("Player transforms unavailable; leaving audio unchanged.");
        }
    }
    MusicStart(self);
}

MAKE_HOOK_MATCH(MusicSeek, &GlobalNamespace::AudioTimeSyncController::SeekTo, void,
    GlobalNamespace::AudioTimeSyncController* self, float offset) {
    if (auto filter = GetFilter(self)) filter->ResetHistory();
    MusicSeek(self, offset);
}

MAKE_HOOK_MATCH(MusicPause, &GlobalNamespace::AudioTimeSyncController::Pause, void,
    GlobalNamespace::AudioTimeSyncController* self) {
    if (auto filter = GetFilter(self)) filter->ResetHistory();
    MusicPause(self);
}

MAKE_HOOK_MATCH(MusicResume, &GlobalNamespace::AudioTimeSyncController::Resume, void,
    GlobalNamespace::AudioTimeSyncController* self) {
    if (auto filter = GetFilter(self)) filter->ResetHistory();
    MusicResume(self);
}

MODLOADER_FUNC void setup(CModInfo* info) noexcept {
    *info = modInfo.to_c();
    Paper::Logger::RegisterFileContextId(Log.tag);
    Log.info("Quest Spatial Audio {}; target Beat Saber 1.40.8_7379.", VERSION);
}

MODLOADER_FUNC void late_load() noexcept {
    il2cpp_functions::Init();
    custom_types::Register::AutoRegister();
    INSTALL_HOOK(Log, MusicStart);
    INSTALL_HOOK(Log, MusicSeek);
    INSTALL_HOOK(Log, MusicPause);
    INSTALL_HOOK(Log, MusicResume);
    Log.info("Music hooks installed. KEMAR HRTF data: Bill Gardner and Keith Martin, MIT Media Lab, 1994.");
}
