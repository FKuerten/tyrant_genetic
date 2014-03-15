#include "tyrantOptimizerBinding.h++"
#include <errorHandling/assert.h++>
#include <pstreams/pstream.h>
#include <sstream>
#include <iostream>
#include <string>
#include <tuple>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <core/autoDeckTemplate.h++>
#include <core/simpleOrderedDeckTemplate.h++>
#include <core/missionIdDeckTemplate.h++>
#include <core/raidDeckTemplate.h++>
#include <core/questDeckTemplate.h++>

std::string const executable = "./tyrant_optimize";

namespace Core = Tyrant::Core;
namespace TyrantCache {
    namespace TO {

        std::string
        TyrantOptimizerCLI::getCoreVersion() const
        {
            redi::ipstream in(executable + " -version");
            std::stringstream ss;
            std::string str;
            while(std::getline(in, str)) {
                ss << str << std::endl;
            }
            std::string result = ss.str();
            boost::algorithm::trim(result);
            return result;
        }

        std::string
        TyrantOptimizerCLI::getCoreVersionHumanReadable() const
        {
            return this->getCoreVersion();
        }

        std::string
        TyrantOptimizerCLI::getCoreName() const
        {
            return executable;
        }

        class TODeckVisitor : public Core::AutoDeckTemplate::Visitor
                            , public Core::SimpleOrderedDeckTemplate::Visitor
                            , public Core::MissionIdDeckTemplate::Visitor
                            , public Core::RaidDeckTemplate::Visitor
                            , public Core::QuestDeckTemplate::Visitor
                            , public Praetorian::Basics::Visitor::AcyclicVisitor
        {
            private:
                std::string result;
                bool ordered = false;
                bool raid = false;

                char encodeBase64(unsigned int x)
                {
                    if (x < 26) {
                        return x + 'A';
                    } else if (x < 52) {
                        return x - 26 + 'a';
                    } else if (x < 62) {
                        return x - 52 + '0';
                    } else if ( x == 62) {
                        return '+';
                    } else if ( x == 63) {
                        return '/';
                    } else {
                        assertX(false);
                        return '\0';
                    }
                }

                std::string idToBase64Minus(unsigned int cardId)
                {
                    std::stringstream ssBase64Minus;
                    if (cardId > 4000) {
                        ssBase64Minus << '-';
                        cardId-=4000;
                    }
                    unsigned int high = (cardId >> 6) & 077;
                    unsigned int low = cardId & 077;
                    ssBase64Minus << encodeBase64(high);
                    ssBase64Minus << encodeBase64(low);
                    return ssBase64Minus.str();
                }

                template <class T>
                std::string idsToBase64Minus(unsigned int commanderId
                                       ,T start
                                       ,T end)
                {
                    std::stringstream ssBase64Minus;
                    ssBase64Minus << idToBase64Minus(commanderId);
                    for(T iterator = start; iterator != end; iterator++) {
                        unsigned int const cardId = *iterator;
                        std::string cardHash = idToBase64Minus(cardId);
                        ssBase64Minus << cardHash;
                    }
                    return ssBase64Minus.str();
                }

                virtual void visit(Core::AutoDeckTemplate const & autoDeckTemplate) override
                {
                    this->result = idsToBase64Minus(autoDeckTemplate.commanderId, autoDeckTemplate.cards.begin(), autoDeckTemplate.cards.end());
                }
                virtual void visit(Core::SimpleOrderedDeckTemplate const & simpleOrderedDeckTemplate) override
                {
                    this->ordered = true;
                    this->result = idsToBase64Minus(simpleOrderedDeckTemplate.commanderId, simpleOrderedDeckTemplate.cards.begin(), simpleOrderedDeckTemplate.cards.end());
                }
                virtual void visit(Core::MissionIdDeckTemplate const & missionIdDeckTemplate) override
                {
                    std::stringstream ssResult;
                    ssResult << "Mission #";
                    ssResult << missionIdDeckTemplate.missionId;
                    this->result = ssResult.str();
                }
                virtual void visit(Core::RaidDeckTemplate const & raidDeckTemplate) override
                {
                    this->raid = true;
                    std::stringstream ssResult;
                    ssResult << "Raid #";
                    ssResult << raidDeckTemplate.raidId;
                    this->result = ssResult.str();
                }
                virtual void visit(Core::QuestDeckTemplate const & questDeckTemplate) override
                {
                    std::stringstream ssResult;
                    ssResult << "Quest #";
                    ssResult << questDeckTemplate.questId;
                    this->result = ssResult.str();
                }

            public:
                std::string getResult() const { return this->result; }
                bool isOrdered() const { return this->ordered; }
                bool isRaid() const { return this->raid; }
        };

