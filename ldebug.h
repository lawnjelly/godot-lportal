#pragma once

//#define LPRINT_RUN(a, b) ;

#define LPRINT_RUN(a, b) {String sz;\
for (int n=0; n<Lawn::LDebug::m_iTabDepth; n++)\
sz += "\t";\
LPRINT(a, sz + b);}

#define LPRINT(a, b) if (!Lawn::LDebug::m_bRunning) {\
if (a >= Lawn::LDebug::m_iLoggingLevel)\
{\
Lawn::LDebug::print(b);\
}\
}

#define LWARN(a, b) if (a >= Lawn::LDebug::m_iWarningLevel)\
{\
Lawn::LDebug::print(String("\tWARNING : ") + b);\
}


namespace Lawn
{

class LDebug
{
public:
	static void print(String sz);
	static int m_iLoggingLevel;
	static int m_iWarningLevel;
	static bool m_bRunning;

	static int m_iTabDepth;
};

} // namespace

