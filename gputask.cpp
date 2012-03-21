#include "gputask.hh"
#include <deque>

static std::deque<std::shared_ptr<GpuTask>> g_gpuTasks;
static std::deque<std::shared_ptr<GpuTask>> g_kickedTasks;

void gputask_Kick()
{
	if(!g_gpuTasks.empty())
	{
		auto ptr = g_gpuTasks.front();
		g_gpuTasks.pop_front();

		if(ptr->m_submit) ptr->m_submit();
		if(ptr->m_complete)
			g_kickedTasks.push_back(ptr);
	}
}

void gputask_Join()
{
	if(!g_kickedTasks.empty())
	{
		auto ptr = g_kickedTasks.front();
		g_kickedTasks.pop_front();
		ptr->m_complete();
	}
}

void gputask_Append(const std::shared_ptr<GpuTask>& task)
{
	g_gpuTasks.push_back(task);
}