        std::tuple<std::string, bool, bool>
        deckTemplateToTOArgument(Core::DeckTemplate::ConstPtr deckTemplate)
        {
            TODeckVisitor visitor;
            deckTemplate->accept(visitor);
            std::string deckDescription = visitor.getResult();
            bool isOrdered = visitor.isOrdered();
            bool isRaid = visitor.isRaid();
            return std::make_tuple(deckDescription, isOrdered, isRaid);
        }

        Core::SimulationResult
        TyrantOptimizerCLI::simulate(Core::SimulationTask const & task)
        {
            assertX(!this->theProgram);
            redi::ipstream theProgram;
            this->theProgram = &theProgram;

            // A Build the command:
            std::stringstream command;

            // A.0 We start with the executable.
            command << executable;

            // A.10 the decks
            // TODO need something more standardized than passing literal input values
            // TODO also order
            std::tuple<std::string, bool, bool> attackerInformation = deckTemplateToTOArgument(task.attacker);
            std::tuple<std::string, bool, bool> defenderInformation = deckTemplateToTOArgument(task.defender);
            command << " " << '"' << std::get<0>(attackerInformation) << '"';
            command << " " << '"' << std::get<0>(defenderInformation) << '"';

            // A.20 the flags
            if (task.achievement > 0) {
                command << " " << "-A" << " " << task.achievement;
            }

            // A.30 the operation
            command << " " << "sim";
            // A.31 the number of iterations
            command << " " << task.minimalNumberOfGames;
            // A.32 surge
            if (task.surge) {
                command << " " << "-s";
            }
            // A.33 ordered decks
            if (std::get<1>(attackerInformation)) {
                command << " " << "-r";
            }
            if (std::get<1>(defenderInformation)) {
                throw LogicError("Tyrant Optimize binding does not support ordered defense decks.");
            }
            bool isRaid = false;
            if (std::get<2>(attackerInformation)) {
                throw LogicError("Tyrant Optimize binding does not support raid attack decks.");
            }
            if (std::get<2>(defenderInformation)) {
                isRaid = true;
            }

            // A.34 delay first attacker
            if (task.delayFirstAttacker) {
                throw LogicError("Tyrant Optimize binding does not support delay first attacker.");
            }

            // A.36 raid rules
            if (task.useRaidRules != Core::tristate::UNDEFINED && !isRaid) {
                throw LogicError("Tyrant Optimize binding does not support force raid rules.");
            }

            // A.37 number of rounds
            if (task.numberOfRounds != -1) {
                command << " " << " -turnlimit " << task.numberOfRounds;
            }

            bool const DEBUG_TO = false;
            if (DEBUG_TO) {
                std::clog << "Command: " << std::endl;
                std::clog << command.str();
                std::clog << std::endl;
            }
            this->theProgram->open(command.str());

            if (DEBUG_TO) {
                std::clog << "Result: " << std::endl;
            }
            std::stringstream ssResult;
            std::string line;
            while(std::getline(*(this->theProgram), line)) {
                ssResult << line << std::endl;
                if (DEBUG_TO) {
                    std::clog << line << std::endl;
                }
            }
            this->theProgram->close();
            std::string sResult = ssResult.str();
            int exitCode = this->theProgram->rdbuf()->status();
            this->theProgram = nullptr;

            if (exitCode != 0) {
                std::stringstream ssMessage;
                ssMessage << "Simulation failed. Exit code " << exitCode << " (" << (exitCode % 256) << ")" << std::endl;
                ssMessage << "Command was " << command.str() << std::endl;
                ssMessage << "Result is: " << std::endl;
                ssMessage << sResult << std::endl;
                throw RuntimeError(ssMessage.str());
            }

            //std::clog << std::endl;

            // Parse the result
            Core::SimulationResult simulationResult;
            while(std::getline(ssResult, line)) {
                if (boost::starts_with(line, "win%: ")) {
                    boost::smatch match;
                    std::string sRegex = "win%: (.+) \\((.+) / (.+)\\)";
                    boost::regex regex{sRegex};
                    if (boost::regex_match(line, match, regex)) {
                        if (!isRaid) {
                            simulationResult.gamesWon = boost::lexical_cast<unsigned int>(match.str(2));
                        } else {
                            simulationResult.gamesStalled = boost::lexical_cast<unsigned int>(match.str(2));
                        }
                    }
                } else if (boost::starts_with(line, "stall%: ")) {
                    assertX(!isRaid);
                    boost::smatch match;
                    std::string sRegex = "stall%: (.+) \\((.+) / (.+)\\)";
                    boost::regex regex{sRegex};
                    if (boost::regex_match(line, match, regex)) {
                        simulationResult.gamesStalled = boost::lexical_cast<unsigned int>(match.str(2));
                    }
                } else if (boost::starts_with(line, "loss%: ")) {
                    boost::smatch match;
                    std::string sRegex = "loss%: (.+) \\((.+) / (.+)\\)";
                    boost::regex regex{sRegex};
                    if (boost::regex_match(line, match, regex)) {
                        simulationResult.gamesLost = boost::lexical_cast<unsigned int>(match.str(2));
                    }
                } else if (boost::starts_with(line, "slay%: ")) {
                    assertX(isRaid);
                    boost::smatch match;
                    std::string sRegex = "slay%: (.+) \\((.+) / (.+)\\)";
                    boost::regex regex{sRegex};
                    if (boost::regex_match(line, match, regex)) {
                        simulationResult.gamesWon = boost::lexical_cast<unsigned int>(match.str(2));
                    }
                } else if (boost::starts_with(line, "ard: ")) {
                    assertX(isRaid);
                    boost::smatch match;
                    std::string sRegex = "ard: (.+) \\((.+) / (.+)\\)";
                    boost::regex regex{sRegex};
                    if (boost::regex_match(line, match, regex)) {
                        simulationResult.pointsAttacker = boost::lexical_cast<unsigned int>(match.str(2));
                        simulationResult.pointsAttackerAuto = simulationResult.pointsAttacker;
                    }
                }
            }
            if (isRaid) {
                if (simulationResult.gamesStalled < simulationResult.gamesWon) {
                    std::clog << std::endl;
                    std::clog << "Commandline was: " << command.str() << std::endl;
                    std::clog << "Result is: " << std::endl;
                    std::clog << sResult << std::endl;
                    assertGE(simulationResult.gamesStalled, simulationResult.gamesWon);
                }
                simulationResult.gamesStalled -= simulationResult.gamesWon;
            }
            simulationResult.numberOfGames = simulationResult.gamesWon
                                           + simulationResult.gamesStalled
                                           + simulationResult.gamesLost;
            //std::clog << std::flush;
            return simulationResult;
        }

        void
        TyrantOptimizerCLI::abort()
        {
            if (this->theProgram) {
                this->theProgram->rdbuf()->kill(SIGINT);
            }
        }

    }
}
