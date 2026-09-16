#include "keybindings.h"

namespace {
constexpr sf::Keyboard::Key kDefaults[KeyBindings::Count] = {
    sf::Keyboard::Key::A,
    sf::Keyboard::Key::D,
    sf::Keyboard::Key::Space,
    sf::Keyboard::Key::Escape,
    sf::Keyboard::Key::R,
};
}

KeyBindings& KeyBindings::instance() {
    static KeyBindings inst;
    return inst;
}

KeyBindings::KeyBindings() {
    resetToDefaults();
}

void KeyBindings::resetToDefaults() {
    for (int i = 0; i < Count; ++i)
        keys_[i] = kDefaults[i];
}

sf::Keyboard::Key KeyBindings::get(Action a) const {
    if (a < 0 || a >= Count) return sf::Keyboard::Key::Unknown;
    return keys_[a];
}

void KeyBindings::set(Action a, sf::Keyboard::Key k) {
    if (a < 0 || a >= Count) return;
    keys_[a] = k;
}

const char* KeyBindings::actionName(Action a) {
    switch (a) {
        case MoveLeft:  return "左移";
        case MoveRight: return "右移";
        case Jump:      return "跳跃";
        case Pause:     return "暂停";
        case Restart:   return "重开";
        default:        return "?";
    }
}

const char* KeyBindings::keyToString(sf::Keyboard::Key k) {
    switch (k) {
        case sf::Keyboard::Key::A: return "A";
        case sf::Keyboard::Key::B: return "B";
        case sf::Keyboard::Key::C: return "C";
        case sf::Keyboard::Key::D: return "D";
        case sf::Keyboard::Key::E: return "E";
        case sf::Keyboard::Key::F: return "F";
        case sf::Keyboard::Key::G: return "G";
        case sf::Keyboard::Key::H: return "H";
        case sf::Keyboard::Key::I: return "I";
        case sf::Keyboard::Key::J: return "J";
        case sf::Keyboard::Key::K: return "K";
        case sf::Keyboard::Key::L: return "L";
        case sf::Keyboard::Key::M: return "M";
        case sf::Keyboard::Key::N: return "N";
        case sf::Keyboard::Key::O: return "O";
        case sf::Keyboard::Key::P: return "P";
        case sf::Keyboard::Key::Q: return "Q";
        case sf::Keyboard::Key::R: return "R";
        case sf::Keyboard::Key::S: return "S";
        case sf::Keyboard::Key::T: return "T";
        case sf::Keyboard::Key::U: return "U";
        case sf::Keyboard::Key::V: return "V";
        case sf::Keyboard::Key::W: return "W";
        case sf::Keyboard::Key::X: return "X";
        case sf::Keyboard::Key::Y: return "Y";
        case sf::Keyboard::Key::Z: return "Z";

        case sf::Keyboard::Key::Num0: return "0";
        case sf::Keyboard::Key::Num1: return "1";
        case sf::Keyboard::Key::Num2: return "2";
        case sf::Keyboard::Key::Num3: return "3";
        case sf::Keyboard::Key::Num4: return "4";
        case sf::Keyboard::Key::Num5: return "5";
        case sf::Keyboard::Key::Num6: return "6";
        case sf::Keyboard::Key::Num7: return "7";
        case sf::Keyboard::Key::Num8: return "8";
        case sf::Keyboard::Key::Num9: return "9";

        case sf::Keyboard::Key::Space:  return "Space";
        case sf::Keyboard::Key::Enter:  return "Enter";
        case sf::Keyboard::Key::Escape: return "Esc";
        case sf::Keyboard::Key::Tab:    return "Tab";
        case sf::Keyboard::Key::Backspace: return "Backspace";

        case sf::Keyboard::Key::Left:  return "←";
        case sf::Keyboard::Key::Right: return "→";
        case sf::Keyboard::Key::Up:    return "↑";
        case sf::Keyboard::Key::Down:  return "↓";

        case sf::Keyboard::Key::LShift: return "LShift";
        case sf::Keyboard::Key::RShift: return "RShift";
        case sf::Keyboard::Key::LControl: return "LCtrl";
        case sf::Keyboard::Key::RControl: return "RCtrl";
        case sf::Keyboard::Key::LAlt: return "LAlt";
        case sf::Keyboard::Key::RAlt: return "RAlt";

        case sf::Keyboard::Key::F1:  return "F1";
        case sf::Keyboard::Key::F2:  return "F2";
        case sf::Keyboard::Key::F3:  return "F3";
        case sf::Keyboard::Key::F4:  return "F4";
        case sf::Keyboard::Key::F5:  return "F5";
        case sf::Keyboard::Key::F6:  return "F6";
        case sf::Keyboard::Key::F7:  return "F7";
        case sf::Keyboard::Key::F8:  return "F8";
        case sf::Keyboard::Key::F9:  return "F9";
        case sf::Keyboard::Key::F10: return "F10";
        case sf::Keyboard::Key::F11: return "F11";
        case sf::Keyboard::Key::F12: return "F12";

        case sf::Keyboard::Key::Unknown: return "?";
        default: return "?";
    }
}