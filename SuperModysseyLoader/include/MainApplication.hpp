#pragma once

#include <ModSelect.hpp>
#include <Loader.hpp>
#include <pu/Plutonium>
#include <string>
#include <json.hpp>

class MainApplication : public pu::ui::Application {
    public:
        using Application::Application;
        PU_SMART_CTOR(MainApplication)

        int page;
        int renderCountLoader;
        nlohmann::json mods;
        nlohmann::json enabledMods;
        ModSelect::Ref modSelectLayout;
        Loader::Ref loaderLayout;

        void OnLoad() override;
};