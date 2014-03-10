#include "geneticArguments.h++"

namespace Tyrant {
    namespace Genetic {

        GeneticArguments::GeneticArguments()
        : initialPopulation()
        , minPopulationSize(10)
        , maxPopulationSize(1000)
        , parentDeathProbability(0.25)
        , numberOfChildren(4)
        , childMutationProbability(0.01)
        , numberOfGenerations(10)
        , byArd(false)
        {
        }

    }
}
