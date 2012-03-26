#pragma once

class Clock
{
public:
	Clock();

	float GetDt() const { return m_lastDt; }
	void Step(float minDt = 0.0);

private:
	unsigned long long m_lastTime;
	float m_lastDt;
};

class Timer
{
public:
	Timer();

	void Start();
	void Stop();

	float GetTime();
private:
	unsigned long long m_startTime;
	unsigned long long m_stopTime;
};

