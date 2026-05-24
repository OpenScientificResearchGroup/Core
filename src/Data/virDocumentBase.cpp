/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Core/Data/virDocumentBase.hpp"

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <queue>
#include <stack>

#include "Core/Log/lgcLogManager.hpp"
#include "Core/Event/lgcEventManager.hpp"
#include "Core/Utility/lgcString.hpp"

namespace core
{
	TransactionGuard::TransactionGuard(DocumentBase* doc)
		: mDoc(doc)
	{
		if (mDoc) mDoc->startTransaction();
	}

	TransactionGuard::~TransactionGuard()
	{
		if (mDoc) mDoc->endTransaction();
	}

	DocumentBase::DocumentBase()
		: NodeSetBase(), mPath(""), mCommandManager(50), mCnt(0), mTransactionCount(0), mIsLoading(false)
	{

	}

	ObjectType DocumentBase::getObjectType() const
	{
		return ObjectType::DOCUMENT;
	}

	bool DocumentBase::load(const std::string& path)
	{
		// 实现加载逻辑
		mPath = path;
		std::ifstream ifs(mPath);
		if (!ifs.is_open()) return false;
		std::stringstream buffer;
		buffer << ifs.rdbuf();
		nlohmann::json file = nlohmann::json::parse(buffer.str());
		mIsLoading = true; // 标记正在加载，避免触发事件
		if (!read(file)) { mIsLoading = false; return false; }
		mIsLoading = false;
		return true;
	}

	bool DocumentBase::save(const std::string& path)
	{
		
		if (!path.empty()) mPath = path;
		nlohmann::json file = write();
		std::ofstream ofs(mPath);
		if (!ofs.is_open()) return false;
		ofs << file.dump();
		return true; // 成功
	}

	const std::string& DocumentBase::getPath() const
	{
		return mPath;
	}

	NodeBase* DocumentBase::findNode(const std::string& uuid) const
	{
		auto it = mNodeIndex.find(uuid);
		if (it != mNodeIndex.end())
			return it->second;
		return nullptr;
	}

	std::string DocumentBase::generateUniqueName(const std::string& prefix)
	{
		return prefix + "_" + std::to_string(++mCnt);
	}

	core::CommandManager& DocumentBase::getCommandManager()
	{
		return mCommandManager;
	}

	void DocumentBase::onUpdate(ObjectBase* obj)
	{
		if (mIsLoading) return; // 加载时不触发更新事件
		mPendingSet.insert(obj);
		if (mTransactionCount == 0 && !mIsExecuting) run();// 如果不是在事务中且当前不在结算过程中，立即执行
	}

	void DocumentBase::onAttach(ObjectBase* obj)
	{
		if (obj->getObjectType() == ObjectType::NODE || obj->getObjectType() == ObjectType::NODE_SET)
		{
			auto* node = static_cast<NodeBase*>(obj);
			mNodeIndex[node->getUuid()] = node;
			for (const auto& prop : node->getAllProperties())
				if (prop->isLink()) // 如果该属性是链接状态
				{
					// 从属性元数据中提取源 UUID 和 源 Key
					std::string srcUuid = prop->getLink().uuid;
					std::string srcPath = prop->getLink().path;

					// 利用 DocumentBase 的全局索引查找源节点
					auto it = mNodeIndex.find(srcUuid);
					if (it != mNodeIndex.end())
					{
						NodeBase* srcNode = it->second;
						// 从源节点提取值（使用 any 类型擦除接口）
						PropertyBase* srcProp = srcNode->getProperty(util::string::split(srcPath, '/'));
						insertDagLink(prop, srcProp);
					}
					//else
					//{
					//	// 如果源节点找不到了（比如被删了），在此处处理失效逻辑
					//	prop->resetLink();
					//}
				}
		}
		if (mIsLoading) return; // 加载时不触发更新事件
		mPendingSet.insert(obj);
		if (mTransactionCount == 0 && !mIsExecuting) run();// 如果不是在事务中且当前不在结算过程中，立即执行
	}

