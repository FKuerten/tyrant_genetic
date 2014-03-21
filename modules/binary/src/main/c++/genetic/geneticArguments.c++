#include "geneticArguments.h++"

namespace Tyrant {
    namespace Genetic {

        GeneticArguments::GeneticArguments()
        : initialPopulation()
        , minPopulationSize(1000)
        , maxPopulationSize(2000)
        , parentDeathProbability(0.15)
        , numberOfChildren(10)
        , childMutationProbability(0.15)
        , numberOfGenerations(20)
        , byPoints(false)
        , mutateAttacker(true)
        {
        }

    }
}
