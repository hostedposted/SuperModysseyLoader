#include <MainApplication.hpp>
#include <string>
#include <Network.hpp>
#include <json.hpp>
#include <fstream>
#include <filesystem>
#include <zzip/zzip.h>

extern MainApplication::Ref g_MainApplication;

#define GAME_TITLEID std::string("0100000000010000")
#define GAME_BANANA_ID std::string("6376")
#define NAME_IN_SWITCH_FOLDER std::string("SuperModysseyLoader")

bool CreateDirectoryRecursive(const std::string & dirName) {
    std::error_code err;
    if (!std::filesystem::create_directories(dirName, err))
    {
        if (std::filesystem::exists(dirName))
        {
            return true;    // the folder probably already existed
        }
        g_MainApplication->CreateShowDialog("Error", "CreateDirectoryRecursive: FAILED to create [" + dirName + "], err:" + err.message(), { "Ok" }, true);
        return false;
    }
    return true;
}

std::string dirnameOf(const std::string& fname)
{
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


void loadMods() {
    int page = g_MainApplication->page;
    auto mods = nlohmann::json::parse(net::RetrieveContent("https://api.gamebanana.com/Core/List/New?page=" + std::to_string(page) + "&itemtype=Mod&gameid=" + GAME_BANANA_ID, "application/json"));
    std::string query = "";
    for (auto mod : mods) {
        int gameid = mod[1];
        query += "itemtype[]=Mod&itemid[]=" + std::to_string(gameid) + "&fields[]=name,description,likes,views,downloads,Files().aFiles()&";
    }
    g_MainApplication->mods = nlohmann::json::parse(net::RetrieveContent("https://api.gamebanana.com/Core/Item/Data?" + query, "application/json"));
}


// Implement all the layout/application functions here

CustomLayout::CustomLayout() : Layout::Layout() {
    // Create the TextBlock instance with the text we want
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
                if (indexOf(g_MainApplication->enabledMods, it.key()) == -1) {
                    options.push_back("Install " + fileName);
                } else {
                    options.push_back("Uninstall " + fileName);
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
                        auto fileList = splitString(net::RetrieveContent("https://gamebanana.com/apiv9/File/" + it.key() + "/RawFileList"), "\n");
                        std::vector<std::string> atmosphereFiles = {};
                        std::vector<std::string> romfsFiles = {};
                        std::vector<std::string> exefsFiles = {};
                        for (auto file : fileList) {
                            auto pathsInFile = splitString(file, "/");
                            if (indexOf(pathsInFile, "atmosphere") >= 0) {
                                atmosphereFiles.push_back(file);
                            } else if (indexOf(pathsInFile, "romfs") >= 0) {
                                romfsFiles.push_back(file);
                            } else if (indexOf(pathsInFile, "exefs") >= 0) {
                                exefsFiles.push_back(file);
                            }
                        }
                        if (atmosphereFiles.size() + romfsFiles.size() + exefsFiles.size() == 0) {
                            g_MainApplication->CreateShowDialog("Error", "Could not find an atmosphere, romfs or exefs folder.", { "Ok" }, true);
                        } else {
                            CreateDirectoryRecursive("sdmc:/switch/" + NAME_IN_SWITCH_FOLDER + "/temp/");
                            std::string zipPath = "sdmc:/switch/" + NAME_IN_SWITCH_FOLDER + "/temp/" + it.key() + ".zip";
                            net::RetrieveToFile(it.value()["_sDownloadUrl"].get<std::string>(), zipPath);

                            ZZIP_DIR *dir;
                            zzip_error_t error;
                            
                            dir = zzip_dir_open(zipPath.c_str(), &error);
                            if (!dir) {
                                return;
                            }


                            if (atmosphereFiles.size() > 0) {
                                for (auto atmosphereFile : atmosphereFiles) {
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
                                }
                            } else {
                                for (auto romfsFile : romfsFiles) {
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
                                }
                                for (auto exefsFile : exefsFiles) {
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
                                }
                            }
                            
                            zzip_dir_close(dir);

                            g_MainApplication->enabledMods.push_back(it.key());
                            saveConfig();

                            g_MainApplication->CreateShowDialog("Success", "Installed", { "Ok" }, true);
                        };
                    } else {
                        auto fileList = splitString(net::RetrieveContent("https://gamebanana.com/apiv9/File/" + it.key() + "/RawFileList"), "\n");
                        std::vector<std::string> atmosphereFiles = {};
                        std::vector<std::string> romfsFiles = {};
                        std::vector<std::string> exefsFiles = {};
                        for (auto file : fileList) {
                            auto pathsInFile = splitString(file, "/");
                            if (indexOf(pathsInFile, "atmosphere") >= 0) {
                                atmosphereFiles.push_back(file);
                            } else if (indexOf(pathsInFile, "romfs") >= 0) {
                                romfsFiles.push_back(file);
                            } else if (indexOf(pathsInFile, "exefs") >= 0) {
                                exefsFiles.push_back(file);
                            }
                        }

                        if (atmosphereFiles.size() > 0) {
                            for (auto atmosphereFile : atmosphereFiles) {
                                std::string atmosphereFilePath = "sdmc:/atmosphere/" + splitString(atmosphereFile, "atmosphere/").back();
                                std::filesystem::remove(atmosphereFilePath.c_str());
                            }
                        } else {
                            for (auto romfsFile : romfsFiles) {
                                std::string romfsFilePath = "sdmc:/atmosphere/contents/" + GAME_TITLEID + "/romfs/" + splitString(romfsFile, "romfs/").back();
                                std::filesystem::remove(romfsFilePath.c_str());
                            }
                            for (auto exefsFile : exefsFiles) {
                                std::string exefsFilePath = "sdmc:/atmosphere/contents/" + GAME_TITLEID + "/exefs/" + splitString(exefsFile, "exefs/").back();
                                std::filesystem::remove(exefsFilePath.c_str());
                            }
                        }

                        g_MainApplication->enabledMods.erase(indexOf(g_MainApplication->enabledMods, it.key()));
                        saveConfig();

                        g_MainApplication->CreateShowDialog("Success", "Uninstalled", { "Ok" }, true);
                    }
                }
            }
        });

        this->modsMenu->AddItem(menuItem);
    }

    auto prevButton = pu::ui::elm::Button::New(0, 640, pu::ui::render::ScreenWidth / 2, 80, "Previous Page", pu::ui::Color::FromHex("#FFFFFF"), pu::ui::Color::FromHex("#000000"));
    auto nextButton = pu::ui::elm::Button::New(pu::ui::render::ScreenWidth / 2, 640, pu::ui::render::ScreenWidth / 2, 80, "Next Page", pu::ui::Color::FromHex("#000000"), pu::ui::Color::FromHex("#FFFFFF"));

    prevButton->SetOnClick([&]() {
        if (g_MainApplication->page > 1) {
            g_MainApplication->page -= 1;
            loadMods();
            g_MainApplication->layout = CustomLayout::New();

            g_MainApplication->LoadLayout(g_MainApplication->layout);
        } else {
            g_MainApplication->CreateShowDialog("Error", "You are on the first page", { "Ok" }, true);
        }
    });

    nextButton->SetOnClick([&]() {
        g_MainApplication->page += 1;
        loadMods();
        g_MainApplication->layout = CustomLayout::New();

        g_MainApplication->LoadLayout(g_MainApplication->layout);
    });

    // Add the instance to the layout. IMPORTANT! this MUST be done for them to be used, having them as members is not enough (just a simple way to keep them)
    this->Add(this->modsMenu);
    this->Add(prevButton);
    this->Add(nextButton);
}

void MainApplication::OnLoad() {
    // Create the layout (calling the smart constructor above)
    g_MainApplication->page = 1;

    loadConfig();
    loadMods();
    this->layout = CustomLayout::New();

    // Load the layout. In applications layouts are loaded, not added into a container (you don't select an added layout, just load it from this function)
    // Simply explained: loading layout = the application will render that layout in the very next frame
    this->LoadLayout(this->layout);

    // Set a function when input is caught. This input handling will be the first one to be handled (before Layout or any Elements)
    // Using a lambda function here to simplify things
    // You can use member functions via std::bind() C++ wrapper
    this->SetOnInput([&](const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        // If + is pressed, exit application
        if (keys_down & HidNpadButton_Plus) {
            this->Close();
        }
    });
}