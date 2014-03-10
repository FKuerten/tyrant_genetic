#ifndef TYRANT_GENETIC_GENETIC_HPP
    #define TYRANT_GENETIC_GENETIC_HPP

    #include <memory>
    #include <core/simulatorCore.h++>
    #include <mutator/mutator.h++>
    #include <core/staticDeckTemplate.h++>
    #include "geneticArguments.h++"
    #include "geneticResult.h++"
    #include <vector>
    #include <logging/logger.h++>

    namespace Tyrant {
        namespace Genetic {

            typedef std::vector<Core::DeckTemplate::ConstPtr> CDeckVector;
            typedef std::vector<Core::StaticDeckTemplate::ConstPtr> CSDeckVector;
            typedef CSDeckVector Population;

            class GeneticAlgorithm {
                public:
                    typedef std::shared_ptr<GeneticAlgorithm> Ptr;

                private:
                    Praetorian::Basics::Logging::Logger::Ptr logger;
                    Core::SimulatorCore::Ptr simulator;
                    Mutator::Mutator::Ptr mutator;
                    bool aborted;

                private: // methods
                    Core::StaticDeckTemplate::ConstPtr mutate
                        (Core::StaticDeckTemplate::ConstPtr deck);

                    Core::StaticDeckTemplate::ConstPtr generateRandomMutation
                        (Population const & population);

                    void preMutate
                        (GeneticArguments const & arguments
                        ,Population & population);

                    Population selectStep
                        (GeneticArguments const & arguments
                        ,Population population);

                    Population reproduceStep
                        (Praetorian::Basics::Logging::Logger::Ptr logger
                        ,GeneticArguments const & arguments
                        ,Population population);

                    void sortAndCrop
                        (GeneticArguments const & arguments
                        ,Population & decks
                        ,unsigned int targetSize);

                public: // methods
                    GeneticAlgorithm
                        (Praetorian::Basics::Logging::Logger::Ptr logger
                        ,Core::SimulatorCore::Ptr simulator
                        ,Mutator::Mutator::Ptr mutator
                        );

                    GeneticResult evolve(GeneticArguments const & arguments);

                    void abort();

            };
        }
    }
#endif
