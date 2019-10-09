#include "ldebug.h"

namespace Lawn
{

int LDebug::m_iLoggingLevel = 0; // 2
int LDebug::m_iWarningLevel = 0;
int LDebug::m_iTabDepth = 0;
bool LDebug::m_bRunning = true;


void LDebug::print(String sz)
{
	print_line(sz);
}


} // namespace
