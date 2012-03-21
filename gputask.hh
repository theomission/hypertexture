#pragma once

#include <memory>
#include <functional>

class GpuTask
{
public:
	GpuTask( std::function<void()> submit, std::function<void()> complete )
		: m_submit(submit), m_complete(complete) {}

	std::function<void()> m_submit;
	std::function<void()> m_complete;
};

void gputask_Kick();
void gputask_Join();
void gputask_Append(const std::shared_ptr<GpuTask>& task);

