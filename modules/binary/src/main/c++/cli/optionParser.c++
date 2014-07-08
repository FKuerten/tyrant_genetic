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
                std::string attacker, defender;
                bool attackerFromStdIn;
                bool const defenderFromStdIn = false;
                bool useRaidRules;
                bool byPoints;

                po::options_description desc("Allowed options");
                desc.add_options()
                    ("help,h", "produce help message")
                    ("version,V", "version information")
                    ("core-version", "version of the core")
                    ("verbose,v", "increase verbosity")
                    ("attacker"
                    ,po::value<std::string>(&attacker)->default_value(std::string())
                    ,"the attacker"
                    )
                    ("attacker-from-stdin"
                    ,po::bool_switch(&attackerFromStdIn)->default_value(false)
                    ,"read attacker decks from stdin"
                    )
                    ("defender"
                    ,po::value<std::string>(&defender)->default_value(std::string())
                    ,"the defender"
                    )
                    //("defender-from-stdin"
                    //,po::bool_switch(&defenderFromStdIn)->default_value(false)
                    //,"read defender decks from stdin"
                    //)
                    ("raid-rules"
                    ,po::bool_switch(&useRaidRules)->default_value(false)
                    ,"force use of raid rules (default: raid rules will be used if one deck is a raid deck)"
                    )
                    ("by-points"
                    ,po::bool_switch(&byPoints)->default_value(false)
                    ,"compare by averade points/damage"
                    )
                ;

                po::variables_map vm;
                po::store(po::parse_command_line(argc, argv, desc), vm);

                if (vm.count("help")) {
                    return Command::Ptr(new HelpCommand(configuration, desc));
                } else if (vm.count("core-version")) {
                    return Command::Ptr(new CoreVersionCommand(configuration));
                } else if (vm.count("version")) {
                    return Command::Ptr(new VersionCommand(configuration));
                }

                po::notify(vm);

                // check some argument relation
                // check some argument relation
                if (attacker.empty() && !attackerFromStdIn) {
                    throw InvalidUserInputError("Need an attacker.");
                } else if (defender.empty() && !defenderFromStdIn) {
                    throw InvalidUserInputError("Need a defender.");
                } else if (!attacker.empty() && attackerFromStdIn) {
                    throw InvalidUserInputError("Can not supply both --attacker and --attacker-from-stdin.");
                } else if (!defender.empty() && defenderFromStdIn) {
                    throw InvalidUserInputError("Can not supply both --defender and --defender-from-stdin.");
                }

                configuration.verbosity = static_cast<int>(vm.count("verbose"));

                RunCommand::Ptr command = RunCommand::Ptr(
                    new RunCommand(configuration, attackerFromStdIn)
                );

                if (!attacker.empty()) {
                    Core::DeckTemplate::ConstPtr attackerDeck = TyrantCache::CLI::parseDeck(attacker);
                    if (Core::StaticDeckTemplate::ConstPtr sDeck = std::dynamic_pointer_cast<Core::StaticDeckTemplate const>(attackerDeck)) {
                        command->task.initialPopulation.insert(sDeck);
                    } else {
                        throw InvalidUserInputError("Can only work with static decks.");
                    }
                }
                if (!defender.empty()) {
                    command->task.simulationTask.defender
                        = TyrantCache::CLI::parseDeck(defender);
                }
                if (useRaidRules) {
                    command->task.simulationTask.useRaidRules = Core::tristate::TRUE;
                }
                command->task.simulationTask.surge = false;
                command->task.byPoints = byPoints;
		command->task.simulationTask.minimalNumberOfGames = 10000;

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
