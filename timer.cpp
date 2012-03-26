#include <ctime>
#include "timer.hh"
#include <iostream>

////////////////////////////////////////////////////////////////////////////////
static unsigned long long CurTime() 
{
	struct timespec currentTime;
	clock_gettime(CLOCK_MONOTONIC, &currentTime);
	unsigned long long timeUsec = (unsigned long long)(currentTime.tv_sec * 1000000) +
		(unsigned long long)(currentTime.tv_nsec / 1000);
	return timeUsec;	
}

////////////////////////////////////////////////////////////////////////////////
Clock::Clock()
	: m_lastDt(0)
{
	m_lastTime = CurTime();
}
	
void Clock::Step(float minDt)
{
	unsigned long long cur = CurTime();
	unsigned long long diff = cur - m_lastTime;
	float dt = diff / 1e6f;
	if(dt < minDt)
	{
		m_lastDt = 0.0;
		return;
	}

	m_lastDt = dt;
	m_lastTime = cur;
}

////////////////////////////////////////////////////////////////////////////////
Timer::Timer()
	: m_startTime(0)
	, m_stopTime(0)
{
}

void Timer::Start()
{
	m_startTime = CurTime();
}

void Timer::Stop()
{
	m_stopTime = CurTime();
}
	
float Timer::GetTime()
{
	return (m_stopTime - m_startTime) / 1e6f;
}

