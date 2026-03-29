#include "SoundManager.h"
#include <iostream>

namespace Chess {

SoundManager::SoundManager() {}

bool SoundManager::loadSounds([[maybe_unused]] const std::string& assetPath) {
#ifdef HAS_SFML
    // Map events to sound file names
    std::vector<std::pair<GameEvent, std::string>> soundFiles = {
        { GameEvent::BoardChanged,      "move.wav"     },
        { GameEvent::PieceCapture,      "capture.wav"  },
        { GameEvent::CheckDetected,     "check.wav"    },
        { GameEvent::CheckmateDetected, "checkmate.wav"},
        { GameEvent::Castled,           "castle.wav"   },
        { GameEvent::PawnPromoted,      "promote.wav"  },
        { GameEvent::DrawDetected,      "draw.wav"     },
    };

    bool allLoaded = true;
    for (auto& [event, file] : soundFiles) {
        sf::SoundBuffer buf;
        if (buf.loadFromFile(assetPath + "/" + file)) {
            m_buffers[event] = std::move(buf);
            m_sounds[event].setBuffer(m_buffers[event]);
            m_sounds[event].setVolume(m_volume);
        } else {
            std::cerr << "Note: Could not load sound: " << file << " (optional)\n";
            allLoaded = false;
        }
    }
    return allLoaded;
#else
    return false;
#endif
}

void SoundManager::play([[maybe_unused]] GameEvent event) {
#ifdef HAS_SFML
    auto it = m_sounds.find(event);
    if (it != m_sounds.end())
        it->second.play();
#endif
}

void SoundManager::setVolume(float v) {
    m_volume = v;
#ifdef HAS_SFML
    for (auto& [evt, sound] : m_sounds)
        sound.setVolume(v);
#endif
}

} // namespace Chess
