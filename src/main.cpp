#include "GameManager.h"
#include "BoardRenderer.h"
#include "UIManager.h"
#include "SoundManager.h"
#include "MoveValidator.h"
#include <iostream>
#include <string>

#ifdef HAS_SFML
#include <SFML/Graphics.hpp>

static const float SQUARE_SIZE  = 80.f;
static const float BOARD_OFFSET = 20.f;
static const float PANEL_WIDTH  = 240.f;
static const unsigned WINDOW_W  = static_cast<unsigned>(BOARD_OFFSET * 2 + SQUARE_SIZE * 8 + PANEL_WIDTH);
static const unsigned WINDOW_H  = static_cast<unsigned>(BOARD_OFFSET * 2 + SQUARE_SIZE * 8);

int runSFML() {
    sf::RenderWindow window(
        sf::VideoMode(WINDOW_W, WINDOW_H),
        "Chess - C++ OOP",
        sf::Style::Titlebar | sf::Style::Close
    );
    window.setFramerateLimit(60);
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event))
            if (event.type == sf::Event::Closed) window.close();
        window.clear(sf::Color(15, 15, 15));
        window.display();
    }
    return 0;
}
#endif
int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
#ifdef HAS_SFML
    return runSFML();
#endif
    return 0;
}
//checking