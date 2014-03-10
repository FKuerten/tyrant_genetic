#include "genetic.h++"
#include <errorHandling/exceptions.h++>
#include <cstdlib>
#include <core/autoDeckTemplate.h++>
#include <core/simpleOrderedDeckTemplate.h++>
#include <boost/iterator/transform_iterator.hpp>
#include <errorHandling/assert.h++>
#include <algorithm>
#include <logging/indentedLogger.h++>

using namespace Praetorian::Basics::Logging;
namespace Tyrant {
    namespace Genetic {

        GeneticAlgorithm::GeneticAlgorithm
            (Praetorian::Basics::Logging::Logger::Ptr logger
            ,Core::SimulatorCore::Ptr simulator
            ,Mutator::Mutator::Ptr mutator
            )
        : logger(logger)
        , simulator(simulator)
        , mutator(mutator)
        , aborted(false)
        {
        }

        Core::StaticDeckTemplate::ConstPtr
        GeneticAlgorithm::mutate
            (Core::StaticDeckTemplate::ConstPtr deck
            )
        {
            Mutator::MutationTask mutationTask;
            mutationTask.baseDeck = deck;
            Mutator::MutationResult mutationResult
                = this->mutator->mutate(mutationTask);
            CSDeckVector mutations(mutationResult.begin, mutationResult.end);
            unsigned int size = mutations.size();
            unsigned int index = rand() % size;
            Core::StaticDeckTemplate::ConstPtr element
                = mutations[index];
            return element;
        }

        Core::StaticDeckTemplate::ConstPtr
        GeneticAlgorithm::generateRandomMutation
            (Population const & population
            )
        {
            unsigned int size = population.size();
            unsigned int index = rand() % size;
            Core::StaticDeckTemplate::ConstPtr element
                = population[index];
            return this->mutate(element);
        }

        void
        GeneticAlgorithm::preMutate
            (GeneticArguments const & arguments
            ,Population & population
            )
        {
            assertX(this->logger);
            this->logger->write("Premutating... ");
            unsigned int failures = 0;
            unsigned int const maxFailures = 1000;
            unsigned int populationSize;
            while((populationSize = population.size()) < arguments.minPopulationSize) {
                Core::StaticDeckTemplate::ConstPtr randomMutation
                    = this->generateRandomMutation(population);
                population.push_back(randomMutation);
                if (population.size() <= populationSize) {
                    failures++;
                    if (failures >= maxFailures) {
                        throw InvalidUserInputError("Cannot find enough mutations.");
                    }
                } else {
                    failures = 0;
                }
            }
            this->logger->writeln("done.");
        }

        template <class T>
        T
        pickAndRemoveRandomElement
            (std::vector<T> & population)
        {
            unsigned int size = population.size();
            unsigned int index = rand() % size;
            T element = population[index];
            population.erase(population.begin() + index);
            return element;
        }

        Population
        pickAndRemoveRandomElements
            (Population & population
            ,unsigned int numberOfDecksToPick
            )
        {
            // Randomly pick decks
            Population pickedDecks;
            for(unsigned int i = 0; i < numberOfDecksToPick; i++) {
                Core::StaticDeckTemplate::ConstPtr pickedDeck
                    = pickAndRemoveRandomElement
                        (population);
                pickedDecks.push_back(pickedDeck);
            }
            return pickedDecks;
        }

        class DeckComparator {
            Core::SimulatorCore::Ptr simulator;
            GeneticArguments const & arguments;
            bool ascending;
            Logger::Ptr logger;
            bool changeAttacker = true;

            public:
                DeckComparator
                    (Core::SimulatorCore::Ptr simulator
                    ,GeneticArguments const & arguments
                    ,bool ascending
                    ,Logger::Ptr logger
                    )
                : simulator(simulator)
                , arguments(arguments)
                , ascending(ascending)
                , logger(logger)
                {}

                bool operator() (Core::StaticDeckTemplate::ConstPtr a, Core::StaticDeckTemplate::ConstPtr b)
                {
                    //this->logger->write("Comparing ")->write(*a);
                    //this->logger->write(" with ")->write(*b);
                    Core::SimulationTask taskA = this->arguments.simulationTask;
                    Core::SimulationTask taskB = this->arguments.simulationTask;
                    if (this->changeAttacker) {
                        taskA.attacker = a;
                        taskB.attacker = b;
                    } else {
                        taskA.defender = a;
                        taskB.defender = b;
                    }
                    Core::SimulationResult resultA = this->simulator->simulate(taskA);
                    Core::SimulationResult resultB = this->simulator->simulate(taskB);
                    bool smaller;
                    if (this->arguments.byArd) {
                        smaller = resultA.getManualANPAttacker() < resultB.getManualANPAttacker();
                    } else {
                        smaller = resultA.getWinRate() < resultB.getWinRate();
                    }
                    //this->logger->writeln();
                    return this->ascending ? smaller : !smaller;
                }
        };

