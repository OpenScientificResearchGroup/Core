#include "Command/lgcCompositeCommand.hpp"

namespace core
{
	CompositeCommand::CompositeCommand (const std::string& name, const std::string& id)
		: CommandBase(), mName(name), mId(id)
	{

	}

	//CompositeCommand::~CompositeCommand()
	//{
	//	// 自动调用 mCmds 的析构，进而释放 unique_ptr
	//	// 逻辑发生在 DLL 内部堆中
	//}

	bool CompositeCommand::execute()
	{
		// 依次执行所有子命令
		// 注意：如果是事务提交模式，通常子命令在加入时已经 execute 过一次了。
		// 所以这个 execute 通常用于 Redo 或者作为整体命令初次执行。
		//for (auto& cmd : mCmds)
		//	if (!cmd->execute()) return false; // 如果中间出错，可能需要回滚（根据具体需求实现）
		for (unsigned int i = 0; i < mCmds.size(); ++i)
		{
			if (!mCmds[i]->execute())
			{
				// 如果执行失败，撤销已经成功执行的命令
				for (int j = i - 1; j >= 0; --j) mCmds[j]->undo();
				return false;
			}
		}
		return true;
	}

	void CompositeCommand::undo()
	{
		// 逆序撤销
		for (auto it = mCmds.rbegin(); it != mCmds.rend(); ++it) (*it)->undo();
	}

	void CompositeCommand::redo()
	{
		// 正序重做
		for (auto& cmd : mCmds) cmd->redo();
	}

	std::string CompositeCommand::getName() const
	{
		return mName;
	}

	std::string CompositeCommand::getId() const
	{
		return mId;
	}

	void CompositeCommand::append(std::unique_ptr<CommandBase> cmd)
	{
		if (cmd) mCmds.push_back(std::move(cmd));
	}

	bool CompositeCommand::isEmpty() const
	{
		return mCmds.empty();
	}

	void CompositeCommand::setName(const std::string& name)
	{
		mName = name;
	}

	void CompositeCommand::setId(const std::string& id)
	{
		mId = id;
	}
}
