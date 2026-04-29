/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Data/lgcDocument.hpp"

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <queue>
#include <stack>

#include "Log/lgcLogManager.hpp"
#include "Utility/lgcString.hpp"

namespace core
{
	Document::Document()
		: NodeSetBase(), mPath(""), mCommandManager(50), mCnt(0), mTransactionCount(0)
	{

	}

	bool Document::load(const std::string& path)
	{
		// 实现加载逻辑
		mPath = path;

		std::ifstream ifs(mPath);
		if (!ifs.is_open()) return false;
		std::stringstream buffer;
		buffer << ifs.rdbuf();
		nlohmann::json file = nlohmann::json::parse(buffer.str());
		if (!read(file)) return false;
		return true;
	}

	bool Document::save(const std::string& path)
	{
		
		if (!path.empty()) mPath = path;
		nlohmann::json file = write();
		std::ofstream ofs(mPath);
		if (!ofs.is_open()) return false;
		ofs << file.dump();
		return true; // 成功
	}

	const std::string& Document::getPath() const
	{
		return mPath;
	}

	NodeBase* Document::findNode(const std::string& uuid) const
	{
		auto it = mNodeIndex.find(uuid);
		if (it != mNodeIndex.end())
			return it->second;
		return nullptr;
	}

	std::string Document::generateUniqueName(const std::string& prefix)
	{
		return prefix + "_" + std::to_string(++mCnt);
	}

	core::CommandManager& Document::getCommandManager()
	{
		return mCommandManager;
	}

	void Document::onUpdate(ObjectBase* obj)
	{
		mPendingSet.insert(obj);
	}

	void Document::onAttach(ObjectBase* obj)
	{
		mPendingSet.insert(obj);

		if (obj->getObjectType() == ObjectType::NODE)
			if (auto* node = static_cast<NodeBase*>(obj))
			{
				mNodeIndex[node->getUuid()] = node;
				for (const auto& prop : node->getAllProperties())
					if (prop->isLink()) // 如果该属性是链接状态
					{
						// 从属性元数据中提取源 UUID 和 源 Key
						std::string srcUuid = prop->getLink().uuid;
						std::string srcPath = prop->getLink().path;

						// 利用 Document 的全局索引查找源节点
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
	}

	void Document::onDetach(ObjectBase* obj)
	{
		if (obj->getObjectType() == ObjectType::NODE)
			if (auto* node = static_cast<NodeBase*>(obj))
				mNodeIndex.erase(node->getUuid());
		deleteDagNode(obj);
	}

	void Document::onLink(ObjectBase* obj)
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
		insertDagLink(source, obj);
	}

	void Document::onUnlink(ObjectBase* obj)
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
	}

	//void Document::update()
	//{
	//	// Document 不能被更新
	//}

	void Document::attach()
	{
		// Document 不能被附加
	}

	void Document::detach()
	{
		// Document 不能被移除
	}

	void Document::startTransaction()
	{
		mTransactionCount++;
	}

	void Document::endTransaction()
	{
		if (mTransactionCount <= 0) return;
		if (--mTransactionCount == 0/* && mPendingExecute*/)
			execute();
	}

	bool Document::execute()
	{
		// 1. 安全检查：如果没有待处理的对象，直接跳过
		if (mPendingSet.empty()) return true;

		// 2. 拓扑排序：计算执行顺序
		// 该函数已经将 Property 变动提升到了 Node 级别，并根据 DAG 确定了顺序
		std::vector<NodeBase*> evalOrder = calculateEvaluationOrder(mPendingSet);

		bool allSuccess = true;

		// 3. 核心执行循环
		for (NodeBase* node : evalOrder)
		{
			// A. 属性同步 (Data Binding Sync)
			// 在执行 Node 逻辑前，必须拉取它所有引用属性的最新值。
			// 否则 execute() 拿到的可能是旧的缓存数据。
			// 遍历该 Node 的所有属性，检查是否存在外部引用（Link）
			for (const auto& prop : node->getAllProperties())
			{
				if (prop->isLink()) // 如果该属性是链接状态
				{
					// 从属性元数据中提取源 UUID 和 源 Key
					std::string srcUuid = prop->getLink().uuid;
					std::string srcPath = prop->getLink().path;

					// 利用 Document 的全局索引查找源节点
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

			// B. 逻辑刷新 (Logic Evaluation)
			// 内部调用子类实现的 execute()，并自动清除节点的 mIsDirty 标记
			if (!node->execute())
			{
				allSuccess = false;
				// 这里可以记录错误日志，或者根据业务决定是否中断
			}
		}

		// 4. 状态收尾
		// 计算完成后清空脏集合，准备迎接下一个事务
		mPendingSet.clear();

		// 5. 视图同步 (View Synchronization)
		// 所有的计算都已在内存中完成且一致，此时可以通知 UI 进行全量或增量重绘
		// this->notifyUiRepaint(); 

		return allSuccess;
	}

	bool Document::insertDagNode(ObjectBase* obj)
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
	bool Document::insertDagLink(ObjectBase* source, ObjectBase* sink)
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

	void Document::deleteDagNode(ObjectBase* obj)
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

	bool Document::deleteDagLink(ObjectBase* source, ObjectBase* sink)
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

	std::vector<NodeBase*> Document::calculateEvaluationOrder(const std::unordered_set<ObjectBase*>& dirtySet)
	{
		std::vector<NodeBase*> result;
		std::unordered_set<ObjectBase*> visited;

		// 核心 DFS 访问函数
		std::function<void(ObjectBase*)> visit = [&](ObjectBase* curr) {
			if (!curr || visited.count(curr)) return;

			// 1. 标记已访问，防止重复处理
			visited.insert(curr);

			// 2. 纵深探测：先访问所有依赖于 curr 的对象（出边）
			// 在 DAG 中，这意味着我们要先处理“下游”
			auto it = mOutGraph.find(curr);
			if (it != mOutGraph.end())
				for (ObjectBase* sink : it->second)
					visit(sink);

			// 3. 逻辑提升（Property -> Node）：
			// 如果一个属性变了，它所属的节点必须参与计算。
			// 我们将其视为一种隐式的依赖关系。
			if (curr->getObjectType() == ObjectType::PROPERTY)
			{
				auto* prop = static_cast<PropertyBase*>(curr);
				if (NodeBase* owner = prop->getNode())
					visit(owner);
			}

			// 4. 后序收集：
			// 只有节点（Node）需要执行 refresh()。
			// 由于是后序，最底层的“叶子”节点会最先被存入列表。
			if (curr->getObjectType() == ObjectType::NODE)
				result.push_back(static_cast<NodeBase*>(curr));
		};

		// 5. 从所有脏对象开始探测受影响的闭包
		for (auto* obj : dirtySet)
			visit(obj);

		// 6. 逆序反转
		// DFS 后序的结果是 [下游, 中游, 上游]，反转后得到正确的执行序 [上游, 中游, 下游]
		std::reverse(result.begin(), result.end());

		return result;
	}

	bool Document::isCycle(ObjectBase* source, ObjectBase* sink)
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