        void
        GeneticAlgorithm::sortAndCrop
            (GeneticArguments const & arguments
            ,Population & decks
            ,unsigned int targetSize)
        {
            DeckComparator compare(this->simulator, arguments, false, this->logger);
            std::sort(decks.begin(), decks.end(), compare);
            decks.resize(targetSize);
        }

        Population
        GeneticAlgorithm::selectStep
            (GeneticArguments const & arguments
            ,Population population)
        {
            unsigned int const numberOfDecksToPick = 2;
            unsigned int const numberOfDecksToKeep = 1;
            Population result;
            while(population.size() >= numberOfDecksToPick) {
                // Randomly pick decks
                Population pickedDecks
                    = pickAndRemoveRandomElements
                        (population, numberOfDecksToPick);
                // Find best deck
                this->sortAndCrop(arguments, pickedDecks, numberOfDecksToKeep);
                result.insert(result.end(), pickedDecks.cbegin(), pickedDecks.cend());
            }
            result.insert(result.end(), population.cbegin(), population.cend());
            return result;
        }

        double randomDouble() {
            return static_cast<double>(random())
                /
                static_cast<double>(RAND_MAX);
        }

        bool
        isOrdered
            (Core::StaticDeckTemplate::ConstPtr deck
            )
        {
            if (std::dynamic_pointer_cast<Core::SimpleOrderedDeckTemplate const>(deck)) {
                return true;
            } else {
                return false;
            }
        }

        unsigned int
        getCommanderId
            (Core::StaticDeckTemplate::ConstPtr deck
            )
        {
            return deck->getCommanderId();
        }

        unsigned int
        getSize
            (Core::StaticDeckTemplate::ConstPtr deck
            )
        {
            return deck->getNumberOfNonCommanderCards();
        }

        std::vector<unsigned int>
        getCards
            (Core::StaticDeckTemplate::ConstPtr deck
            )
        {
            std::vector<unsigned int> result;
            if (auto oDeck = std::dynamic_pointer_cast<Core::SimpleOrderedDeckTemplate const>(deck)) {
                for(unsigned int i = 0; i < oDeck->getNumberOfNonCommanderCards(); i++) {
                    result.push_back(oDeck->getCardIdAtIndex(i));
                }
            } else if (auto aDeck = std::dynamic_pointer_cast<Core::AutoDeckTemplate const>(deck)) {
                std::vector<unsigned int> temp;
                for(unsigned int i = 0; i < aDeck->getNumberOfNonCommanderCards(); i++) {
                    temp.push_back(aDeck->getCardIdAtIndex(i));
                }
                while(!temp.empty()) {
                    unsigned int cardId = pickAndRemoveRandomElement(temp);
                    result.push_back(cardId);
                }
            }
            return result;
        }

        bool
        recombinateBool
            (GeneticArguments const & arguments
            ,std::vector<bool> parents
            )
        {
            unsigned int size = parents.size();
            return parents[random() % size];
        }

        unsigned int
        recombinateCard
            (GeneticArguments const & arguments
            ,std::vector<unsigned int> parents
            )
        {
            unsigned int size = parents.size();
            return parents[random() % size];
        }

        unsigned int
        recombinateUnsignedInt
            (GeneticArguments const & arguments
            ,std::vector<unsigned int> parents
            )
        {
            unsigned int size = parents.size();
            return parents[random() % size];
        }

        std::vector<unsigned int>
        recombinateCards
            (GeneticArguments const & arguments
            ,unsigned int size
            ,std::vector<std::vector<unsigned int>> parents
            )
        {
            std::vector<unsigned int> result;
            for(unsigned int index = 0; index < size; index++) {
                // Fill temp with the "genes" left.
                std::vector<unsigned int> temp;
                for(std::vector<unsigned int> parent : parents) {
                    if (parent.size() > index) {
                        unsigned int cardId = parent[index];
                        temp.push_back(cardId);
                    }
                }
                assertX(!temp.empty());
                unsigned int cardId = recombinateCard(arguments, temp);
                result.push_back(cardId);
            }
            return result;
        }

