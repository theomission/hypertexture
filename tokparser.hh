#pragma once

#include <cstdio>
#include <vector>

class Tokenizer
{
public:
	Tokenizer(const char* data, int size) : m_data(data), m_size(size), m_pos(0) {}

	void Reset() { m_pos = 0; }
	bool Next(char* tok, int maxLen);
private:
	const char* m_data;
	int m_size;
	int m_pos;
};

class TokFlagDef
{
public:
	TokFlagDef(const char* name, int flag) : m_name(name), m_flag(flag) {}
	const char* m_name;
	int m_flag;
};

class TokParser
{
public:
	TokParser(const char* data, int size);
	operator bool() const { return Ok(); }
	bool Ok() const ;

	int GetString(char* buffer, int maxLen);
	int GetInt();
	float GetFloat();
	int GetFlag(const std::vector<TokFlagDef>& flagdef);
	bool ExpectTok(const char* str); 
	bool IsTok(const char* str); 

private:
	void Consume();

	Tokenizer m_tokenizer;
	char m_token[256];
	bool m_error;
	bool m_eof;
};

class TokWriter
{
public:
	TokWriter(const char* filename);
	~TokWriter();
	operator bool() const { return Ok(); }
	bool Ok() const ;
	bool Error() const;

	TokWriter(const TokWriter&) = delete;
	TokWriter& operator=(const TokWriter&) = delete;

	void Comment(const char* str);
	void String(const char* str);
	void Int(int value);
	void Float(float value);
	void Nl();
	void Flag(const std::vector<TokFlagDef>& flags, int value);
	void Token(const char* str);
private:
	FILE* m_fp;
	bool m_error;
	bool m_emptyLine;
} ;

