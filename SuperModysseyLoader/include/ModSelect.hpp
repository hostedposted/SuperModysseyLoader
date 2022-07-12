#pragma once

#include <pu/Plutonium>

class ModSelect : public pu::ui::Layout {
    private:
        pu::ui::elm::Menu::Ref modsMenu;
        pu::ui::elm::Button::Ref prevButton;
        pu::ui::elm::Button::Ref nextButton;
    public:
        ModSelect();

        PU_SMART_CTOR(ModSelect)
};
