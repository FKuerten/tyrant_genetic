#ifndef TYRANT_CACHE_CLI_DECKPARSER_HPP
    #define TYRANT_CACHE_CLI_DECKPARSER_HPP

    #include <core/deckTemplate.h++>
    #include <istream>
    #include <string>

    namespace TyrantCache {
        namespace CLI {

            ::Tyrant::Core::DeckTemplate::Ptr
            parseDeckFromStream(std::istream & is);

            ::Tyrant::Core::DeckTemplate::Ptr
            parseDeck(std::string deckDescription);

        }
    }
#endif

