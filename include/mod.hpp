#pragma once
#include "scotland2/shared/modloader.h"
#include "paper2_scotland2/shared/logger.hpp"

inline constexpr auto Log = Paper::ConstLoggerContext("quest-spatial-audio");
namespace qsa {
struct Settings {
    bool enabled = true;
    float surroundAmount = 0.3f;
    float outputGain = 0.5f;
};
Settings LoadSettings();
}
