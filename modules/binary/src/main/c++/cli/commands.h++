#ifndef TYRANT_MUTATOR_CLI_COMMANDS_HPP
    #define TYRANT_MUTATOR_CLI_COMMANDS_HPP

    // for std::shared_ptr
    #include <memory>
    #include <boost/program_options/options_description.hpp>
    #include "configuration.h++"

    namespace TyrantGenetic {
        namespace CLI {

            class Command {
                private:
                    bool aborted;

                protected:
                    bool isAborted();
                    Configuration configuration;

                public:
                    typedef std::shared_ptr<Command> Ptr;
                public:
                    Command(Configuration configuration);
                    virtual ~Command();
                    virtual int execute() = 0;
                    virtual void abort();
            };

            class VersionCommand : public Command {
                public:
                    VersionCommand(Configuration configuration);
                    int execute();
            };

            class CoreVersionCommand : public Command {
                public:
                    CoreVersionCommand(Configuration configuration);
                    int execute();
            };

            class HelpCommand : public Command {
                private:
                    boost::program_options::options_description desc;
                public:
                    HelpCommand(Configuration configuration, boost::program_options::options_description const & desc);
                    int execute();
            };

        }
    }
#endif
