#include <Loader.hpp>
#include <MainApplication.hpp>
#include <ModSelect.hpp>
#include <json.hpp>
#include <zzip/zzip.h>
#include <string>
#include <Network.hpp>

extern MainApplication::Ref g_MainApplication;

#define GAME_TITLEID std::string("0100000000010000")
#define GAME_BANANA_ID std::string("6376")
#define NAME_IN_SWITCH_FOLDER std::string("SuperModysseyLoader")

bool vectorHas (std::vector<std::string> arr, std::string item) {
    return std::find(arr.begin(), arr.end(), item) != arr.end();
}

bool CreateDirectoryRecursive(const std::string & dirName) {
    std::error_code err;
    if (!std::filesystem::create_directories(dirName, err)) {
        if (std::filesystem::exists(dirName)) {
            return true;    // the folder probably already existed
        }
        g_MainApplication->modSelectLayout = ModSelect::New();
        g_MainApplication->LoadLayout(g_MainApplication->modSelectLayout);
        g_MainApplication->CreateShowDialog("Error", "CreateDirectoryRecursive: FAILED to create [" + dirName + "], err:" + err.message(), { "Ok" }, true);
        return false;
    }
    return true;
}

std::string dirnameOf(const std::string& fname) {
    size_t pos = fname.find_last_of("\\/");
    return (std::string::npos == pos)
        ? ""
        : fname.substr(0, pos);
}


