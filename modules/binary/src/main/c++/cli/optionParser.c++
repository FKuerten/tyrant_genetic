#include "optionParser.h++"

#include <boost/program_options/parsers.hpp>
//#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>

#include <errorHandling/exceptions.h++>
#include "runCommand.h++"
#include "deckParser.h++"

namespace po = boost::program_options;
namespace TyrantGenetic {
    namespace CLI {

        Command::Ptr parseArguments(int argc
                                   ,char const * const * argv
                                   )
        {
            try {
                Configuration configuration;
                std::string deckDescription;

                po::options_description desc("Allowed options");
                desc.add_options()
                    ("help,h", "produce help message")
                    ("version,V", "version information")
                    ("core-version", "version of the core")
                    ("verbose,v", "increase verbosity")
                    ("base-deck"
                    ,po::value<std::string>(&deckDescription)
                    ,"the deck to mutate"
                    )
                ;
                po::positional_options_description poDesc;
                poDesc.add("base-deck", 1);

                po::variables_map vm;
                po::store(po::command_line_parser(argc, argv)
                            .options(desc)
                            .positional(poDesc)
                            .run()
                         ,vm
                         );

                if (vm.count("help")) {
                    return Command::Ptr(new HelpCommand(configuration, desc));
                } else if (vm.count("core-version")) {
                    return Command::Ptr(new CoreVersionCommand(configuration));
                } else if (vm.count("version")) {
                    return Command::Ptr(new VersionCommand(configuration));
                }

                po::notify(vm);

                // check some argument relation
                if (deckDescription.empty()) {
                    throw InvalidUserInputError("Need a deck to mutate.");
                }

                configuration.verbosity = static_cast<int>(vm.count("verbose"));

                RunCommand::Ptr command = RunCommand::Ptr(
                    new RunCommand(configuration)
                );

                Core::DeckTemplate::ConstPtr deck = TyrantCache::CLI::parseDeck(deckDescription);
                if (Core::StaticDeckTemplate::ConstPtr sDeck = std::dynamic_pointer_cast<Core::StaticDeckTemplate const>(deck)) {
                    command->task.initialPopulation.insert(sDeck);
                    command->task.simulationTask.defender
                        = TyrantCache::CLI::parseDeck("RAIDID:19");
                    command->task.simulationTask.surge = false;
                    command->task.byPoints = true;
                } else {
                    throw InvalidUserInputError("Can only work with static decks.");
                }


                return command;
             } catch (boost::program_options::required_option &e) {
                 std::stringstream ssMessage;
                 ssMessage << "Error parsing the arguments:" << std::endl;
                 ssMessage << e.what() << std::endl;
                 throw InvalidUserInputError(ssMessage.str());
             }
        }

    }
}
