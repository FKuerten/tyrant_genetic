#include "geneticArguments.h++"

namespace Tyrant {
    namespace Genetic {

        GeneticArguments::GeneticArguments()
        : initialPopulation()
        , minPopulationSize(1000)
        , maxPopulationSize(10000)
        , initialParentDeathProbability(0.15)
        , parentDeathProbabilityPerGeneration(-0.01)
        , numberOfChildren(3)
        , initialChildMutationProbability(0.70)
        , childMutationProbabilityPerGeneration(-0.03)
        , numberOfGenerations(50)
        , byPoints(false)
        , mutateAttacker(true)
        {
        }

        double GeneticArguments::childMutationProbability
            (unsigned int generation
            ) const
        {
            double p = this->initialChildMutationProbability
                     + generation * this->childMutationProbabilityPerGeneration
                     ;
            if (p < 0) {
                return 0;
            } else if (p > 1) {
                return 1;
            } else {
                return p;
            }
        }

        double GeneticArguments::parentDeathProbability
            (unsigned int generation
            ) const
        {
            double p = this->initialParentDeathProbability
                     + generation * this->parentDeathProbabilityPerGeneration
                     ;
            if (p < 0) {
                return 0;
            } else if (p > 1) {
                return 1;
            } else {
                return p;
            }
        }

    }
}
