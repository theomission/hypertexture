#include <cstdlib>
#include <cstring>
#include <cctype>
#include "tokparser.hh"

////////////////////////////////////////////////////////////////////////////////
bool Tokenizer::Next(char* tok, int maxLen)
{
	const char* data = m_data;
	int pos = m_pos;
	const int maxSize = m_size;

	// skip to the first non space
	while(pos < maxSize && isspace(data[pos]) )
		++pos;
	if(pos == maxSize)
		return 0;

	// skip comments
	while(pos < maxSize && data[pos] == ';')
	{
		while(pos < maxSize && data[pos] != '\n')
			++pos;
		while(pos < maxSize && isspace(data[pos]))
			++pos;
	}
	if(pos == maxSize)
		return 0;
	int tokStart = pos;

	// read the token
	int posTok = 0;
	const int maxPosTok = maxLen - 1 ;
	if(data[pos] == '"')
	{
		++pos;
		while( pos < maxSize && data[pos] != '"' )
		{
			if(posTok < maxPosTok)
				tok[posTok++] = data[pos];
			++pos;
		}

		if(pos < maxSize && data[pos] == '"')
			++pos;
	}
	else
	{
		while( pos < maxSize && !isspace(data[pos]))
		{
			if(posTok < maxPosTok)
				tok[posTok++] = data[pos];
			++pos;
		}
	}
	tok[posTok] = '\0';
	m_pos = pos;
	return pos > tokStart;
}

////////////////////////////////////////////////////////////////////////////////
TokParser::TokParser(const char* data, int size)
	: m_tokenizer(data, size)
	, m_token{}
	, m_error(false)
	, m_eof(false)
{
	Consume();
}

void TokParser::Consume()
{
	if(m_eof) { m_error = true; return; }
	if(!m_tokenizer.Next(m_token, sizeof(m_token)))
	{
		m_token[0] = '\0';
		m_eof = true;
	}
}

int TokParser::GetString(char* buffer, int maxLen)
{
	if(m_eof) { m_error = true; return 0; }
	int i, len = maxLen - 1;
	for(i = 0; i < len; ++i)
		buffer[i] = m_token[i];
	buffer[i] = '\0';
	Consume();
	return i;
}

int TokParser::GetInt()
{
	if(m_eof) { m_error = true; return 0; }
	int value = atoi(m_token);
	Consume();
	return value;
}

float TokParser::GetFloat()
{
	if(m_eof) { m_error = true; return 0.f; }
	float value = atof(m_token);
	Consume();
	if(m_token[0] == ':')
	{
		Consume();
		char* endPtr;
		unsigned long rawValue = strtoul(m_token, &endPtr, 16);
		if(endPtr == m_token)
		{
			m_error = true;
			return 0.f;
		}
		unsigned int rawValue32 = (unsigned int)(rawValue);
		// TODO: correct endian-ness
		memcpy(&value, &rawValue32, sizeof(value));
		Consume();
	}
	return value;
}

int TokParser::GetFlag(const std::vector<TokFlagDef>& def)
{
	if(m_eof) { m_error = true; return 0; }
	const int numFlags = def.size();
	int value = 0;
	const char* curWord = m_token;
	const char* cursor = m_token;
	while(1) {
		if(*cursor == '+' || !*cursor) {
			int len = cursor - curWord;
			for(int flag = 0; flag < numFlags; ++flag)
			{
				if(strncasecmp(def[flag].m_name, curWord, len) == 0)
				{
					value |= def[flag].m_flag;
					break;
				}
			}
			// warn here if flag not recognized?

			if(!*cursor)
				break;

			++cursor;
			curWord = cursor;
		}
		else
			++cursor;
	}
	Consume();
	return value;
}

bool TokParser::ExpectTok(const char* str)
{
	if(m_eof) { m_error = true; return 0; }
	bool result = strcmp(str, m_token) == 0;
	Consume();
	if(!result)
		m_error = true;
	return result;
}
	
bool TokParser::IsTok(const char* str)
{
	if(m_eof) { return false; }
	bool result = strcmp(str, m_token) == 0;
	return result;
}

bool TokParser::Ok() const
{
	return !m_error && !m_eof;
}

////////////////////////////////////////////////////////////////////////////////
TokWriter::TokWriter(const char* filename)
	: m_fp(nullptr)
	, m_error(false)
	, m_emptyLine(true)
{
	m_fp = fopen(filename, "w");
	if(!m_fp)
	{
		m_error = true;
	}
}

TokWriter::~TokWriter()
{
	if(m_fp)
		fclose(m_fp);
}
	
bool TokWriter::Ok() const 
{
	return !m_error;
}
	
bool TokWriter::Error() const 
{ 
	return m_error; 
}

void TokWriter::Comment(const char* str)
{
	fprintf(m_fp, "%s; %s\n", 
		(m_emptyLine ? "" : " "), str);
	m_emptyLine = true;
}

void TokWriter::String(const char* str)
{
	bool hasSpace = false;
	const char* cursor = str;
	while(*cursor) {
		if(isspace(*cursor))
		{
			hasSpace = true;
			break;
		}
		++cursor;
	}

	const char* start = m_emptyLine ? "" : " ";
	const char* quote = (hasSpace || *str) ? "\"" : "";
	fprintf(m_fp, "%s%s%s%s", start, quote, str, quote);
	m_emptyLine = false;
}

void TokWriter::Int(int value)
{
	fprintf(m_fp, "%s%d", 
		(m_emptyLine ? "" : " "), value);
	m_emptyLine = false;
}

void TokWriter::Float(float value)
{
	unsigned int rawValue32 = 0;
	memcpy(&rawValue32, &value, sizeof(rawValue32));
	fprintf(m_fp, "%s%g : %x", 
		(m_emptyLine ? "" : " "), value, rawValue32);
	m_emptyLine = false;
}

void TokWriter::Nl()
{
	fprintf(m_fp, "\n");
	m_emptyLine = true;
}

void TokWriter::Flag(const std::vector<TokFlagDef>& def, int value)
{
	const int numFlags = def.size();
	int emptyFlag = 1;
	if(!m_emptyLine) fprintf(m_fp, " ");
	int flag;
	for(flag = 0; flag < numFlags; ++flag)
	{
		if(value & def[flag].m_flag)
		{
			const char* plus = emptyFlag ? "" : "+";
			emptyFlag = 0;
			fprintf(m_fp, "%s%s", plus, def[flag].m_name);
		}
	}
	if(emptyFlag)
		fprintf(m_fp, "\"\"");
	m_emptyLine = false;
}

void TokWriter::Token(const char* tok)
{
	fprintf(m_fp, "%s%s",
		(m_emptyLine ? "" : " "), tok);
	m_emptyLine = false;
}


