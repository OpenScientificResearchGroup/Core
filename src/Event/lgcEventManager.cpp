#include "Event/lgcEventManager.hpp"

namespace core
{
    EventManager& EventManager::get()
    {
        static EventManager instance;
        return instance;
    }

    EventManager::EventManager() : mNextId(0) {}

    EventManager::~EventManager() {};
}
