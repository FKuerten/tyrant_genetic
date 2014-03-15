#include "deckParser.h++"

#include <list>
#include <core/autoDeckTemplate.h++>
#include <core/simpleOrderedDeckTemplate.h++>
#include <errorHandling/assert.h++>
#include <core/missionIdDeckTemplate.h++>
#include <core/raidDeckTemplate.h++>
#include <core/questDeckTemplate.h++>
#include <core/multiDeckTemplate.h++>

#include <iostream>

namespace C = Tyrant::Core;
namespace TyrantCache {
    namespace CLI {

        C::DeckTemplate::Ptr
        parseIdDeckFromStream
            (std::istream & is
            ,bool ordered = false
            )
        {
            // we expect ids (numbers) seperated by commata
            std::list<unsigned int> ids;
            while (true) {
                unsigned int id;
                is >> id;
                ids.push_back(id);

                if(is.eof()) {
                    // we are done
                    break;
                } else {
                    int chr = is.peek();
                    if (chr == ',') {
                        is.get();
                    } else {
                        // we are done
                        break;
                    }
                }
            } // while
            if (!ordered) {
                return C::DeckTemplate::Ptr( new C::AutoDeckTemplate(ids) );
            } else {
                return C::DeckTemplate::Ptr( new C::SimpleOrderedDeckTemplate(ids) );
            }
        }

        unsigned short base64CharToNumber(char const base64Char) {
            if (base64Char >= 'A' && base64Char <= 'Z') {
                return static_cast<unsigned short>(base64Char - 'A');
            } else if (base64Char >= 'a' && base64Char <= 'z') {
                return static_cast<unsigned short>(base64Char - 'a' + 26);
            } else if (base64Char >= '0' && base64Char <= '9') {
                return static_cast<unsigned short>(base64Char - '0' + 52);
            } else if (base64Char == '+') {
                return 62u;
            } else {
                assertEQ(base64Char, '/');
                return 63u;
            }
        }

        // Still old NETRAT comment...
        unsigned int base64ToId(unsigned short const base64)
        {
            // same stuff as with ID2BASE64, hi and lo swapped
            char const high = static_cast<char>((base64 >> 8) && 0xFF);
            char const low = static_cast<char>(base64 & 0xFF);
            return base64CharToNumber(low) + base64CharToNumber(high) * 64u;
        }

        unsigned long const CARD_MAX_ID = 5000u;

        std::list<unsigned int>
        base64RLEMinusEncodingToIdList
            (std::string const & hash
            )
        {
            //std::clog << "decoding freaky base64 stuff: " << hash << std::endl;
            std::list<unsigned int> list;
            size_t len = hash.size();

            unsigned int lastid;
            for (unsigned int i = 0; i < len; i+=2) {
                //std::clog << "current character is '" << hash[i] << "'" << std::endl;
                if (hash[i] == '.') break; // delimeter
                if (isspace(hash[i])) {
                    assertX(false);
                }
                unsigned int tid = 0;
                bool isCardOver4000 = false;
                if(hash[i] == '-') {
                    i++;
                    isCardOver4000 = true;
                    tid = 4000;
                }
                assertX(i + 1u < len); // make sure we have a full hash
                //std::clog << "reading characters '" << hash[i] << hash[i+1] << "' ";
                unsigned short cardHash = static_cast<unsigned short>((hash[i] << 8) + hash[i + 1]);
                tid += base64ToId(cardHash);
                //std::clog << "tid is " << tid << std::endl;
                if (i==0) {
                    // first card is commander
                    assertX(tid < CARD_MAX_ID);
                    assertX((tid >= 1000) && (tid < 2000)); // commander Id boundaries
                    //std::clog << "adding commander " << tid << std::endl;
                    list.push_back(tid);
                } else {
                    // later cards are not commander
                    assertX(i>0 || (tid < CARD_MAX_ID)); // commander card can't be encoded with RLE

                    if (tid < 4000 || isCardOver4000) {
                        // this is a (non commander) card
                        list.push_back(tid);
                        //std::clog << "adding card " << tid << " the first time " << std::endl;
                        lastid = tid;
                    } else {
                        // this is a RLE
                        for (unsigned int k = 4000+1; k < tid; k++) {
                            // decode RLE, +1 because we already added one card
                            list.push_back(lastid);
                            //std::clog << "adding card " << lastid << " again " << std::endl;
                        } // for RLE
                    }
                }
            } // for

            //for(std::list<unsigned int>::const_iterator i = list.begin(); i != list.end(); i++) {
            //    std::clog << *i << " ";
            //}
            //std::clog << std::endl;
            return list;
        }

