#include <cstdlib>
#include <cstring>
#include <fstream>
#include "tweaker.hh"
#include "tokparser.hh"
#include "commonmath.hh"

////////////////////////////////////////////////////////////////////////////////
TweakInt::TweakInt(const char* name, int* var, int def, const Limits<int>& limits)
	: TweakVarBase(name)
	, m_get([=](){return *var;})
	, m_set([=](int val){*var = val; })
	, m_limits(limits)
	, m_default(def)
{}

TweakInt::TweakInt(const char* name, std::function<int()> get,
		std::function<void(int)> set,
		int def,
		const Limits<int>& limits)
	: TweakVarBase(name)
	, m_get(get)
	, m_set(set)
	, m_limits(limits)
	, m_default(def)
{}	

void TweakInt::Parse(TokParser& parser)
{
	m_set(m_limits(parser.GetInt()));
}

void TweakInt::Write(TokWriter& writer)
{
	writer.Int(m_get());
}

////////////////////////////////////////////////////////////////////////////////
TweakBool::TweakBool(const char* name, int *var, bool def)
	: TweakVarBase(name)
	, m_get([=](){ return bool(*var); })
	, m_set([=](float val){ *var = val ? 1 : 0; })
	, m_default(def)
{
}

TweakBool::TweakBool(const char* name, bool *var, bool def)
	: TweakVarBase(name)
	, m_get([=](){ return *var; })
	, m_set([=](float val){ *var = val; })
	, m_default(def)
{
}
	
TweakBool::TweakBool(const char* name, std::function<bool()> get, 
	std::function<void(bool)> set, bool def)
	: TweakVarBase(name)
	, m_get(get)
	, m_set(set)
	, m_default(def)
{
}
	
void TweakBool::Parse(TokParser& parser)
{
	m_set(parser.GetInt() == 1);
}

void TweakBool::Write(TokWriter& writer)
{
	int val = m_get() ? 1 : 0;
	writer.Int(val);
}

////////////////////////////////////////////////////////////////////////////////
TweakFloat::TweakFloat(const char* name, float* var, float def, const Limits<float>& limits)
	: TweakVarBase(name)
	, m_get([=](){return *var;})
	, m_set([=](float val){*var = val; })
	, m_limits(limits)
	, m_default(def)
{}

TweakFloat::TweakFloat(const char* name, std::function<float()> get,
		std::function<void(float)> set,
		float def,
		const Limits<float>& limits)
	: TweakVarBase(name)
	, m_get(get)
	, m_set(set)
	, m_limits(limits)
	, m_default(def)
{}	

void TweakFloat::Parse(TokParser& parser)
{
	m_set(m_limits(parser.GetFloat()));
}

void TweakFloat::Write(TokWriter& writer)
{
	writer.Float(m_get());
}

////////////////////////////////////////////////////////////////////////////////
TweakColor::TweakColor(const char* name, Color* var, const Color& def, const Limits<Color>& limits)
	: TweakVarBase(name)
	, m_get([=](){return *var;})
	, m_set([=](const Color& val){*var = val; })
	, m_limits(limits)
	, m_default(def)
{}

TweakColor::TweakColor(const char* name, std::function<Color()> get,
		std::function<void(const Color&)> set,
		const Color& def, 
		const Limits<Color>& limits)
	: TweakVarBase(name)
	, m_get(get)
	, m_set(set)
	, m_limits(limits)
	, m_default(def)
{}	

void TweakColor::Parse(TokParser& parser)
{
	float r = parser.GetFloat(), g = parser.GetFloat(), b = parser.GetFloat();
	Color c = Color(r,g,b);
	m_set(m_limits(c));
}

void TweakColor::Write(TokWriter& writer)
{
	Color c = m_get();
	writer.Float(c.r);
	writer.Float(c.g);
	writer.Float(c.b);
}

////////////////////////////////////////////////////////////////////////////////
TweakVector::TweakVector(const char* name, vec3* var, const vec3& def, const Limits<vec3>& limits)
	: TweakVarBase(name)
	, m_get([=](){return *var;})
	, m_set([=](const vec3& val){*var = val; })
	, m_limits(limits)
	, m_default(def)
{}

TweakVector::TweakVector(const char* name, std::function<vec3()> get,
		std::function<void(const vec3&)> set,
		const vec3& def,
		const Limits<vec3>& limits)
	: TweakVarBase(name)
	, m_get(get)
	, m_set(set)
	, m_limits(limits)
	, m_default(def)
{}	

void TweakVector::Parse(TokParser& parser)
{
	float x = parser.GetFloat(), y = parser.GetFloat(), z = parser.GetFloat();
	vec3 v = vec3(x,y,z);
	m_set(m_limits(v));
}

void TweakVector::Write(TokWriter& writer)
{
	vec3 v = m_get();
	writer.Float(v.x);
	writer.Float(v.y);
	writer.Float(v.z);
}


////////////////////////////////////////////////////////////////////////////////
bool tweaker_LoadVars(const char* filename, const std::vector<std::shared_ptr<TweakVarBase>>& vars)
{
	for(auto var: vars)
		var->Reset();

	std::fstream file(filename, std::ios_base::in | std::ios_base::binary);
	if(!file) return true;

	file.seekg(0, std::ios_base::end);
	size_t fileSize = file.tellg();
	file.seekg(0);

	std::vector<char> data(fileSize);
	file.read(&data[0], fileSize);

	if(file.fail())
		return false;

	TokParser parser(&data[0], fileSize);
	
	// read in name = value pairs
	while(parser)
	{
		char bufname[256];
		parser.GetString(bufname, sizeof(bufname));
		parser.ExpectTok("=");
		if(!parser) break;
		for(auto var: vars)
		{
			if(strcasecmp(bufname, var->GetName().c_str()) == 0)
			{
				var->Parse(parser);
				break;
			}
		}
	}

	return true;
}


bool tweaker_SaveVars(const char* filename, const std::vector<std::shared_ptr<TweakVarBase>>& vars)
{
	TokWriter w(filename);
	if(!w)
		return false;

	for(auto var: vars)
	{
		w.String(var->GetName().c_str());
		w.Token("=");
		var->Write(w);
		w.Nl();
	}
	
	return !w.Error();
}

