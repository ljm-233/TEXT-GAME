#include "button_style.h"

namespace {
ButtonStyle g_style;
}

const ButtonStyle& getButtonStyle() { return g_style; }
void setButtonStyle(const ButtonStyle& s) { g_style = s; }