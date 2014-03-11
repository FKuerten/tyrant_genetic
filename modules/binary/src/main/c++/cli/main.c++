#include <iostream>
#include "commands.h++"
#include "optionParser.h++"
#include <errorHandling/exceptions.h++>

#ifdef _POSIX_SOURCE
    #include <csignal>
#endif

#ifdef _POSIX_SOURCE
    TyrantGenetic::CLI::Command::Ptr theCommand;

    extern "C" {
        void abortHandler(int sig) {
            // this is evil style, but well...
            theCommand->abort();
        }
    }
#endif

/**
 * Reads cli arguments, parses them, then mutates
 */
int main(int const argc, char * const * const argv)
{
    try {
        TyrantGenetic::CLI::Command::Ptr command = TyrantGenetic::CLI::parseArguments(argc, argv);

        #ifdef _POSIX_SOURCE
            ::theCommand = command;
            struct sigaction action, oldIntAction, oldTermAction;
            action.sa_handler = &abortHandler;
            //action.sa_sigaction = &abortHandler;
            sigemptyset(&action.sa_mask);
            action.sa_flags = 0;
            sigaction(SIGINT, &action, &oldIntAction);
            sigaction(SIGTERM, &action, &oldTermAction);
            //std::clog << "Signal handlers installed." << std::endl;
        #endif

        srand(static_cast<unsigned int>(time(nullptr)));
        command->execute();

        #ifdef _POSIX_SOURCE
            sigaction(SIGINT, &oldIntAction, nullptr);
            sigaction(SIGTERM, &oldTermAction, nullptr);
            //std::clog << "Signal handlers removed." << std::endl;
            ::theCommand = nullptr;
        #endif

    } catch(Exception const & e) {
        std::cerr << "Exception:" << std::endl;
        std::cerr << e.what() << std::endl;
        e.printStacktrace(std::cerr);
        return -1;
    }
}