	void DocumentBase::onDetach(ObjectBase* obj)
	{
		if (obj->getObjectType() == ObjectType::NODE || obj->getObjectType() == ObjectType::NODE_SET)
			if (auto* node = static_cast<NodeBase*>(obj))
				mNodeIndex.erase(node->getUuid());
		deleteDagNode(obj);
		if (mTransactionCount == 0 && !mIsExecuting) run();// 如果不是在事务中且当前不在结算过程中，立即执行
	}

	void DocumentBase::onLink(ObjectBase* obj)
	{
		if (mIsLoading) return; // 加载时不触发更新事件
		mPendingSet.insert(obj);

		ObjectBase* source = nullptr;
		if (obj->getObjectType() == ObjectType::PROPERTY)
			if (auto* prop = static_cast<PropertyBase*>(obj))
			{
				auto it = mNodeIndex.find(prop->getLink().uuid);
				if (it == mNodeIndex.end()) return; // 源节点不存在，可能已经被删除了
				NodeBase* srcNode = it->second;
				source = srcNode->getProperty(util::string::split(prop->getLink().path, '/'));
			}
		insertDagLink(source, obj);
		if (mTransactionCount == 0 && !mIsExecuting) run();// 如果不是在事务中且当前不在结算过程中，立即执行
	}

	void DocumentBase::onUnlink(ObjectBase* obj)
	{
		mPendingSet.insert(obj);

		ObjectBase* source = nullptr;
		if (obj->getObjectType() == ObjectType::PROPERTY)
			if (auto* prop = static_cast<PropertyBase*>(obj))
			{
				auto it = mNodeIndex.find(prop->getLink().uuid);
				if (it == mNodeIndex.end()) return; // 源节点不存在，可能已经被删除了
				NodeBase* srcNode = it->second;
				source = srcNode->getProperty(util::string::split(prop->getLink().path, '/'));
			}
		deleteDagLink(source, obj);
		if(mTransactionCount == 0 && !mIsExecuting) run();// 如果不是在事务中且当前不在结算过程中，立即执行
	}

	//void DocumentBase::update()
	//{
	//	// DocumentBase 不能被更新
	//}

	void DocumentBase::attach()
	{
		// DocumentBase 不能被附加
	}

	void DocumentBase::detach()
	{
		// DocumentBase 不能被移除
	}

	void DocumentBase::startTransaction()
	{
		mTransactionCount++;
	}

	void DocumentBase::endTransaction()
	{
		if (mTransactionCount <= 0) return;
		if (--mTransactionCount == 0/* && mPendingExecute*/)
			run();
	}

	//void DocumentBase::insertCommitCallback(std::function<void()> callback)
	//{
	//	mCommitCallbacks.push_back(callback);
	//}

	bool DocumentBase::commit()
	{
		if (mPendingSet.empty() || mIsExecuting) return true;
		mIsExecuting = true; // 开启守卫，拦截所有重入请求
		bool allSuccess = true;

		while (!mPendingSet.empty()/* && iterations < mMaxIterations*/)
		{
			// 1. 获取精准的受影响对象序列 (包含 Node 和 Property)
			std::vector<ObjectBase*> evalOrder = calculateEvaluationOrder(mPendingSet);

			// 2. 清理待办集
			mPendingSet.clear();

			// 3. 按照拓扑时序执行流水线
			for (ObjectBase* obj : evalOrder)
			{
				// 检查对象是否依然在文档中（防止事务中途被级联删除的对象残留在序列里）
				// 如果是属性，检查其所属节点是否存活
				if (obj->getObjectType() == ObjectType::NODE || obj->getObjectType() == ObjectType::NODE_SET || obj->getObjectType() == ObjectType::DOCUMENT)
				{
					auto* node = static_cast<NodeBase*>(obj);
					if (mNodeIndex.find(node->getUuid()) == mNodeIndex.end()) continue;
				}
				else if (obj->getObjectType() == ObjectType::PROPERTY)
				{
					auto* prop = static_cast<PropertyBase*>(obj);
					if (mNodeIndex.find(prop->getNode()->getUuid()) == mNodeIndex.end()) continue;
				}

				// 分类处理
				if (obj->getObjectType() == ObjectType::PROPERTY)
				{
					auto* prop = static_cast<PropertyBase*>(obj);
					if (prop->isLink())
					{
						// --- 精准同步 ---
						// 只同步这一个属性，而不是遍历 Node 的全部属性
						// 从属性元数据中提取源 UUID 和 源 Key
						std::string srcUuid = prop->getLink().uuid;
						std::string srcPath = prop->getLink().path;

						// 利用 DocumentBase 的全局索引查找源节点
						auto it = mNodeIndex.find(srcUuid);
						if (it != mNodeIndex.end())
						{
							NodeBase* srcNode = it->second;
							// 从源节点提取值（使用 any 类型擦除接口）
							const PropertyBase* srcProperty = srcNode->getProperty(util::string::split(srcPath, '/'));
							std::any val = srcProperty->getValueAny();
							// 将值同步给当前节点的属性
							prop->sync(val);
						}
						else
						{
							// 如果源节点找不到了（比如被删了），在此处处理失效逻辑
							prop->resetLink();
						}
					}
				}
				else if (obj->getObjectType() == ObjectType::NODE || obj->getObjectType() == ObjectType::NODE_SET || obj->getObjectType() == ObjectType::DOCUMENT)
				{
					auto* node = static_cast<NodeBase*>(obj);
					// --- 逻辑刷新 ---
					// 此时 node 依赖的所有上游属性已经通过上面的逻辑 sync 过了
					if (!node->execute())
						allSuccess = false;
				}
			}
		}
		mIsExecuting = false;
		//for (const auto& callback : mCommitCallbacks)
		//	callback();

		return allSuccess;
	}

