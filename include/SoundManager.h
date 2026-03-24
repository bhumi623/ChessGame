#pragma once
#include "Types.h"
#include <string>
#include <map>

#ifdef HAS_SFML
#include <SFML/Audio.hpp>
#endif

namespace Chess {

// ─── SoundManager ────────────────────────────────────────────────────────────
// Loads and plays chess sound effects.

class SoundManager {
public:
    SoundManager();

    bool loadSounds(const std::string& assetPath);

    void play(GameEvent event);
    void setVolume(float v); // 0-100

private:
#ifdef HAS_SFML
    std::map<GameEvent, sf::SoundBuffer> m_buffers;
    std::map<GameEvent, sf::Sound>       m_sounds;
#endif
    float m_volume = 80.f;
};

} // namespace Chess
