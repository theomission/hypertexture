#pragma once

#include <vector>
#include <memory>
#include <string>
#include <functional>

#include "vec.hh"
#include "commonmath.hh"

class TokParser;
class TokWriter;

class TweakVarBase
{
public:
	TweakVarBase(const char* name) : m_name(name) {}
	virtual void Parse(TokParser& parser) = 0;	
	virtual void Write(TokWriter& writer) = 0;
	virtual void Reset() = 0;
	const std::string& GetName() const { return m_name; }
private:
	std::string m_name;
};

class TweakInt : public TweakVarBase
{
public:	
	TweakInt(const char* name, int* var, int def = 0, const Limits<int>& limits = Limits<int>());
	TweakInt(const char* name, std::function<int()> get,
		std::function<void(int)> set,
		int def = 0,
		const Limits<int>& limits = Limits<int>());

	void Parse(TokParser& parser);
	void Write(TokWriter& writer);
	void Reset() { m_set(m_default); }
private:
	std::function<int()> m_get;
	std::function<void(int)> m_set;
	Limits<int> m_limits;
	int m_default;
};

class TweakBool : public TweakVarBase
{
public:
	TweakBool(const char* name, int* var, bool def = false);
	TweakBool(const char* name, bool* var, bool def = false);
	TweakBool(const char* name, std::function<bool()> get, 
		std::function<void(bool)> set, bool def = false);

	void Parse(TokParser& parser);
	void Write(TokWriter& writer);
	void Reset() { m_set(m_default); }
private:
	std::function<bool()> m_get;
	std::function<void(bool)> m_set;
	bool m_default;
};

class TweakFloat : public TweakVarBase
{
public:	
	TweakFloat(const char* name, float* var, float def = 0, const Limits<float>& limits = Limits<float>());
	TweakFloat(const char* name,
		std::function<float()> get,
		std::function<void(float)> set,
		float def = 0,
		const Limits<float>& limits = Limits<float>());

	void Parse(TokParser& parser);
	void Write(TokWriter& writer);
	void Reset() { m_set(m_default); }
private:
	std::function<float()> m_get;
	std::function<void(float)> m_set;
	Limits<float> m_limits;
	float m_default;
};

class TweakColor : public TweakVarBase
{
public:	
	TweakColor(const char* name, Color* var, const Color& def = Color{0,0,0}, const Limits<Color>& limits = Limits<Color>());
	TweakColor(const char* name,
		std::function<Color()> get,
		std::function<void(const Color&)> set,
		const Color& def = Color{0,0,0},
		const Limits<Color>& limits = Limits<Color>());

	void Parse(TokParser& parser);
	void Write(TokWriter& writer);
	void Reset() { m_set(m_default); }
private:
	std::function<Color()> m_get;
	std::function<void(const Color&)> m_set;
	Limits<Color> m_limits;
	Color m_default;
};

class TweakVector : public TweakVarBase
{
public:	
	TweakVector(const char* name, vec3* var, const vec3& def = vec3{0}, const Limits<vec3>& limits = Limits<vec3>());
	TweakVector(const char* name, std::function<vec3()> get,
		std::function<void(const vec3&)> set,
		const vec3& def = vec3{0},
		const Limits<vec3>& limits = Limits<vec3>());

	void Parse(TokParser& parser);
	void Write(TokWriter& writer);
	void Reset() { m_set(m_default); }
private:
	std::function<vec3()> m_get;
	std::function<void(const vec3&)> m_set;
	Limits<vec3> m_limits;
	vec3 m_default;
};

bool tweaker_LoadVars(const char* filename, const std::vector<std::shared_ptr<TweakVarBase>>& vars);
bool tweaker_SaveVars(const char* filename, const std::vector<std::shared_ptr<TweakVarBase>>& vars);

