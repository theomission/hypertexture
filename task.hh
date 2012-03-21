#pragma once

#include <thread>
#include <functional>
#include <memory>

class Task
{
public:
	Task(std::function<void()> run, std::function<bool()> canStart = nullptr)
		: m_init()
		, m_join()
		, m_run(run)
		, m_canStart(canStart)
		, m_complete(0) {}

	Task(std::function<void()> init, 
		std::function<void()> join,
		std::function<void()> run,
		std::function<bool()> canStart = nullptr) 
		: m_init(init)
		, m_join(join)
		, m_run(run)
		, m_canStart(canStart)
		, m_complete(0) {}

	bool IsComplete() const { return m_complete; }
	void SetComplete() { m_complete = 1; }
	
	bool CanStart() const { return m_canStart == nullptr || m_canStart(); }
	std::function<void()> m_init;
	std::function<void()> m_join;
	std::function<void()> m_run;
	std::function<bool()> m_canStart;
private:

	int m_complete;
} ;

void task_Startup(int numWorkers);
void task_Shutdown();
void task_Update();
void task_AppendTask(const std::shared_ptr<Task>& task);
void task_RenderProgress();