	bool DocumentBase::insertDagNode(ObjectBase* obj)
	{
		// 1. 基础检查
		if (!obj) return false;

		// 2. 检查是否已经存在该节点，避免重复插入
		// 确保入口存在，使用 emplace 避免覆盖已有数据
		mOutGraph.emplace(obj, std::vector<ObjectBase*>());
		mInGraph.emplace(obj, std::vector<ObjectBase*>());

		return true;
	}

	// 建立依赖：source 被 sink 依赖 (source -> sink)
	bool DocumentBase::insertDagLink(ObjectBase* source, ObjectBase* sink)
	{
		// 1. 基础合法性检查
		if (!source || !sink || source == sink) return false;

		// 2. 环路检测 (Cycle Detection)
		// 在建立 A -> B 之前，先检查 B 是否已经直接或间接依赖了 A
		if (isCycle(source, sink))
			// 抛出异常或返回 false，阻止非法连接
			return false;

		// 3. 更新出边表 (Source -> Sink)
		auto& outRef = mOutGraph[source]; // 如果不存在会自动创建
		if (std::find(outRef.begin(), outRef.end(), sink) == outRef.end())
			outRef.push_back(sink);

		// 4. 更新入边表 (Sink -> Source)
		// 这就是为了让你在删除 Sink 时，能瞬间找到谁是它的“源”
		auto& inRef = mInGraph[sink];
		if (std::find(inRef.begin(), inRef.end(), source) == inRef.end())
			inRef.push_back(source);

		return true;
	}

	void DocumentBase::deleteDagNode(ObjectBase* obj)
	{
		if (!obj) return;

		// --- 阶段 1: 识别所有需要被级联删除的对象 ---
		std::vector<ObjectBase*> targets;
		std::unordered_set<ObjectBase*> visited;
		std::queue<ObjectBase*> q;

		q.push(obj);
		visited.insert(obj);

		while (!q.empty())
		{
			ObjectBase* curr = q.front();
			q.pop();
			targets.push_back(curr);

			if (mOutGraph.count(curr))
			{
				for (ObjectBase* sink : mOutGraph[curr])
				{
					if (visited.find(sink) == visited.end())
					{
						visited.insert(sink);
						q.push(sink);
					}
				}
			}
		}

		// --- 阶段 2: 逻辑与内存清理 ---
		// 按照发现顺序的逆序或正序清理（这里建议正序，先清理源头）
		for (ObjectBase* current : targets)
		{
			// 1. 从 UUID 索引移除
			if (current->getObjectType() == ObjectType::NODE)
			{
				auto* node = static_cast<NodeBase*>(current);
				mNodeIndex.erase(node->getUuid());

				// 【核心改进】：处理所有权
				// 如果当前正在事务中，且不是由 Command 发起的删除，
				// 你可能需要在这里 extractChild 并处理生命周期。
				// 如果这是由 DeleteCommand 发起的，所有权由 Command 处理。
			}

			// 2. 清理该节点的所有连线
			// 处理入边：告诉我的 Source 们，以后别给我发消息了
			if (mInGraph.count(current))
			{
				for (ObjectBase* source : mInGraph[current])
				{
					auto it = mOutGraph.find(source);
					if (it != mOutGraph.end())
					{
						auto& sinks = it->second;
						sinks.erase(std::remove(sinks.begin(), sinks.end(), current), sinks.end());
					}
				}
				mInGraph.erase(current);
			}

			// 处理出边：直接删除（因为下游已经在 targets 列表里等着被删了）
			mOutGraph.erase(current);

			// 从脏集合中移除，防止 execute 访问野指针
			mPendingSet.erase(current);
		}
	}

