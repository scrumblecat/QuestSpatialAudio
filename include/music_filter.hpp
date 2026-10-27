#pragma once
#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/Transform.hpp"
#include "custom-types/shared/macros.hpp"
#include "qsa/renderer.hpp"
#include "mod.hpp"

DECLARE_CLASS_CODEGEN(QuestSpatialAudio, MusicFilter, UnityEngine::MonoBehaviour) {
    DECLARE_DEFAULT_CTOR();
    DECLARE_INSTANCE_FIELD(UnityEngine::Transform*, head);
    DECLARE_INSTANCE_FIELD(UnityEngine::Transform*, origin);
    DECLARE_INSTANCE_METHOD(void, LateUpdate);
    DECLARE_INSTANCE_METHOD(void, OnAudioFilterRead, ArrayW<float> data, int channels);
    DECLARE_INSTANCE_METHOD(void, OnDisable);
    DECLARE_INSTANCE_METHOD(void, OnDestroy);
public:
    void Initialize(UnityEngine::Transform* headTransform, UnityEngine::Transform* originTransform, qsa::Settings settings);
    void ResetHistory() noexcept { renderer.Reset(); }
    qsa::Renderer renderer;
    float diagnosticTime = 0;
    bool reported = false;
};
