#include "genetic.h++"
#include <errorHandling/exceptions.h++>
#include <cstdlib>
#include <core/autoDeckTemplate.h++>
#include <core/simpleOrderedDeckTemplate.h++>
#include <boost/iterator/transform_iterator.hpp>
#include <errorHandling/assert.h++>
#include <algorithm>
#include <logging/indentedLogger.h++>
#include <logging/nullLogger.h++>
#include <limits>

using namespace Praetorian::Basics::Logging;
namespace Tyrant {
    namespace Genetic {

         class DeckComparator {
            Core::SimulatorCore & simulator;
            Mutator::Mutator & mutator;
            GeneticArguments const & arguments;
            bool ascending;
            Logger::Ptr logger;

            public:
                DeckComparator
                    (Core::SimulatorCore & simulator
                    ,Mutator::Mutator & mutator
                    ,GeneticArguments const & arguments
                    ,bool ascending
                    ,Logger::Ptr logger
                    )
                : simulator(simulator)
                , mutator(mutator)
                , arguments(arguments)
                , ascending(ascending)
                , logger(logger)
                {}

                double getScore(Core::StaticDeckTemplate::ConstPtr deck)
                {
                    bool const DEBUG_GET_SCORE = false;
                    assertX(deck.get() != nullptr);
                    Core::SimulationTask task = this->arguments.simulationTask;
                    if (this->arguments.mutateAttacker) {
                        task.attacker = deck;
                    } else {
                        task.defender = deck;
                    }
                    if (DEBUG_GET_SCORE) {
                        this->logger->write("::getScore(")->write(*deck)->writeln(")");
                        this->logger->write("    task.attacker=")->writeln(*task.attacker);
                        this->logger->write("    task.defender=")->writeln(*task.defender);
                        this->logger->write("    task.surge=")->writeln(task.surge);
                        this->logger->write("    task.useRaidRules=");
                        if(task.useRaidRules == Core::tristate::TRUE) {
                            this->logger->writeln("true");
                        } else if(task.useRaidRules == Core::tristate::FALSE) {
                            this->logger->writeln("false");
                        } else {
                            this->logger->writeln("undefined");
                        }
                    }

                    Core::SimulationResult result = this->simulator.simulate(task);
                    if (DEBUG_GET_SCORE) {
                        this->logger->write("    result.gamesWon=")->writeln(result.gamesWon);
                        this->logger->write("    result.numberOfGames=")->writeln(result.numberOfGames);
                        this->logger->write("    arguments.byPoints=")->writeln(arguments.byPoints);
                    }
                    double score;
                    if (this->arguments.byPoints) {
                        score = result.getManualANPAttacker();
                    } else {
                        score = result.getWinRate();
                    }
                    if (DEBUG_GET_SCORE) {
                        this->logger->write("::getScore(")->write(*deck)->write(")")
                                    ->write(" = ")->write(score)
                                    ->writeln();
                    }
                    return score;
                }

                bool operator() (Core::StaticDeckTemplate::ConstPtr a, Core::StaticDeckTemplate::ConstPtr b)
                {
                    bool const aValid = this->mutator.isValid(*a);
                    bool const bValid = this->mutator.isValid(*b);
                    if (!aValid && bValid) {
                        return this->ascending;
                    } else if (aValid && !bValid) {
                        return !this->ascending;
                    }
                    bool const aComposable = this->mutator.canCompose(*a);
                    bool const bComposable = this->mutator.canCompose(*a);
                    if (!aComposable && bComposable) {
                        return this->ascending;
                    } else if (aComposable && !bComposable) {
                        return !this->ascending;
                    }
                    double const aScore = this->getScore(a);
                    double const bScore = this->getScore(b);
                    if (this->ascending) {
                        return aScore < bScore;
                    } else {
                        return aScore > bScore;
                    }
                }
        };

        //##############################################################
        //##                   Constructors                           ##
        //##############################################################
        GeneticAlgorithm::GeneticAlgorithm
            (Praetorian::Basics::Logging::Logger::Ptr logger
            ,Core::SimulatorCore::Ptr simulator
            ,Mutator::Mutator::Ptr mutator
            ,int verbosity
            )
        : logger(logger)
        , simulator(simulator)
        , mutator(mutator)
        , aborted(false)
        , verbosity(verbosity)
        {
        }

        //##############################################################
        //##                      Helpers                             ##
        //##############################################################

        Logger::Ptr
        GeneticAlgorithm::getSubLogger(Logger::Ptr logger, int subLevel)
        {
            if(subLevel <= this->verbosity) {
                return Logger::Ptr(
                    new IndentedLogger(logger, "    ")
                );
            } else {
                return Logger::Ptr(
                    new NullLogger()
                );
            }
        }

