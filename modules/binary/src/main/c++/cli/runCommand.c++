#include "runCommand.h++"

#include "configuration.h++"
#include <errorHandling/exceptions.h++>
#include <mutator/mutator.h++>
#include <core/simulatorCore.h++>
#include "../genetic/genetic.h++"
#include <errorHandling/assert.h++>
#include "deckParser.h++"

namespace TyrantGenetic {
    namespace CLI {

        RunCommand::RunCommand(Configuration configuration
                              ,bool attackerFromStdIn)
        : Command(configuration)
        , attackerFromStdIn(attackerFromStdIn)
        {
            Praetorian::Basics::Logging::Logger::Ptr rootLogger = configuration.constructRootLogger();
            assertX(rootLogger);
            Tyrant::Core::SimulatorCore::Ptr simulator
                = this->configuration.constructCore();
            Tyrant::Mutator::Mutator::Ptr mutator
                = this->configuration.constructMutator();
            int verbosity = configuration.verbosity;
            this->geneticAlgorithm = Tyrant::Genetic::GeneticAlgorithm::Ptr(
                new Tyrant::Genetic::GeneticAlgorithm(rootLogger, simulator, mutator, verbosity)
            );
        }

        RunCommand::~RunCommand()
        {
        }

        int RunCommand::execute() {
            if (this->attackerFromStdIn) {
                std::string line;
                while (std::getline(std::cin, line) && !this->isAborted()) {
                    Core::DeckTemplate::ConstPtr attackerDeck = TyrantCache::CLI::parseDeck(line);
                    if (Core::StaticDeckTemplate::ConstPtr sDeck = std::dynamic_pointer_cast<Core::StaticDeckTemplate const>(attackerDeck)) {
                        this->task.initialPopulation.insert(sDeck);
                    } else {
                        throw InvalidUserInputError("Can only work with static decks.");
                    }
                }
            }
            Tyrant::Genetic::GeneticResult result = this->geneticAlgorithm->evolve(this->task);

            for(Core::DeckTemplate::ConstPtr deck : result) {
                std::cout << std::string(*deck) << std::endl;
            }
            return 0;
        }

        void
        RunCommand::abort()
        {
            Command::abort();
            this->geneticAlgorithm->abort();
        }

    }
}
