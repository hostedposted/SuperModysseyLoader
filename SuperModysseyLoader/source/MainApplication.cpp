#include <MainApplication.hpp>

extern MainApplication::Ref g_MainApplication;

void MainApplication::OnLoad() {
    this->first_run = true;
    this->loaderLayout = Loader::New();
    this->LoadLayout(this->loaderLayout);

    this->SetOnInput([&](const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if (keys_down & HidNpadButton_Plus) {
            this->Close();
        }
    });
}