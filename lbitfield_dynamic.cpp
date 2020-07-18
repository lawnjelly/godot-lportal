#include "lbitfield_dynamic.h"

#include <string.h>


namespace Lawn { // namespace start

void LBitField_Dynamic::Initialize() {assert (0 && "LBitField_Dynamic : Does not support Initialize, use IT version");}
void LBitField_Dynamic::Terminate() {assert (0 && "LBitField_Dynamic : Does not support Terminate, use IT version");}

 
void LBitField_Dynamic_IT::Initialize()
{
	Initialize_Do();
}

void LBitField_Dynamic_IT::Terminate()
{
	Terminate_Do();
}


void LBitField_Dynamic_IT::Initialize_Do()
{
	memset (this, 0, sizeof (LBitField_Dynamic));
}

void LBitField_Dynamic_IT::Terminate_Do()
{
	Destroy();
}

void LBitField_Dynamic_IT::CopyFrom(const LBitField_Dynamic_IT &source)
{
	Create(source.GetNumBits(), false);
	memcpy(m_pucData, source.GetData(), source.GetNumBytes());
}


void LBitField_Dynamic_IT::Create(unsigned int uiNumBits, bool bBlank)
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

void LBitField_Dynamic_IT::Destroy()
{
	if (m_pucData)
	{
		delete[] m_pucData;
		m_pucData = 0;
	}

	memset (this, 0, sizeof (LBitField_Dynamic));
}


void LBitField_Dynamic_IT::Blank(bool bSetOrZero)
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

void LBitField_Dynamic_IT::Invert()
{
	for (unsigned int n=0; n<m_uiNumBytes; n++)
	{
		m_pucData[n] = ~m_pucData[n];
	}
}

////////////////////////////////////////////////////////////////////////////



} // namespace end
