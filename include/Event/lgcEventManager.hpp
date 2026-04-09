/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 * If a copy of the MPL was not distributed with this file, You can obtain one at 
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#pragma once
#include "defCoreApi.hpp"

#include <any>
#include <atomic>
#include <functional>
#include <iostream>
#include <shared_mutex>
#include <string>
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "Log/lgcLogManager.hpp"
#include "Task/lgcTaskManager.hpp"

namespace core
{
	class CORE_API EventManager
	{
	public:
		static EventManager& get();

		EventManager(const EventManager&) = delete;
		EventManager& operator=(const EventManager&) = delete;

		bool init();
		void shutdown();

		template <typename EventType>
		size_t subscribe(const std::string& topic, std::function<void(const EventType&)> callback)
		{
			std::unique_lock<std::shared_mutex> lock(mMutex);

			std::type_index typeIdx = std::type_index(typeid(EventType));
			size_t id = ++mNextId;

			auto& typeMap = mSubscribers[topic];

			if (typeMap.find(typeIdx) == typeMap.end())
				typeMap[typeIdx] = std::unordered_map<size_t, std::function<void(const EventType&)>>();

			auto& handlers = std::any_cast<std::unordered_map<size_t, std::function<void(const EventType&)>>&>(typeMap[typeIdx]);
			handlers[id] = std::move(callback);

			mIdLookup[id] = { topic, typeIdx };

			return id;
		}

		template <typename EventType>
		size_t subscribe(std::function<void(const EventType&)> callback)
		{
			return subscribe<EventType>("", std::move(callback));
		}

		template <typename EventType>
		void publish(const std::string& topic, const EventType& event, const size_t& excludedId = 0)
		{
			static_assert(std::is_copy_constructible_v<EventType>, "Async publish requires copy-constructible event type");

			std::vector<std::function<void(const EventType&)>> callbacks;
			{
				std::shared_lock<std::shared_mutex> lock(mMutex);

				auto topicIt = mSubscribers.find(topic);
				if (topicIt == mSubscribers.end()) return;

				std::type_index typeIdx = std::type_index(typeid(EventType));
				const auto& typeMap = topicIt->second;

				auto typeIt = typeMap.find(typeIdx);
				if (typeIt == typeMap.end()) return;

				try
				{
					const auto& handlers = std::any_cast<const std::unordered_map<size_t, std::function<void(const EventType&)>>&>(typeIt->second);
					callbacks.reserve(handlers.size());
					for (const auto& [id, callback] : handlers)
					{
						if (id == excludedId) continue;
						if (callback) callbacks.push_back(callback);
					}
				}
				catch (const std::bad_any_cast&)
				{
					APP_LOG_ERROR("[Event Manager]: Type mismatch when publishing event of type {} on topic '{}'", typeid(EventType).name(), topic);
					return;
				}
			}

			if (callbacks.empty()) return;

			EventType copiedEvent = event;
			try
			{
				TaskManager::get().add(Priority::NORMAL, [callbacks = std::move(callbacks), copiedEvent = std::move(copiedEvent)]() mutable
				{
					for (auto& cb : callbacks)
					{
						if (!cb) continue;
						try
						{
							cb(copiedEvent);
						}
						catch (const std::exception& ex)
						{
							APP_LOG_ERROR("[Event Manager]: Exception in async callback: {}", ex.what());
						}
						catch (...)
						{
							APP_LOG_ERROR("[Event Manager]: Unknown exception in async callback");
						}
					}
				});
			}
			catch (const std::exception& ex)
			{
				APP_LOG_ERROR("[Event Manager]: Failed to enqueue publish task: {}", ex.what());
			}
		}

		template <typename EventType>
		void publish(const EventType& event, const size_t& excludedId = 0)
		{
			publish<EventType>("", event, excludedId);
		}

		template <typename EventType>
		void unsubscribe(size_t id)
		{
			std::unique_lock<std::shared_mutex> lock(mMutex);

			auto lookupIt = mIdLookup.find(id);
			if (lookupIt == mIdLookup.end()) return;

			const std::string topic = lookupIt->second.topic;
			const std::type_index expectedType = lookupIt->second.type;
			const std::type_index requestType = std::type_index(typeid(EventType));

			// 订阅 id 与事件类型是一一对应的；类型不一致时拒绝删除，
			// 避免把 mIdLookup 先删掉后留下孤儿 handler，导致后续无法清理。
			if (expectedType != requestType)
			{
				APP_LOG_ERROR("[Event Manager]: Type mismatch when unsubscribing id {}. expected='{}', request='{}'", id, expectedType.name(), requestType.name());
				return;
			}

			auto topicIt = mSubscribers.find(topic);
			if (topicIt == mSubscribers.end())
			{
				// 主题桶已经不存在，直接清理索引即可，防止 idLookup 残留。
				mIdLookup.erase(lookupIt);
				return;
			}

			auto& typeMap = topicIt->second;
			auto typeIt = typeMap.find(requestType);
			if (typeIt == typeMap.end())
			{
				// 类型桶已不存在，清理索引并返回。
				mIdLookup.erase(lookupIt);
				if (typeMap.empty())
					mSubscribers.erase(topicIt);
				return;
			}

			try
			{
				auto& handlers = std::any_cast<std::unordered_map<size_t, std::function<void(const EventType&)>>&>(typeIt->second);
				handlers.erase(id);

				// 分层清理，确保不会留下空桶，避免单例析构时出现异常状态。
				if (handlers.empty())
					typeMap.erase(typeIt);
				if (typeMap.empty())
					mSubscribers.erase(topicIt);
			}
			catch (const std::bad_any_cast&)
			{
				APP_LOG_ERROR("[Event Manager]: Type mismatch when unsubscribing event of type {} on topic '{}'", typeid(EventType).name(), topic);
				return;
			}

			mIdLookup.erase(lookupIt);
		}

	private:
		EventManager();
		~EventManager();

	private:
		struct HandlerInfo
		{
			std::string topic;
			std::type_index type;

			// 默认构造函数
			HandlerInfo() : type(typeid(void)) {}  // 用 void 作为占位符

			// 带参数的构造函数（方便使用）
			HandlerInfo(const std::string& t, const std::type_index& ti)
				: topic(t), type(ti) {}
		};

		mutable std::shared_mutex mMutex;
		std::unordered_map<std::string, std::unordered_map<std::type_index, std::any>> mSubscribers;
		std::unordered_map<size_t, HandlerInfo> mIdLookup;
		std::atomic<size_t> mNextId;

	};
}
