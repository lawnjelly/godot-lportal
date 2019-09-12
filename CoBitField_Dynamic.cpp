#include "CoBitField_Dynamic.h"

#include <string.h>


namespace Core { // namespace start

void CoBitField_Dynamic::Initialize() {assert (0 && "CoBitField_Dynamic : Does not support Initialize, use IT version");}
void CoBitField_Dynamic::Terminate() {assert (0 && "CoBitField_Dynamic : Does not support Terminate, use IT version");}

 
void CoBitField_Dynamic_IT::Initialize()
{
	Initialize_Do();
}

void CoBitField_Dynamic_IT::Terminate()
{
	Terminate_Do();
}


void CoBitField_Dynamic_IT::Initialize_Do()
{
	memset (this, 0, sizeof (CoBitField_Dynamic));
}

void CoBitField_Dynamic_IT::Terminate_Do()
{
	Destroy();
}

void CoBitField_Dynamic_IT::CopyFrom(const CoBitField_Dynamic_IT &source)
{
	Create(source.GetNumBits(), false);
	memcpy(m_pucData, source.GetData(), source.GetNumBytes());
}


void CoBitField_Dynamic_IT::Create(unsigned int uiNumBits, bool bBlank)
{
	// first delete any initial
	Destroy();

	m_uiNumBits = uiNumBits;
	if (uiNumBits)
	{
		m_uiNumBytes = (uiNumBits / 8) + 1;

		m_pucData = new unsigned char[m_uiNumBytes];

		if (bBlank)
			Blank(false);
	}
}

void CoBitField_Dynamic_IT::Destroy()
{
	if (m_pucData)
	{
		delete[] m_pucData;
		m_pucData = 0;
	}

	memset (this, 0, sizeof (CoBitField_Dynamic));
}


void CoBitField_Dynamic_IT::Blank(bool bSetOrZero)
{
	if (bSetOrZero)
	{
		memset(m_pucData, 255, m_uiNumBytes);
	}
	else
	{
		memset(m_pucData, 0, m_uiNumBytes);
	}
}

void CoBitField_Dynamic_IT::Invert()
{
	for (unsigned int n=0; n<m_uiNumBytes; n++)
	{
		m_pucData[n] = ~m_pucData[n];
	}
}

////////////////////////////////////////////////////////////////////////////



} // namespace end
