#include "commands.h++"
#include <version.h++>
#include <errorHandling/assert.h++>

namespace TyrantGenetic {
    namespace CLI {

        Command::Command(Configuration configuration)
        : aborted(false)
        , configuration(configuration)
        {}

        Command::~Command()
        {}

        void
        Command::abort()
        {
            this->aborted = true;
        }

        VersionCommand::VersionCommand(Configuration configuration)
        : Command(configuration)
        {
        }

        int VersionCommand::execute()
        {
            //human readable
            std::cout << "human readable version description:" << std::endl;
            std::cout << "\t" << tyrantGenetic_VERSION_DESCRIBE;
            if (tyrantGenetic_DIRTY_HEAD) {
                std::cout << "+";
                std::cout << tyrantGenetic_DIRTY_HASH;
            }
            std::cout << std::endl;

            // commit-ish
            std::cout << "tyrantMutator version hash:" << std::endl;
            std::cout << '\t' << tyrantGenetic_VERSION;
            if (tyrantGenetic_DIRTY_HEAD) {
                std::cout << "+";
                std::cout << tyrantGenetic_DIRTY_HASH;
            }
            std::cout << std::endl;

            // tags
            if (!tyrantGenetic_DIRTY_HEAD) {
                std::cout << "Tags pointing at this version: " << std::endl;
                std::cout << '\t' << tyrantGenetic_VERSION_TAGS << std::endl;
            }

            std::cout << "XML data version: "       << std::endl;
            Tyrant::Core::SimulatorCore::Ptr core = this->configuration.constructCore();
            std::map<std::string,std::string> xmlHashes = core->getXMLVersions();
            assertGT(xmlHashes.size(),0u);
            for(std::map<std::string,std::string>::const_iterator iter = xmlHashes.begin()
               ;iter != xmlHashes.end()
               ;iter++
               )
            {
                std::cout << '\t';
                std::cout << std::setw(20) << std::left << iter->first << " ";
                std::cout << iter->second << std::endl;
            }
            return 0;
        }

        CoreVersionCommand::CoreVersionCommand(Configuration configuration)
        : Command(configuration)
        {
        }

        int CoreVersionCommand::execute()
        {
            Tyrant::Core::SimulatorCore::Ptr core = this->configuration.constructCore();

            std::cout << core->getCoreVersionHumanReadable() << std::endl;
            return 0;
        }

        HelpCommand::HelpCommand(Configuration configuration, boost::program_options::options_description const & desc)
        : Command(configuration)
        , desc(desc)
        {
        }

        int HelpCommand::execute()
        {
            std::cout << this->desc << std::endl;
            return 0;
        }

    }
}
