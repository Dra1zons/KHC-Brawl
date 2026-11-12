#include "modcore.hpp"
#include <iostream>
#include <stdexcept>
using namespace modcore;
int main(int argc,char** argv) {
    if(argc!=3){std::cerr<<"Usage: modcore_cli init|validate|plan SETTINGS_FILE\n";return 2;}
    try {
        std::string command=argv[1];
        if(command=="init") {
            if(std::filesystem::exists(argv[2]))throw std::runtime_error("Refusing to overwrite existing settings");
            SettingsStore::save(argv[2],Settings{});
            std::cout<<"Created local settings. No game changes.\n";return 0;
        }
        if(command!="validate" && command!="plan")throw std::invalid_argument("Unknown command");
        if(!std::filesystem::is_regular_file(argv[2]))throw std::runtime_error("Settings file does not exist");
        auto settings=SettingsStore::load(argv[2]);
        if(command=="validate"){std::cout<<"Settings schema and values are valid. Resource availability is NOT checked.\n";return 0;}
        // The CLI uses user resource IDs only for planning; this is not a game resource check.
        Runtime runtime({{settings.background,settings.music}},1);runtime.configure(settings);
        std::cout<<"NO GAME ADAPTER ATTACHED. This is not a modded game.\n";
        const char* names[]={"appearance","audio","frame_limit","camera","quality","particles"};
        auto results=runtime.applySettings();
        for(std::size_t i=0;i<results.size();++i)std::cout<<names[i]<<": "<<statusName(results[i].status)<<'\n';
        return 0;
    } catch(const std::exception& e){std::cerr<<"Error: "<<e.what()<<'\n';return 1;}
}
