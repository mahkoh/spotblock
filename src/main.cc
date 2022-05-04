#include <string>
#include <iostream>

#include "daemon.h"

static void run() {
    auto daemon = new sbdaemon::Daemon;
    daemon->init();
    daemon->loop();
}

void emit_help() {
    const char *help = R"(Usage: spotblock [OPTION]
Block spotify ads.

      --help  display this help and exit

For a detailed description see https://github.com/mahkoh/spotblock.
)";
    std::cout << help;
}

int main(int argc, char **argv) {
    if (argc != 1) {
        emit_help();
        return 0;
    }

    try {
        run();
    } catch (std::string s) {
        std::cerr << "an unrecoverable error occured: " << s << "\n";
        return 1;
    }

    return 0;
}

// vim: et:sw=4:tw=90:ts=4:sts=4:cc=+1
