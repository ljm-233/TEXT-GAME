#include "scene_id.h"

const char* sceneIdName(SceneId id) {
    switch (id) {
    case SceneId::None:         return "None";
    case SceneId::Back:         return "Back";
    case SceneId::Exit:         return "Exit";
    case SceneId::MainMenu:     return "MainMenu";
    case SceneId::SaveSelect:   return "SaveSelect";
    case SceneId::Game:         return "Game";
    case SceneId::Settings:     return "Settings";
    case SceneId::Console:      return "Console";
    case SceneId::LevelSelect:  return "LevelSelect";
    case SceneId::Editor:       return "Editor";
    case SceneId::Achievements: return "Achievements";
    }
    return "Unknown";
}
