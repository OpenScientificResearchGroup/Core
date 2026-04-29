/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#pragma once
#include "Data/virNodeSetBase.hpp"

#include <unordered_map>
#include <vector>
// #include <set>
#include <unordered_set>

#include "Command/lgcCommandManager.hpp"

namespace core
{
	class TransactionGuard
	{
	public:
		TransactionGuard(Document* doc) : mDoc(doc)
		{
			if (mDoc) mDoc->startTransaction();
		}

		~TransactionGuard()
		{
			if (mDoc) mDoc->endTransaction();
		}

		TransactionGuard(const TransactionGuard&) = delete;
		TransactionGuard& operator=(const TransactionGuard&) = delete;
		TransactionGuard(TransactionGuard&&) = delete;
		TransactionGuard& operator=(TransactionGuard&&) = delete;

	private:
		Document* mDoc;

	};

	class Document : public NodeSetBase
	{
	public:
		Document();
		virtual ~Document() = default;

		// --- 1. 持久化 (Persistence) ---
		virtual bool load(const std::string& path);		
		virtual bool save(const std::string& path = "");
		const std::string& getPath() const;

		// --- 2. 全局索引 (Fast Lookup) ---
		// 允许任何命令通过 UUID 快速定位节点，无需遍历整棵树
		NodeBase* findNode(const std::string& uuid) const;

		// --- 3. 命名管理 ---
		// 生成如 "Part_1", "Part_2" 的默认名称
		std::string generateUniqueName(const std::string& prefix);

		// --- 4. 命令管理 ---
		core::CommandManager& getCommandManager();

        // --- 5. 事件钩子 (Hooks) ---
        // 用于维护属性依赖图、发送 UI 刷新信号等。
		void onUpdate(ObjectBase* obj) override;
		void onAttach(ObjectBase* obj) override;
		void onDetach(ObjectBase* obj) override;
		void onLink(ObjectBase* obj) override;
		void onUnlink(ObjectBase* obj) override;

		//void update() override;
		void attach() override;
		void detach() override;

		// --- 6. 事务支持 (Transaction) ---
		void startTransaction();
		void endTransaction();
		bool execute() override;

	private:
		// --- 7. DAG支持 ---
		bool insertDagNode(ObjectBase* obj);
		bool insertDagLink(ObjectBase* source, ObjectBase* sink);
		void deleteDagNode(ObjectBase* obj);
		bool deleteDagLink(ObjectBase* source, ObjectBase* sink);
		
		bool isCycle(ObjectBase* source, ObjectBase* sink);
		std::vector<NodeBase*> calculateEvaluationOrder(const std::unordered_set<ObjectBase*>& dirtySet);

		std::string mPath;															// 当前文档路径，保存时更新
		core::CommandManager mCommandManager;										// 命令管理器，用于处理撤销/重做等操作
		size_t mCnt;																// 用于生成新节点的计数器，确保每个新节点都有唯一的名称
		int mTransactionCount = 0;													// 当前事务计数，支持嵌套事务

		std::unordered_set<ObjectBase*> mPendingSet;								// 脏对象集合，使用 set 自动去重
		std::unordered_set<ObjectBase*> mPendingLinks;								// 记录哪些对象的 Link 发生了变化，需要在 execute 时重新连线
		std::unordered_map<std::string, NodeBase*> mNodeIndex;						// UUID 到节点的快速索引
		
		std::unordered_map<ObjectBase*, std::vector<ObjectBase*>> mOutGraph;		// 出边表：Source -> 被谁依赖 (用于传播“脏”信号)
		std::unordered_map<ObjectBase*, std::vector<ObjectBase*>> mInGraph;			// 入边表：Sink -> 依赖谁 (用于删除对象时，快速找到它的“上游”)

	};
}