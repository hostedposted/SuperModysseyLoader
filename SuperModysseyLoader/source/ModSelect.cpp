#include <ModSelect.hpp>
#include <MainApplication.hpp>
#include <algorithm>

extern MainApplication::Ref g_MainApplication;

bool vectorContains (std::vector<std::string> arr, std::string item) {
    return std::find(arr.begin(), arr.end(), item) != arr.end();
}

ModSelect::ModSelect() : Layout::Layout() {
    this->modsMenu = pu::ui::elm::Menu::New(0, 0, pu::ui::render::ScreenWidth, pu::ui::Color::FromHex("#008080"), pu::ui::Color::FromHex("#005380"), 80, 640 / 80);


    for (auto mod : g_MainApplication->mods) {
        std::string modName = mod[0];
        auto menuItem = pu::ui::elm::MenuItem::New(modName);

        menuItem->SetColor(pu::ui::Color::FromHex("#FFFFFF"));
        menuItem->AddOnKey([&]() {
            auto mod = g_MainApplication->mods[this->modsMenu->GetSelectedIndex()];
            std::string modName = mod[0];
            std::string modDescription = mod[1];
            int modLikes = mod[2];
            int modViews = mod[3];
            int modDownloads = mod[4];
            nlohmann::json fileInfo = mod[5];
            auto fileInfoItems = fileInfo.items();
            std::vector<std::string> options = {};
            for (auto it = fileInfoItems.begin(); it != fileInfoItems.end(); ++it) {
                auto fileName = it.value()["_sFile"].get<std::string>();
                if (vectorContains(g_MainApplication->enabledMods, it.key())) {
                    options.push_back("Uninstall " + fileName);
                } else {
                    options.push_back("Install " + fileName);
                }
            }
            options.push_back("Cancel");
            int installSelected = g_MainApplication->CreateShowDialog("Would you like to install " + modName + "?", "Description: " + modDescription + "\nLikes: " + std::to_string(modLikes) + "\nViews: " + std::to_string(modViews) + "\nDownloads: " + std::to_string(modDownloads), options, true);
            if (installSelected < 0) {
                return;
            }
            for (auto it = fileInfoItems.begin(); it != fileInfoItems.end(); ++it) {
                if (options[installSelected].find(it.value()["_sFile"].get<std::string>()) != std::string::npos) {
                    if (options[installSelected][0] == 'I') {
                        g_MainApplication->loaderLayout = Loader::New();
                        g_MainApplication->LoadLayout(g_MainApplication->loaderLayout);

                        g_MainApplication->loaderLayout->InstallMod(it);
                    } else {
                        g_MainApplication->loaderLayout = Loader::New();
                        g_MainApplication->LoadLayout(g_MainApplication->loaderLayout);

                        g_MainApplication->loaderLayout->UninstallMod(it);
                    }
                }
            }
        });

        this->modsMenu->AddItem(menuItem);
    }

    this->prevButton = pu::ui::elm::Button::New(0, 640, pu::ui::render::ScreenWidth / 2, 80, "Previous Page", pu::ui::Color::FromHex("#FFFFFF"), pu::ui::Color::FromHex("#000000"));
    this->nextButton = pu::ui::elm::Button::New(pu::ui::render::ScreenWidth / 2, 640, pu::ui::render::ScreenWidth / 2, 80, "Next Page", pu::ui::Color::FromHex("#000000"), pu::ui::Color::FromHex("#FFFFFF"));

    this->prevButton->SetOnClick([&]() {
        if (g_MainApplication->page > 1) {
            g_MainApplication->loaderLayout = Loader::New();
            g_MainApplication->LoadLayout(g_MainApplication->loaderLayout);
            g_MainApplication->loaderLayout->LoadPage(g_MainApplication->page - 1);
        } else {
            g_MainApplication->CreateShowDialog("Error", "You are on the first page", { "Ok" }, true);
        }
    });

    this->nextButton->SetOnClick([&]() {
        g_MainApplication->loaderLayout = Loader::New();
        g_MainApplication->LoadLayout(g_MainApplication->loaderLayout);
        g_MainApplication->loaderLayout->LoadPage(g_MainApplication->page + 1);
    });

    this->Add(this->modsMenu);
    this->Add(this->prevButton);
    this->Add(this->nextButton);
}
