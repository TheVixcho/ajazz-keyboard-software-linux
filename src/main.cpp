#include <csignal>
#include <iostream>
#include <string>

#include "GUI/app.hpp"
#include "constants.hpp"

void signalHandler(int signum) {
    GetKeyboard().reset();
    GetConfig().reset();
    exit(0);
}

int main(int argc, char *argv[]) {
    signal(SIGABRT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGSEGV, signalHandler);

    try {
        AjazzGUI gui("Ajazz Keyboard software");

        GetConfig() = std::make_shared<Config>();
        GetKeyboard() = std::make_shared<AK820Pro>(AJAZZ_VID, 0);

        gui.run();
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}