        void
        GeneticAlgorithm::printStatistics
            (Logger::Ptr logger
            ,GeneticArguments const & arguments
            ,Population const & population
            )
        {
            if (!logger->isEnabled()) {
                return;
            }

            size_t const size = population.size();

            logger->write("Population size:    ")->writeln(size);

            Core::StaticDeckTemplate::ConstPtr bestDeck, worstDeck;
            double bestScore = std::numeric_limits<double>::min();
            double worstScore = std::numeric_limits<double>::max();
            std::vector<double> scores;

            DeckComparator compare(*this->simulator
                                  ,*this->mutator
                                  ,arguments
                                  ,false
                                  ,logger
                                  );
            for(Core::StaticDeckTemplate::ConstPtr deck : population) {
                double score = compare.getScore(deck);
                if (score > bestScore) {
                    bestDeck = deck;
                    bestScore = score;
                }
                if (score < worstScore) {
                    worstDeck = deck;
                    worstScore = score;
                }
                scores.push_back(score);
            }
            logger->write("           best:    ")->write(bestScore)
                  ->write(" with ")->writeln(*bestDeck)
                  ->write("           worst:   ")->write(worstScore)
                  ->write(" with ")->writeln(*worstDeck);

            std::sort(scores.begin(), scores.end());

            double sum = 0;
            for(double score : scores) {
                sum+=score;
            }
            double const average = sum / size;
            logger->write("           average: ")->write(average);

            double median;
            if (size % 2 == 0) {
                median = (scores[size/2-1] + scores[size/2]) / 2;
            } else {
                median = scores[size / 2];
            }
            logger->write("           median:  ")->write(median)
                  ->writeln();
        }

        //##############################################################
        //##                      Level 0                             ##
        //##############################################################
        GeneticResult
        GeneticAlgorithm::evolve
            (GeneticArguments const & arguments
            )
        {
            // Store our population, initially the initial population,
            // will be updated.
            Population population(arguments.initialPopulation.cbegin()
                                  ,arguments.initialPopulation.cend()
                                  );
            this->logger->write("Starting genetic algorithm with initial population of ")
                        ->writeln(population.size());
            Logger::Ptr subLogger = this->getSubLogger(this->logger, 2);
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
                    this->logger->write("Reducing population size to ")
                                ->write(population.size())
                                ->writeln(".");
                }

                // Increase population again
                population = this->reproduceStep
                    (subLogger
                    ,arguments
                    ,generationIndex
                    , population
                    );

                // Statistics
                this->printStatistics(this->logger, arguments, population);

