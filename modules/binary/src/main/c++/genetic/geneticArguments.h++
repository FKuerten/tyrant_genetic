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
                double parentDeathProbability;
                unsigned int numberOfChildren;
                double childMutationProbability;
                unsigned int numberOfGenerations;
                bool byArd;
                Core::SimulationTask simulationTask;

                GeneticArguments();
            };



        }
    }
#endif
