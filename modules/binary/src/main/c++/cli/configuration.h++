#ifndef TYRANT_MUTATOR_CLI_CONFIGURATION_HPP
    #define TYRANT_MUTATOR_CLI_CONFIGURATION_HPP

    #include <core/simulatorCore.h++>
    #include <mutator/mutator.h++>
    #include <logging/logger.h++>

    namespace TyrantGenetic {
        namespace CLI {

            class Configuration {
                public:
                    signed int verbosity = 0;

                    Praetorian::Basics::Logging::Logger::Ptr constructRootLogger() const;
                    Tyrant::Core::SimulatorCore::Ptr constructCore() const;
                    Tyrant::Mutator::Mutator::Ptr constructMutator() const;
            };
        }
    }
#endif