        C::DeckTemplate::Ptr
        parseStrangeBase64RLEMinusEncodingFromStream
            (std::istream & is
            ,bool ordered = false
            )
        {
            std::stringstream ssBase64;
            bool done = false;
            while(!done) {
                int chr = is.peek();
                switch(chr) {
                    case -1:
                    case ';':
                    case '=':
                    case '}':
                        done = true;
                        break;
                    default:
                        is.get();
                        //std::clog << "copying character " << static_cast<char>(chr) << " " << chr << std::endl;
                        ssBase64 << static_cast<char>(chr);
                }
            }
            //std::clog << "copy done" << std::endl;
            std::list<unsigned int> ids = base64RLEMinusEncodingToIdList(ssBase64.str());
            if (!ordered) {
                return C::DeckTemplate::Ptr( new C::AutoDeckTemplate(ids) );
            } else {
                return C::DeckTemplate::Ptr( new C::SimpleOrderedDeckTemplate(ids) );
            }
        }

        C::DeckTemplate::Ptr
        parseMultiDeckFromStream
            (std::istream & is
            )
        {
            // format?
            // MULTI:{deck1}=2;deck2
            // i.e,
            // a deck description, optionally enclosed in {}
            // followed by either = or ;
            // after = follows a number, then ;
            // the last ; is optional
            std::multiset<C::DeckTemplate::Ptr> decks;
            //std::string::const_iterator iter = description:begin();
            // we expect a deck description
            while (true) {
                C::DeckTemplate::Ptr deck = parseDeckFromStream(is);
                //std::clog << "Got one deck: " << deck->toString() << std::endl;
                // now we may have eof, '}', ';' or '='
                if (is.eof() || is.peek() == std::char_traits<char>::eof()) {
                    //std::clog << "at eof" << std::endl;
                    decks.insert(deck);
                    return C::DeckTemplate::Ptr(new C::MultiDeckTemplate(decks));
                } else if (is.peek() == '}') {
                    //std::clog << "at '}'" << std::endl;
                    decks.insert(deck);
                    return C::DeckTemplate::Ptr(new C::MultiDeckTemplate(decks));
                } else if (is.peek() == ';') {
                    //std::clog << "at ';'" << std::endl;
                    is.get();
                    decks.insert(deck);
                } else if (is.peek() == '=') {
                    //std::clog << "at '='" << std::endl;
                    is.get();
                    unsigned int count;
                    is >> count;
                    for(size_t i = 0;i < count; i++) {
                        decks.insert(deck);
                    }
                    // eof or ';'
                    if (is.eof()) {
                        return C::DeckTemplate::Ptr(new C::MultiDeckTemplate(decks));
                    } else if (is.peek() == '}') {
                        return C::DeckTemplate::Ptr(new C::MultiDeckTemplate(decks));
                    } else if (is.peek() == ';') {
                        is.get();
                    } else {
                        throw InvalidUserInputError("expected EOF or ';'");
                    }
                } else {
                    throw InvalidUserInputError("expected EOF, ';' or '='");
                }
            }
        }