                if(this->aborted) {
                    throw AbortionException("Aborted!");
                }

            }
            return GeneticResult(population.begin(), population.end());
        }

        //##############################################################
        //##                     Level >=1                            ##
        //##############################################################
        Core::StaticDeckTemplate::ConstPtr
        GeneticAlgorithm::mutate
            (Core::StaticDeckTemplate::ConstPtr deck
            )
        {
            //Mutator::MutationTask mutationTask;
            //mutationTask.baseDeck = deck;
            //Mutator::MutationResult mutationResult
                //= this->mutator->mutate(mutationTask);
            //CSDeckVector mutations(mutationResult.begin, mutationResult.end);
            //unsigned int size = mutations.size();
            //unsigned int index = static_cast<unsigned int>(rand()) % size;
            //Core::StaticDeckTemplate::ConstPtr element
                //= mutations[index];
            //return element;
            return this->mutator->quickMutate(deck);
        }

        Core::StaticDeckTemplate::ConstPtr
        GeneticAlgorithm::generateRandomMutation
            (Population const & population
            )
        {
            unsigned int size = population.size();
            assertGT(size, 0u);
            unsigned int index = static_cast<unsigned int>(rand()) % size;
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
                if (this->aborted) {
                    throw new AbortionException("Aborted in preMutate");
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
            unsigned int index = static_cast<unsigned int>(rand()) % size;
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


        void
        GeneticAlgorithm::sortAndCrop
            (GeneticArguments const & arguments
            ,Population & decks
            ,unsigned int targetSize)
        {
            DeckComparator compare(*this->simulator
                                  ,*this->mutator
                                  ,arguments
                                  ,false
                                  ,this->logger
                                  );
            if (false) {
                // Unfortunately compare might not be a strict weak ordering
                // this is very unlikely, but might happen.
                // If we use a cache it is probably impossible.
                // We do an expensive check now:
                std::clog << "Expensive pre-check for sort." << std::endl;
                for (auto a : decks) {
                    //
                    if(compare(a,a)) {
                        std::stringstream ssMessage;
                        ssMessage << "Something broke: va<va (this violates irreflexivity)" << std::endl;
                        ssMessage << "Where va=" << compare.getScore(a),
                        ssMessage << " a=" << std::string(*a) << std::endl;
                        throw LogicError(ssMessage.str());
                    }
                    for (auto b : decks) {
                        bool const lt = compare(a,b);
                        bool const gt = compare(b,a);
                        if (lt && gt) {
                            std::stringstream ssMessage;
                            ssMessage << "Something broke: va<vb && vb<va (this violates asymmetry)" << std::endl;
                            ssMessage << "Where va=" << compare.getScore(a),
                            ssMessage << " a=" << std::string(*a) << std::endl;
                            ssMessage << "      vb=" << compare.getScore(b) << std::endl;
                            ssMessage << " b=" << std::string(*b) << std::endl;
                            throw LogicError(ssMessage.str());
                        }
                        if(lt) {
                            for (auto c : decks) {
                                bool const lt2 = compare(b,c);
                                bool const lt3 = compare(a,c);
                                if(lt2 && !lt3) {
                                    std::stringstream ssMessage;
                                    ssMessage << "Something broke: va<vb && vb<vc but va!<vc (this violates transitivity)" << std::endl;
                                    ssMessage << "Where va=" << compare.getScore(a),
                                    ssMessage << " a=" << std::string(*a) << std::endl;
                                    ssMessage << "      vb=" << compare.getScore(b) << std::endl;
                                    ssMessage << " b=" << std::string(*b) << std::endl;
                                    ssMessage << "      vc=" << compare.getScore(c) << std::endl;
                                    ssMessage << " c=" << std::string(*c) << std::endl;
                                    throw LogicError(ssMessage.str());
                                }
                            }
                        }
                    }
                }
                // Check that each pointer points to something valid.
                for (auto a: decks) {
                    assertX(a.get() != nullptr);
                    assertGT(std::string(*a).size(),0u);
                }
            }
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
            while(   population.size() >= numberOfDecksToPick
                  && population.size() + result.size() > arguments.minPopulationSize
                 )
            {
                // Randomly pick decks
                Population pickedDecks
                    = pickAndRemoveRandomElements
                        (population, numberOfDecksToPick);
                // Find best deck
                this->sortAndCrop(arguments, pickedDecks, numberOfDecksToKeep);
                result.insert(result.end(), pickedDecks.cbegin(), pickedDecks.cend());
            }
            result.insert(result.end(), population.cbegin(), population.cend());
            if(this->aborted) {
                throw AbortionException("Aborted!");
            }
            return result;
        }

        double randomDouble() {
            return static_cast<double>(rand())
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
            return parents[static_cast<unsigned int>(rand()) % size];
        }

        unsigned int
        recombinateCard
            (GeneticArguments const & arguments
            ,std::vector<unsigned int> parents
            )
        {
            unsigned int size = parents.size();
            return parents[static_cast<unsigned int>(rand()) % size];
        }

        unsigned int
        recombinateUnsignedInt
            (GeneticArguments const & arguments
            ,std::vector<unsigned int> parents
            )
        {
            unsigned int size = parents.size();
            return parents[static_cast<unsigned int>(rand()) % size];
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
            ,unsigned int generationIndex
            ,Population population)
        {
            double const parentDeathProbability
                = arguments.parentDeathProbability(generationIndex);
            double const childMutationProbability
                = arguments.childMutationProbability(generationIndex);
            unsigned int const numberOfParents = 2;
            Population result;
            while(   population.size() >= numberOfParents
                  && population.size() + result.size() < arguments.maxPopulationSize
                 )
            {
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
                    if (randomDouble() >= parentDeathProbability) {
                        result.push_back(parent);
                    } else {
                        logger->write(" (deceased)");
                    }
                    first = false;
                }}
                logger->write(" produced ");

                // Generate children
                {bool first = true; for(unsigned int i = 0; i < arguments.numberOfChildren; i++) {
                    if (!first) {
                        logger->write(" and ");
                    }
                    // Recombinate one child
                    Core::StaticDeckTemplate::ConstPtr child =
                        recombinateDeck(arguments, parents);

                    while (randomDouble() <= childMutationProbability) {
                        child = mutate(child);
                    }
                    if (this->mutator->isValid(*child)) {
                        result.push_back(child);
                        logger->write(*child);
                        first = false;
                    }
                }}
                logger->writeln(".");
            }
            result.insert(result.end(), population.cbegin(), population.cend());
            return result;
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
