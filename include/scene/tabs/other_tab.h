#pragma once
#include "preferences.h"
#include "logging.h"
#include "toggle_row.h"
#include "multi_row.h"
#include "text_input.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

class OtherTab {
public:
    OtherTab(const sf::Font& font,
             std::shared_ptr<Preferences> prefs,
             std::shared_ptr<Logger> logger);

    void loadFromPrefs();
    void handleEvent(const sf::Event& ev);
    void update();

    float render(sf::RenderTarget& target,
                 float contentX, float ctrlX,
                 float startY);

    void refreshLabels();
    void refreshSelection();
    void registerFocus(std::vector<Button*>& out);
    bool anyEditing() const;

private:
    const sf::Font& font_;
    std::shared_ptr<Preferences> prefs_;
    std::shared_ptr<Logger>      logger_;

    // 状态
    bool rememberSize_      = true;
    bool autoPauseOnBlur_   = true;
    int  logRotateIndex_    = 0;
    int  logKeepIndex_      = 1;

    std::vector<std::unique_ptr<ToggleRow>> toggles_;
    std::vector<std::unique_ptr<MultiRow>>  multiRows_;
    // [0] LogRotate  [1] LogKeep

    std::unique_ptr<TextInput> playerNameInput_;
    sf::Text labelPlayerName_;

    void applyAutoPause();
    void applyLogRotation();
};