std::vector<std::string> splitString (std::string s, std::string delimiter) {
    std::size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find (delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}

int indexOf (std::vector<std::string> arr, std::string str) {
    for (int i = 0; i < arr.size(); i++) {
        if (arr[i] == str) {
            return i;
        }
    }
    return -1;
}

void loadConfig() {
    std::string data;
    std::ifstream input_file("sdmc:/switch/" + NAME_IN_SWITCH_FOLDER + "/config.json", std::ios::binary | std::ios::in);
    std::ostringstream ss;
    ss << input_file.rdbuf();
    data = ss.str();
    input_file.close();
    g_MainApplication->enabledMods = nlohmann::json::parse(data);
}

void saveConfig() {
    std::ofstream configFile("sdmc:/switch/" + NAME_IN_SWITCH_FOLDER + "/config.json");
    configFile << g_MainApplication->enabledMods.dump(4);
    configFile.close();
}

Loader::Loader() : Layout::Layout() {
    this->infoText = pu::ui::elm::TextBlock::New(150, 320, "Loading...");
    this->infoText->SetHorizontalAlign(pu::ui::elm::HorizontalAlign::Center);
    this->infoText->SetColor(pu::ui::Color::FromHex("#000000"));

    this->loadingBar = pu::ui::elm::ProgressBar::New(340, 360, 600, 30, 100.0f);
    this->loadingBar->SetProgressColor(pu::ui::Color::FromHex("#4C87E6"));

    this->Add(this->infoText);
    this->Add(this->loadingBar);

    this->AddRenderCallback([&]() {
        if (g_MainApplication->first_run) {
            g_MainApplication->first_run = false;
            loadConfig();
            this->LoadPage(1);
        };
    });
}

void Loader::LoadPage(int page) {
    this->infoText->SetText("Fetching mods on page " + std::to_string(page) + "...");
    this->loadingBar->SetProgress(0.0f);
    g_MainApplication->CallForRender();
    g_MainApplication->page = page;
    auto mods = nlohmann::json::parse(net::RetrieveContent("https://api.gamebanana.com/Core/List/New?page=" + std::to_string(page) + "&itemtype=Mod&gameid=" + GAME_BANANA_ID, "application/json"));
    this->infoText->SetText("Found " + std::to_string(mods.size()) + " mods on page " + std::to_string(page) + ". Parsing...");
    this->loadingBar->SetProgress(50.0f);
    g_MainApplication->CallForRender();
    std::string query = "";
    for (auto mod : mods) {
        int gameid = mod[1];
        query += "itemtype[]=Mod&itemid[]=" + std::to_string(gameid) + "&fields[]=name,description,likes,views,downloads,Files().aFiles()&";
    }
    g_MainApplication->mods = nlohmann::json::parse(net::RetrieveContent("https://api.gamebanana.com/Core/Item/Data?" + query, "application/json"));
    g_MainApplication->modSelectLayout = ModSelect::New();
    g_MainApplication->LoadLayout(g_MainApplication->modSelectLayout);
}

void Loader::InstallMod(nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::json>> item) {
    this->infoText->SetText("Finding files...");
    this->loadingBar->SetProgress(0.0f);
    g_MainApplication->CallForRender();

    auto fileList = splitString(net::RetrieveContent("https://gamebanana.com/apiv9/File/" + item.key() + "/RawFileList"), "\n");
    std::vector<std::string> atmosphereFiles = {};
    std::vector<std::string> romfsFiles = {};
    std::vector<std::string> exefsFiles = {};
    for (auto file : fileList) {
        auto pathsInFile = splitString(file, "/");
        if (vectorHas(pathsInFile, "atmosphere")) {
            atmosphereFiles.push_back(file);
        } else if (vectorHas(pathsInFile, "romfs")) {
            romfsFiles.push_back(file);
        } else if (vectorHas(pathsInFile, "exefs")) {
            exefsFiles.push_back(file);
        }
    }

    if (atmosphereFiles.size() + romfsFiles.size() + exefsFiles.size() == 0) {
        g_MainApplication->modSelectLayout = ModSelect::New();
        g_MainApplication->LoadLayout(g_MainApplication->modSelectLayout);
        g_MainApplication->CreateShowDialog("Error", "Could not find an atmosphere, romfs or exefs folder.", { "Ok" }, true);
        return;
    }

    this->infoText->SetText("Downloading zip...");
    this->loadingBar->SetProgress(20.0f);
    g_MainApplication->CallForRender();

    CreateDirectoryRecursive("sdmc:/switch/" + NAME_IN_SWITCH_FOLDER + "/temp/");

    std::string zipPath = "sdmc:/switch/" + NAME_IN_SWITCH_FOLDER + "/temp/" + item.key() + ".zip";
    net::RetrieveToFile(item.value()["_sDownloadUrl"].get<std::string>(), zipPath);

    this->infoText->SetText("Extracting zip...");
    this->loadingBar->SetProgress(40.0f);
    g_MainApplication->CallForRender();

    ZZIP_DIR *dir;
    zzip_error_t error;
    
    dir = zzip_dir_open(zipPath.c_str(), &error);
    if (!dir) {
        g_MainApplication->modSelectLayout = ModSelect::New();
        g_MainApplication->LoadLayout(g_MainApplication->modSelectLayout);
        g_MainApplication->CreateShowDialog("Error", "Could not open zip file.", { "Ok" }, true);
        return;
    }


    if (atmosphereFiles.size() > 0) {
        int atmosphereFileIndex = 0;
        for (auto atmosphereFile : atmosphereFiles) {
            this->infoText->SetText("Extracting file " + atmosphereFile + "...");
            this->loadingBar->SetProgress(40.0f + (atmosphereFileIndex / atmosphereFiles.size()) * 55.0f);
            g_MainApplication->CallForRender();

            CreateDirectoryRecursive(dirnameOf("sdmc:/atmosphere/" + splitString(atmosphereFile, "atmosphere/").back()));

            std::ofstream out;

            out.open("sdmc:/atmosphere/" + splitString(atmosphereFile, "atmosphere/").back(), std::ios::out);
            
            ZZIP_FILE *fp = zzip_file_open(dir, atmosphereFile.c_str(), 0);

            char buf[4096];
            ssize_t num_bytes = 0;

            while ((num_bytes = zzip_file_read(fp, buf, sizeof(buf)/sizeof(buf[0]))) > 0) {
                out.write(buf, num_bytes);
            }
            zzip_file_close(fp);
            atmosphereFileIndex++;
        }
    } else {
        int fsFileIndex = 0;
        int fsFileCount = romfsFiles.size() + exefsFiles.size();

        for (auto romfsFile : romfsFiles) {
            this->infoText->SetText("Extracting file " + romfsFile + "...");
            this->loadingBar->SetProgress(40.0f + (fsFileIndex / fsFileCount) * 55.0f);
            g_MainApplication->CallForRender();

            CreateDirectoryRecursive(dirnameOf("sdmc:/atmosphere/contents/" + GAME_TITLEID + "/romfs/" + splitString(romfsFile, "romfs/").back()));

            std::ofstream out;

            out.open("sdmc:/atmosphere/contents/" + GAME_TITLEID + "/romfs/" + splitString(romfsFile, "romfs/").back(), std::ios::out);
            
            ZZIP_FILE *fp = zzip_file_open(dir, romfsFile.c_str(), 0);

            char buf[4096];
            ssize_t num_bytes = 0;

            while ((num_bytes = zzip_file_read(fp, buf, sizeof(buf)/sizeof(buf[0]))) > 0) {
                out.write(buf, num_bytes);
            }
            zzip_file_close(fp);

            fsFileIndex++;
        }
        for (auto exefsFile : exefsFiles) {
            this->infoText->SetText("Extracting file " + exefsFile + "...");
            this->loadingBar->SetProgress(40.0f + (fsFileIndex / fsFileCount) * 55.0f);
            g_MainApplication->CallForRender();

            CreateDirectoryRecursive(dirnameOf("sdmc:/atmosphere/contents/" + GAME_TITLEID + "/exefs/" + splitString(exefsFile, "exefs/").back()));

            std::ofstream out;

            out.open("sdmc:/atmosphere/contents/" + GAME_TITLEID + "/exefs/" + splitString(exefsFile, "exefs/").back(), std::ios::out);
            
            ZZIP_FILE *fp = zzip_file_open(dir, exefsFile.c_str(), 0);

            char buf[4096];
            ssize_t num_bytes = 0;

            while ((num_bytes = zzip_file_read(fp, buf, sizeof(buf)/sizeof(buf[0]))) > 0) {
                out.write(buf, num_bytes);
            }
            zzip_file_close(fp);

            fsFileIndex++;
        }
    }
    
    zzip_dir_close(dir);

    this->infoText->SetText("Saving config...");
    this->loadingBar->SetProgress(95.0f);
    g_MainApplication->CallForRender();

    g_MainApplication->enabledMods.push_back(item.key());
    saveConfig();

    g_MainApplication->modSelectLayout = ModSelect::New();
    g_MainApplication->LoadLayout(g_MainApplication->modSelectLayout);

    g_MainApplication->CreateShowDialog("Success", "Installed", { "Ok" }, true);

}

void Loader:: UninstallMod(nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::json>> item) {
    this->infoText->SetText("Finding files...");
    this->loadingBar->SetProgress(0.0f);
    g_MainApplication->CallForRender();

    auto fileList = splitString(net::RetrieveContent("https://gamebanana.com/apiv9/File/" + item.key() + "/RawFileList"), "\n");
    std::vector<std::string> atmosphereFiles = {};
    std::vector<std::string> romfsFiles = {};
    std::vector<std::string> exefsFiles = {};
    for (auto file : fileList) {
        auto pathsInFile = splitString(file, "/");
        if (vectorHas(pathsInFile, "atmosphere")) {
            atmosphereFiles.push_back(file);
        } else if (vectorHas(pathsInFile, "romfs")) {
            romfsFiles.push_back(file);
        } else if (vectorHas(pathsInFile, "exefs")) {
            exefsFiles.push_back(file);
        }
    }

    this->infoText->SetText("Deleting files...");
    this->loadingBar->SetProgress(40.0f);
    g_MainApplication->CallForRender();

    if (atmosphereFiles.size() > 0) {
        int atmosphereFileIndex = 0;
        for (auto atmosphereFile : atmosphereFiles) {
            this->infoText->SetText("Deleting file " + atmosphereFile + "...");
            this->loadingBar->SetProgress(40.0f + (atmosphereFileIndex / atmosphereFiles.size()) * 55.0f);
            g_MainApplication->CallForRender();
            std::string atmosphereFilePath = "sdmc:/atmosphere/" + splitString(atmosphereFile, "atmosphere/").back();
            if (std::filesystem::exists(atmosphereFilePath.c_str())) {
                std::filesystem::remove(atmosphereFilePath.c_str());
            };
            atmosphereFileIndex++;
        }
    } else {
        int fsFileIndex = 0;
        int fsFileCount = romfsFiles.size() + exefsFiles.size();
        for (auto romfsFile : romfsFiles) {
            this->infoText->SetText("Deleting file " + romfsFile + "...");
            this->loadingBar->SetProgress(40.0f + (fsFileIndex / fsFileCount) * 55.0f);
            g_MainApplication->CallForRender();
            std::string romfsFilePath = "sdmc:/atmosphere/contents/" + GAME_TITLEID + "/romfs/" + splitString(romfsFile, "romfs/").back();
            if (std::filesystem::exists(romfsFilePath.c_str())) {
                std::filesystem::remove(romfsFilePath.c_str());
            };
            fsFileIndex++;
        }
        for (auto exefsFile : exefsFiles) {
            this->infoText->SetText("Deleting file " + exefsFile + "...");
            this->loadingBar->SetProgress(40.0f + (fsFileIndex / fsFileCount) * 55.0f);
            g_MainApplication->CallForRender();
            std::string exefsFilePath = "sdmc:/atmosphere/contents/" + GAME_TITLEID + "/exefs/" + splitString(exefsFile, "exefs/").back();
            if (std::filesystem::exists(exefsFilePath.c_str())) {
                std::filesystem::remove(exefsFilePath.c_str());
            };
            fsFileIndex++;
        }
    }

    this->infoText->SetText("Saving config...");
    this->loadingBar->SetProgress(95.0f);
    g_MainApplication->CallForRender();

    g_MainApplication->enabledMods.erase(indexOf(g_MainApplication->enabledMods, item.key()));
    saveConfig();

    g_MainApplication->modSelectLayout = ModSelect::New();
    g_MainApplication->LoadLayout(g_MainApplication->modSelectLayout);

    g_MainApplication->CreateShowDialog("Success", "Uninstalled", { "Ok" }, true);

}