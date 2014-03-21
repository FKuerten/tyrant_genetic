#ifndef TYRANT_MUTATOR_CLI_RUNCOMMAND_HPP
    #define TYRANT_MUTATOR_CLI_RUNCOMMAND_HPP

    #include "commands.h++"
    #include <memory>
    #include "../genetic/genetic.h++"
    #include "../genetic/geneticArguments.h++"

    using namespace Tyrant;
    namespace TyrantGenetic {
        namespace CLI {

            class RunCommand : public Command {
                public:
                    typedef std::shared_ptr<RunCommand> Ptr;

                public:
                    Genetic::GeneticArguments task;
                    bool attackerFromStdIn;

                private:
                    Tyrant::Genetic::GeneticAlgorithm::Ptr geneticAlgorithm;
                public:
                    RunCommand(Configuration, bool attackerFromStdIn);
                    ~RunCommand();

                    int execute();
                    void abort();
            };
        }
    }

#endif
