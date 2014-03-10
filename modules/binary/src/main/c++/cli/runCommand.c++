#include "runCommand.h++"

#include "configuration.h++"
#include <errorHandling/exceptions.h++>
#include <mutator/mutator.h++>
#include <core/simulatorCore.h++>
#include "../genetic/genetic.h++"
#include <errorHandling/assert.h++>


namespace TyrantGenetic {
    namespace CLI {

        RunCommand::RunCommand(Configuration configuration
                              )
        : Command(configuration)
        {
            Praetorian::Basics::Logging::Logger::Ptr rootLogger = configuration.constructRootLogger();
            assertX(rootLogger);
            Tyrant::Core::SimulatorCore::Ptr simulator
                = this->configuration.constructCore();
            Tyrant::Mutator::Mutator::Ptr mutator
                = this->configuration.constructMutator();
            this->geneticAlgorithm = Tyrant::Genetic::GeneticAlgorithm::Ptr(
                new Tyrant::Genetic::GeneticAlgorithm(rootLogger, simulator, mutator)
            );
        }

        RunCommand::~RunCommand()
        {
        }

        int RunCommand::execute() {
            Tyrant::Genetic::GeneticResult result = this->geneticAlgorithm->evolve(this->task);

            for(Core::DeckTemplate::ConstPtr deck : result) {
                std::clog << std::string(*deck) << std::endl;
            }
            return 0;
        }

        void
        RunCommand::abort()
        {
            this->geneticAlgorithm->abort();
        }

    }
}
