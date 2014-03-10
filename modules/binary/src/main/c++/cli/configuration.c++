#include "configuration.h++"
#include <mutator/cardChangeMutator.h++>
#include "../to/tyrantOptimizerBinding.h++"
#include <logging/streamLogger.h++>
#include <logging/nullLogger.h++>
#include <iostream>
#include <cache/diskBackedCache.h++>
#include <cache/memoryBackedCache.h++>
#include <cache/multiDeckDecompositor.h++>

namespace TyrantGenetic {
    namespace CLI {

        Praetorian::Basics::Logging::Logger::Ptr
        Configuration::constructRootLogger() const
        {
            if(this->verbosity > 0) {
                return Praetorian::Basics::Logging::Logger::Ptr(
                    new Praetorian::Basics::Logging::StreamLogger(std::clog)
                );
            } else {
                return Praetorian::Basics::Logging::Logger::Ptr(
                    new Praetorian::Basics::Logging::NullLogger()
                );
            }
        }

        Tyrant::Core::SimulatorCore::Ptr
        Configuration::constructCore() const
        {
            Tyrant::Core::SimulatorCore::Ptr simulator
                = Tyrant::Core::SimulatorCore::Ptr(
                    new TyrantCache::TO::TyrantOptimizerCLI()
                );
            Tyrant::Cache::SimulatorCache::Ptr diskCache
                = Tyrant::Cache::SimulatorCache::Ptr(
                    new Tyrant::Cache::DiskBackedCache(simulator)
                );
            Tyrant::Cache::SimulatorCache::Ptr memoryCache
                = Tyrant::Cache::SimulatorCache::Ptr(
                    new Tyrant::Cache::MemoryBackedCache(diskCache)
                );
            Tyrant::Core::SimulatorCore::Ptr decompositor
                = Tyrant::Core::SimulatorCore::Ptr(
                    new Tyrant::Cache::MultiDeckDecompositor(memoryCache)
                );
            return decompositor;
        }


        Tyrant::Mutator::Mutator::Ptr
        Configuration::constructMutator() const
        {
            return Tyrant::Mutator::Mutator::Ptr(
                new Tyrant::Mutator::CardChangeMutator()
            );
        }

    }
}
