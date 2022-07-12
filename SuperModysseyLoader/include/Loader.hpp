#pragma once

#include <pu/Plutonium>
#include <json.hpp>

class Loader : public pu::ui::Layout {
    private:
        pu::ui::elm::TextBlock::Ref infoText;
        pu::ui::elm::ProgressBar::Ref loadingBar;
    public:
        Loader();

        PU_SMART_CTOR(Loader)

        void LoadPage(int page);
        void InstallMod(nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::json>> item);
        void UninstallMod(nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::json>> item);
};
