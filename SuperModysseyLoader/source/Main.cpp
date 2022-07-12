#include <MainApplication.hpp>
#include <fstream>

MainApplication::Ref g_MainApplication;

/*
// If you would like to initialize and finalize stuff before or after Plutonium, you can use libnx's userAppInit/userAppExit

extern "C" void userAppInit() {
    // Initialize stuff
}

extern "C" void userAppExit() {
    // Cleanup/finalize stuff
}
*/

// Main entrypoint
int main() {
    auto renderer_opts = pu::ui::render::RendererInitOptions(SDL_INIT_EVERYTHING, pu::ui::render::RendererHardwareFlags);
    renderer_opts.UseImage(pu::ui::render::IMGAllFlags);
    renderer_opts.UseAudio(pu::ui::render::MixerAllFlags);
    renderer_opts.UseTTF();
    auto renderer = pu::ui::render::Renderer::New(renderer_opts);

    g_MainApplication = MainApplication::New(renderer);

    g_MainApplication->Prepare();


    g_MainApplication->ShowWithFadeIn();

    g_MainApplication->CloseWithFadeOut();

    return 0;
}