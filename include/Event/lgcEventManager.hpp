#pragma once
#include "defCoreApi.hpp"

#include <functional>
#include <unordered_map>
#include <typeindex>
#include <any>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <string>
#include <iostream>

namespace core
{
    class CORE_API EventManager
    {
    public:
        static EventManager& get();

        EventManager(const EventManager &) = delete;
        EventManager &operator=(const EventManager &) = delete;

        template <typename EventType>
        size_t subscribe(const std::string &topic, std::function<void(const EventType &)> callback)
        {
            std::unique_lock<std::shared_mutex> lock(mMutex);

            std::type_index typeIdx = std::type_index(typeid(EventType));
            size_t id = ++mNextId;

            auto &typeMap = mSubscribers[topic];

            if (typeMap.find(typeIdx) == typeMap.end())
                typeMap[typeIdx] = std::unordered_map<size_t, std::function<void(const EventType &)>>();

            auto &handlers = std::any_cast<std::unordered_map<size_t, std::function<void(const EventType &)>> &>(typeMap[typeIdx]);
            handlers[id] = std::move(callback);

            mIdLookup[id] = {topic, typeIdx};

            return id;
        }

        template <typename EventType>
        size_t subscribe(std::function<void(const EventType &)> callback)
        {
            return subscribe<EventType>("", std::move(callback));
        }

        template <typename EventType>
        void publish(const std::string &topic, const EventType &event)
        {
            std::shared_lock<std::shared_mutex> lock(mMutex);

            auto topicIt = mSubscribers.find(topic);
            if (topicIt == mSubscribers.end())
                return;

            std::type_index typeIdx = std::type_index(typeid(EventType));
            const auto &typeMap = topicIt->second;

            auto typeIt = typeMap.find(typeIdx);
            if (typeIt == typeMap.end())
                return;

            try
            {
                const auto &handlers = std::any_cast<const std::unordered_map<size_t, std::function<void(const EventType &)>> &>(typeIt->second);
                for (const auto &[id, callback] : handlers)
                    if (callback)
                        callback(event);
            }
            catch (const std::bad_any_cast &)
            {
            }
        }

        template <typename EventType>
        void publish(const EventType &event)
        {
            publish<EventType>("", event);
        }

        template <typename EventType>
        void unsubscribe(size_t id)
        {
            std::unique_lock<std::shared_mutex> lock(mMutex);

            auto lookupIt = mIdLookup.find(id);
            if (lookupIt == mIdLookup.end()) return;

            std::string topic = lookupIt->second.topic;

            auto topicIt = mSubscribers.find(topic);
            if (topicIt != mSubscribers.end())
            {
                auto &typeMap = topicIt->second;
                std::type_index typeIdx = std::type_index(typeid(EventType));
                auto typeIt = typeMap.find(typeIdx);
                if (typeIt != typeMap.end())
                {
                    auto &handlers = std::any_cast<std::unordered_map<size_t, std::function<void(const EventType &)>> &>(typeIt->second);
                    handlers.erase(id);
                }
            }

            mIdLookup.erase(id);
        }

    private:
        EventManager();
        ~EventManager();

        struct HandlerInfo
        {
            std::string topic;
            std::type_index type;
        };

        std::unordered_map<std::string, std::unordered_map<std::type_index, std::any>> mSubscribers;

        std::unordered_map<size_t, HandlerInfo> mIdLookup;

        std::atomic<size_t> mNextId;
        mutable std::shared_mutex mMutex;
    };
}
