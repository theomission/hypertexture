#include <vector>
#include <deque>
#include <iostream>
#include "task.hh"
#include "common.hh"
#include "ui.hh"
#include "camera.hh"
#include "render.hh"
#include "font.hh"

using namespace std;

////////////////////////////////////////////////////////////////////////////////
// Types
class Worker
{
public:
	Worker();
	~Worker();

	void RequestJoin();
	void JoinThread();
	void Signal();
	bool Ready() const { return !m_task && bool(m_ready); }
	bool Finished() const;
	void StartTask(const std::shared_ptr<Task>& task);
	void OnJoin() ;
private:
	static void RunWorker(Worker& worker);
	std::condition_variable m_cond;
	std::mutex m_mutex;
	std::shared_ptr<Task> m_task;
	int m_ready;						// 1 if the worker is ready to go
	int m_joinRequested;				// true if main thread wants this worker to stop
	std::thread m_thread;
} ;

Worker::Worker()
	: m_ready(0)
	, m_joinRequested(0)
	, m_thread(RunWorker, std::ref(*this))
{
}
	
Worker::~Worker()
{
	if(m_thread.joinable()) {
		m_thread.join();
	}
}

void Worker::RunWorker(Worker& worker)
{
	while(!worker.m_joinRequested)
	{
		if(worker.m_task && !worker.m_task->IsComplete())
		{
			worker.m_task->m_run();
			worker.m_task->SetComplete();
		}
	
		if(!worker.m_joinRequested) 
		{
			std::unique_lock<std::mutex> lk(worker.m_mutex);
			worker.m_ready = 1;
			worker.m_cond.wait(lk);
			worker.m_ready = 0;
		}
	}
}
	
void Worker::RequestJoin()
{
	m_joinRequested = 1;
	Signal();
}
	
void Worker::JoinThread()
{
	m_thread.join();
}
	
bool Worker::Finished() const
{
	return(m_task && m_task->IsComplete());
}

void Worker::Signal()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	m_cond.notify_one();
}

void Worker::StartTask(const std::shared_ptr<Task>& task)
{
	ASSERT(!m_task);
	ASSERT(m_ready);
	m_task = task;
	if(m_task->m_init) m_task->m_init();
	Signal();
}

void Worker::OnJoin()
{
	if(m_task) {
		if(m_task->m_join) m_task->m_join();
		m_task.reset();
	}
}

////////////////////////////////////////////////////////////////////////////////
// File-scope globals
static std::vector<std::shared_ptr<Worker>> g_workers;
static std::deque<std::shared_ptr<Task>> g_taskQueue;
static int g_curTotalJobs;
static int g_curCompletedJobs;

////////////////////////////////////////////////////////////////////////////////
void task_Startup(int numWorkers)
{
	for(int i = 0; i < numWorkers; ++i)
		g_workers.push_back(std::make_shared<Worker>());
}

void task_Shutdown()
{
	for(auto worker: g_workers)
		worker->RequestJoin();
	for(auto worker: g_workers)
	{
		worker->JoinThread();
		worker->OnJoin();
	}
	g_workers.clear();
}

static std::shared_ptr<Task> task_PopNext()
{
	int numAttempts = g_taskQueue.size();
	while(numAttempts--> 0)
	{
		std::shared_ptr<Task> nextTask = g_taskQueue.front();
		g_taskQueue.pop_front();

		if(nextTask->CanStart())
			return nextTask;
		
		g_taskQueue.push_back(nextTask);
	}	
	return nullptr;
}

void task_Update()
{
	for(auto worker: g_workers)
	{
		if(worker->Finished()) { 
			worker->OnJoin();
			++g_curCompletedJobs;
		}

		if(worker->Ready() && !g_taskQueue.empty())
		{
			std::shared_ptr<Task> nextTask = task_PopNext();
			if(nextTask) // if no tasks can be started, this can be null
				worker->StartTask(nextTask);
		}
	}

	if(g_curTotalJobs == g_curCompletedJobs)
	{
		g_curTotalJobs = 0;
		g_curCompletedJobs = 0;
	}
}

void task_AppendTask(const std::shared_ptr<Task>& task)
{
	g_taskQueue.push_back(task);
	++g_curTotalJobs;
}

void task_RenderProgress()
{
	if(g_curTotalJobs == 0) return;

	float ratio = float(g_curCompletedJobs) / float(g_curTotalJobs);
	ui_DrawColoredQuad(0.f, g_screen.m_height - 20.f, g_screen.m_width, 20.f, 1.f,
		Color(0,0,0), Color(1,1,1));
	ui_DrawColoredQuad(1.f, g_screen.m_height - 19.f, (g_screen.m_width - 2.f) * ratio, 18.f, 0.f,
		Color(1,1,1));

	{
		char progressStr[32] = {};
		static Color kWhite = {1};
		static Color kBlack = {0};
		snprintf(progressStr, sizeof(progressStr) - 1, "%d/%d", g_curCompletedJobs, g_curTotalJobs);
		font_Print(10.f, g_screen.m_height - 5, progressStr, g_curCompletedJobs > 0 ? kBlack : kWhite, 16.f);
	}

	checkGlError("task_RenderProgress");
}


