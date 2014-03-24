#ifndef TYRANT_GENETIC_GENETICARGUMENTS_HPP
    #define TYRANT_GENETIC_GENETICARGUMENTS_HPP

    #include <mutator/derefCompareLT.h++>
    #include <core/simulationTask.h++>

    namespace Tyrant {
        namespace Genetic {

            struct GeneticArguments {
                Mutator::CSDeckSet initialPopulation;
                unsigned int minPopulationSize;
                unsigned int maxPopulationSize;
                double initialParentDeathProbability;
                double parentDeathProbabilityPerGeneration;
                unsigned int numberOfChildren;
                double initialChildMutationProbability;
                double childMutationProbabilityPerGeneration;
                unsigned int numberOfGenerations;
                bool byPoints;
                bool mutateAttacker;
                Core::SimulationTask simulationTask;

                GeneticArguments();

                double parentDeathProbability(unsigned int const generation) const;
                double childMutationProbability(unsigned int const generation) const;
            };



        }
    }
#endif