        C::DeckTemplate::Ptr
        switchOnIdentifier
            (std::string const & identifier
            ,std::istream & is
            )
        {
            if (identifier.compare("IDS") == 0) {
                // IDS, thats good
                return parseIdDeckFromStream(is);
            } else if (identifier.compare("ORDERED_IDS") == 0
                    || identifier.compare("IDS_ORDERED") == 0 ) {
                // ORDERED_IDS, thats ok
                return parseIdDeckFromStream(is, true);
            } else if (identifier.compare("BASE64RLEMINUS") == 0) {
                // freaky base64 encoding
                return parseStrangeBase64RLEMinusEncodingFromStream(is);
            } else if (identifier.compare("ORDERED_BASE64RLEMINUS") == 0
                    || identifier.compare("BASE64RLEMINUS_ORDERED") == 0) {
                // freaky base64 encoding
                return parseStrangeBase64RLEMinusEncodingFromStream(is, true);
            } else if (identifier.compare("MISSIONID") == 0) {
                unsigned int missionId;
                is >> missionId;
                return C::DeckTemplate::Ptr(new C::MissionIdDeckTemplate(missionId));
            } else if (identifier.compare("RAIDID") == 0) {
                unsigned int raidId;
                is >> raidId;
                return C::DeckTemplate::Ptr(new C::RaidDeckTemplate(raidId));
            } else if (identifier.compare("QUESTID") == 0) {
                unsigned int questId;
                is >> questId;
                return C::DeckTemplate::Ptr(new C::QuestDeckTemplate(questId));
            } else if (identifier.compare("MULTI") == 0) {
                return parseMultiDeckFromStream(is);
            } else {
                std::stringstream ssMessage;
                ssMessage << "Identifier '" << identifier << "' not supported." << std::endl;
                ssMessage << "Try one of the following:" << std::endl;
                ssMessage << "\t'IDS', 'ORDERED_IDS', 'IDS_ORDERED' for id based input" << std::endl;
                ssMessage << "\t'BASE64RLEMINUS', 'ORDERED_BASE64RLEMINUS', 'BASE64RLEMINUS_ORDERED', for some strange base64 encoding with an extra minus" << std::endl;
                ssMessage << "\t'MISSIONID', for missions, use the id, not the name" << std::endl;
                ssMessage << "\t'RAIDID', for raids, use the id, not the name" << std::endl;
                ssMessage << "\t'QUESTID', for quest steps, use the id, not the name" << std::endl;
                ssMessage << "\t'MULTI', for multiple decks" << std::endl;
                throw InvalidUserInputError(ssMessage.str());
            }
        }

        C::DeckTemplate::Ptr
        parseDeckFromStream(std::istream & is)
        {
            std::ios::iostate oldExceptionState = is.exceptions();
            is.exceptions(std::ios::badbit);
            if (is.peek() == '{') {
                is.get();
                C::DeckTemplate::Ptr deck = parseDeckFromStream(is);
                if(is.peek() == '}') {
                    is.get();
                    return deck;
                } else {
                    throw InvalidUserInputError("Expected '}'");
                }
            } else {
                // read until ':'
                std::stringstream ssWord;
                /*while(true) {
                    if(is.eof() {
                        throw InvalidUserInputError("Got eof, but expected data");
                    }
                    char chr;
                    is.get(chr);
                    if (chr == ':') {
                        // done
                        break;
                    }
                    ssWord << chr;
                }*/
                is.get(*(ssWord.rdbuf()), ':');
                is.get(); // discard the ':'
                std::string const word = ssWord.str();
                if (!is.good()) {
                    throw InvalidUserInputError("Every deck description must begin with an identifier followed by ':'");
                }
                // word contains an identifier, the stream is after the ':'
                return switchOnIdentifier(word, is);
            }
            is.exceptions(oldExceptionState);
        }

        C::DeckTemplate::Ptr
        parseDeck(std::string deckDescription)
        {
            std::stringstream stream(deckDescription);
            return parseDeckFromStream(stream);
        }

    }
}