	bool DocumentBase::deleteDagLink(ObjectBase* source, ObjectBase* sink)
	{
		// 1. 基础合法性检查
		if (!source || !sink || source == sink) return false;

		// 2. 从源头的“出边表”中移除目标
		auto itOut = mOutGraph.find(source);
		if (itOut != mOutGraph.end())
		{
			auto& v = itOut->second;
			v.erase(std::remove(v.begin(), v.end(), sink), v.end());
			// 如果该源头不再驱动任何对象，清理掉 Map 项以节省内存
			if (v.empty()) mOutGraph.erase(itOut);
		}

		// 3. 从目标的“入边表”中移除源头
		auto itIn = mInGraph.find(sink);
		if (itIn != mInGraph.end())
		{
			auto& v = itIn->second;
			v.erase(std::remove(v.begin(), v.end(), source), v.end());
			if (v.empty()) mInGraph.erase(itIn);
		}

		return true;
	}

	std::vector<ObjectBase*> DocumentBase::calculateEvaluationOrder(const std::unordered_set<ObjectBase*>& dirtySet)
	{
		std::vector<ObjectBase*> result;
		std::unordered_set<ObjectBase*> visited;

		// 核心 DFS：不再过滤类型，保留所有 ObjectBase
		std::function<void(ObjectBase*)> visit = [&](ObjectBase* curr) {
			if (!curr || visited.count(curr)) return;

			visited.insert(curr);

			// 1. 探测 DAG 显式依赖 (Source Property -> Sink Property)
			auto it = mOutGraph.find(curr);
			if (it != mOutGraph.end())
				for (ObjectBase* sink : it->second)
					visit(sink);

			// 2. 探测隐式依赖：属性变动必然导致所属 Node 变动 (Property -> Owner Node)
			if (curr->getObjectType() == ObjectType::PROPERTY)
			{
				auto* prop = static_cast<PropertyBase*>(curr);
				if (NodeBase* owner = prop->getNode())
				{
					owner->markDirty(prop); // 标记所属节点的哪个属性脏了，方便后续执行时精准同步
					visit(owner);
				}
			}

			// 3. 将对象放入序列（由于是后序，顺序是 [终端 -> 源头]）
			result.push_back(curr);
		};

		for (auto* obj : dirtySet)
			visit(obj);

		// 翻转：得到正确的执行顺序 [源头 -> 终端]
		std::reverse(result.begin(), result.end());

		return result;
	}

	bool DocumentBase::isCycle(ObjectBase* source, ObjectBase* sink)
	{
		// 逻辑：从 sink 开始深度优先遍历，看能不能搜到 source
		std::unordered_set<ObjectBase*> visited;
		std::stack<ObjectBase*> s;
		s.push(sink);

		while (!s.empty())
		{
			ObjectBase* current = s.top();
			s.pop();

			if (current == source) return true; // 发现路径，说明会构成环

			if (mOutGraph.count(current))
				for (ObjectBase* next : mOutGraph[current])
					if (visited.find(next) == visited.end())
					{
						visited.insert(next);
						s.push(next);
					}
		}
		return false;
	}
} // namespace core