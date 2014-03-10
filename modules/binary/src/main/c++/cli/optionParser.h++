#ifndef TYRANT_MUTATOR_CLI_OPTIONPARSER_HPP
    #define TYRANT_MUTATOR_CLI_OPTIONPARSER_HPP

    #include "commands.h++"

    namespace TyrantGenetic {
        namespace CLI {

            Command::Ptr parseArguments(int argc, char const * const * argv);

        }
    }

#endif