        Core::StaticDeckTemplate::ConstPtr
        recombinateDeck
            (GeneticArguments const & arguments
            ,Population const & parents)
        {
            // We treat each deck as a series of twelve genes.
            // The first gene defines whether the deck is ordered.
            // The second gene is the commander.
            // The remaining genes are the cards with the addional option to be empty.
            // These genes will be recombinated.

            bool const willBeOrdered = recombinateBool(arguments,
                std::vector<bool>
                    (boost::make_transform_iterator(parents.cbegin(), isOrdered)
                    ,boost::make_transform_iterator(parents.cend(), isOrdered)
                    )
            );
            unsigned int const commanderId = recombinateCard(arguments,
                std::vector<unsigned int>
                    (boost::make_transform_iterator(parents.cbegin(), getCommanderId)
                    ,boost::make_transform_iterator(parents.cend(), getCommanderId)
                    )
            );
            unsigned int newSize = recombinateUnsignedInt(arguments,
                std::vector<unsigned int>
                    (boost::make_transform_iterator(parents.cbegin(), getSize)
                    ,boost::make_transform_iterator(parents.cend(), getSize)
                    )
            );
            std::vector<unsigned int> const newCards = recombinateCards(arguments, newSize,
                std::vector<std::vector<unsigned int>>
                    (boost::make_transform_iterator(parents.cbegin(), getCards)
                    ,boost::make_transform_iterator(parents.cend(), getCards)
                    )
            );
            if (willBeOrdered) {
                return Core::StaticDeckTemplate::ConstPtr(
                    new Core::SimpleOrderedDeckTemplate(
                        commanderId, newCards
                    )
                );
            } else {
                return Core::StaticDeckTemplate::ConstPtr(
                    new Core::AutoDeckTemplate(
                        commanderId, newCards
                    )
                );

            }
        }

        Population
        GeneticAlgorithm::reproduceStep
            (Praetorian::Basics::Logging::Logger::Ptr logger
            ,GeneticArguments const & arguments
            ,Population population)
        {
            unsigned int const numberOfParents = 2;
            Population result;
            while(population.size() >= numberOfParents) {
                logger->write("Parents ");
                // Randomly pick decks
                Population parents
                    = pickAndRemoveRandomElements
                        (population, numberOfParents);

                // Check if parents survived
                {bool first = true; for(Core::StaticDeckTemplate::ConstPtr parent : parents) {
                    if(!first) {
                        logger->write(" and ");
                    }
                    logger->write(*parent);
                    if (randomDouble() >= arguments.parentDeathProbability) {
                        result.push_back(parent);
                    } else {
                        logger->write(" (deceased)");
                    }
                    first = false;
                }}
                logger->write(" produced ");

                // Genereate children
                {bool first = true; for(unsigned int i = 0; i < arguments.numberOfChildren; i++) {
                    if(!first) {
                        logger->write(" and ");
                    }
                    // Recombinate one child
                    Core::StaticDeckTemplate::ConstPtr child =
                        recombinateDeck(arguments, parents);
                    while (randomDouble() <= arguments.childMutationProbability) {
                        child = mutate(child);
                    }
                    result.push_back(child);
                    logger->write(*child);
                    first = false;
                }}
                logger->writeln(".");
            }
            result.insert(result.end(), population.cbegin(), population.cend());
            return result;
        }

        GeneticResult
        GeneticAlgorithm::evolve
            (GeneticArguments const & arguments
            )
        {
            Population population(arguments.initialPopulation.cbegin()
                                  ,arguments.initialPopulation.cend()
                                  );
            Logger::Ptr subLogger = Logger::Ptr(
                new IndentedLogger(this->logger, "    ")
            );
            this->preMutate(arguments, population);
            unsigned int const populationTreshold =
                (arguments.minPopulationSize + arguments.maxPopulationSize) /2;
            for(unsigned int generationIndex = 0
               ;generationIndex < arguments.numberOfGenerations
               ;generationIndex++
               )
            {
                this->logger->write("Beginning generation ")
                            ->write(generationIndex)
                            ->write(" with size ")
                            ->write(population.size())
                            ->writeln();

                // Reduce population
                while(population.size() > populationTreshold) {
                    population = this->selectStep(arguments, population);
                    if(this->aborted) {
                        throw AbortionException("Aborted!");
                    }
                }

                // Increase population again
                population = this->reproduceStep(subLogger, arguments, population);
                if(this->aborted) {
                    throw AbortionException("Aborted!");
                }
            }
            return GeneticResult(population.begin(), population.end());
        }

        void
        GeneticAlgorithm::abort
            ()
        {
            this->aborted = true;
            this->mutator->abort();
            this->simulator->abort();
        }

    }
}
