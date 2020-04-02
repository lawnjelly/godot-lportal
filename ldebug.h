#pragma once

//	Copyright (c) 2019 Lawnjelly

//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:

//	The above copyright notice and this permission notice shall be included in all
//	copies or substantial portions of the Software.

//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//	SOFTWARE.

// most of the performance sensitive debug output will be compiled out in release builds
// you won't be able to get frame debugging of the visibility tree though.
#ifdef DEBUG_ENABLED

#pragma message ("LPortal DEBUG_ENABLED, verbose mode")
#define LPRINT_RUN(a, b) {if (!Lawn::LDebug::m_bRunning) {String sz;\
for (int n=0; n<Lawn::LDebug::m_iTabDepth; n++)\
sz += "\t";\
LPRINT(a, sz + b);}}

//#define LPRINT_RUN(a, b) ;

#else
#define LPRINT_RUN(a, b) ;
#endif

#ifdef LDEBUG_VERBOSE
#define LPRINT(a, b) {LPRINT_IMPL(a, b);}
#else
#define LPRINT(a, b) {if (!Lawn::LDebug::m_bRunning) {LPRINT_IMPL(a, b);}}
#endif

#define LPRINT_IMPL(a, b) {\
if (a >= Lawn::LDebug::m_iLoggingLevel)\
{\
Lawn::LDebug::print(b);\
}\
}


#define LWARN(a, b) if (a >= Lawn::LDebug::m_iWarningLevel)\
{\
Lawn::LDebug::print(String("\tWARNING : ") + b);\
}

String ftos(float f) {return String(Variant(f));}


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

