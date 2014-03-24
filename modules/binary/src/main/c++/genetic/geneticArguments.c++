#include "geneticArguments.h++"

namespace Tyrant {
    namespace Genetic {

        GeneticArguments::GeneticArguments()
        : initialPopulation()
        , minPopulationSize(1000)
        , maxPopulationSize(2000)
        , initialParentDeathProbability(0.15)
        , parentDeathProbabilityPerGeneration(-0.01)
        , numberOfChildren(10)
        , initialChildMutationProbability(0.15)
        , childMutationProbabilityPerGeneration(-0.01)
        , numberOfGenerations(20)
        , byPoints(false)
        , mutateAttacker(true)
        {
        }

        double GeneticArguments::childMutationProbability
            (unsigned int generation
            ) const
        {
            return this->initialChildMutationProbability
                 + generation * this->childMutationProbabilityPerGeneration
                 ;
        }

        double GeneticArguments::parentDeathProbability
            (unsigned int generation
            ) const
        {
            return this->initialParentDeathProbability
                 + generation * this->parentDeathProbabilityPerGeneration
                 ;
        }

    }
}
