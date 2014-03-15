#ifndef TYRANT_CACHE_TO_TYRANT_OPTIMIZER_BINDING_HPP
    #define TYRANT_CACHE_TO_TYRANT_OPTIMIZER_BINDING_HPP

    #include <core/simulatorCore.h++>
    #include <pstreams/pstream.h>

    namespace TyrantCache {
        namespace TO {

            class TyrantOptimizerCLI : public Tyrant::Core::SimulatorCore {
                private:
                    redi::ipstream * theProgram;

                public:
                    virtual Tyrant::Core::SimulationResult simulate(Tyrant::Core::SimulationTask const & task);
                    virtual void abort();
                    virtual std::string getCoreName() const;
                    virtual std::string getCoreVersion() const;
                    virtual std::string getCoreVersionHumanReadable() const;
            };
        }
    }
#endif
