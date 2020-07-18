#pragma once

#include <assert.h>

namespace Lawn { // namespace start

class LBitField_Dynamic_IT
{
public:
	// construction
	void Initialize();
	void Terminate();

private:
	// prevent copying (see effective C++ scott meyers)
	// there is no implementation for copy constructor, hence compiler will complain if you try to copy.
	LBitField_Dynamic_IT& operator=(const LBitField_Dynamic_IT&);
public:

	// create automatically blanks
	void Create(unsigned int uiNumBits, bool bBlank = true);
	void Destroy();

	// public funcs
	inline unsigned int GetNumBits() const {return m_uiNumBits;}
	inline unsigned int GetBit(unsigned int uiBit) const;
	inline void SetBit(unsigned int uiBit, unsigned int bSet);
	bool CheckAndSet(unsigned int uiBit);
	void Blank(bool bSetOrZero = false);
	void Invert();
	void CopyFrom(const LBitField_Dynamic_IT &source);

	// loading / saving
	unsigned char * GetData() {return m_pucData;}
	const unsigned char * GetData() const {return m_pucData;}
	unsigned int GetNumBytes() const {return m_uiNumBytes;}

protected:
	// member funcs
	void Initialize_Do();
	void Terminate_Do();

	// member vars
	unsigned char * m_pucData;
	unsigned int m_uiNumBytes;
	unsigned int m_uiNumBits;
};

class LBitField_Dynamic : public LBitField_Dynamic_IT
{
public:
	// call initialize and terminate automatically
	LBitField_Dynamic(unsigned int uiNumBits) {Initialize_Do(); Create(uiNumBits);}
	LBitField_Dynamic() {Initialize_Do();}
	~LBitField_Dynamic() {Terminate_Do();}

	// disallow explicit calls
	void Initialize();
	void Terminate();
};


//////////////////////////////////////////////////////////
inline unsigned int LBitField_Dynamic_IT::GetBit(unsigned int uiBit) const
{
	assert (m_pucData);
	unsigned int uiByteNumber = uiBit >> 3; // divide by 8
	assert (uiByteNumber < m_uiNumBytes);
	unsigned char uc = m_pucData[uiByteNumber];
	unsigned int uiBitSet = uc & (1 << (uiBit & 7));
	return uiBitSet;
}

inline bool LBitField_Dynamic_IT::CheckAndSet(unsigned int uiBit)
{
	assert (m_pucData);
	unsigned int uiByteNumber = uiBit >> 3; // divide by 8
	assert (uiByteNumber < m_uiNumBytes);
	unsigned char &uc = m_pucData[uiByteNumber];
	unsigned int uiMask = (1 << (uiBit & 7));
	unsigned int uiBitSet = uc & uiMask;
	if (uiBitSet)
		return false;

	// set
	uc = uc | uiMask;
	return true;
}


inline void LBitField_Dynamic_IT::SetBit(unsigned int uiBit, unsigned int bSet)
{
	assert (m_pucData);
	unsigned int uiByteNumber = uiBit >> 3; // divide by 8
	assert (uiByteNumber < m_uiNumBytes);
	unsigned char uc = m_pucData[uiByteNumber];
	unsigned int uiMask = 1 << (uiBit & 7);
	if (bSet)
	{
		uc = uc | uiMask;
	}
	else
	{
		uc &= ~uiMask;
	}
	m_pucData[uiByteNumber] = uc;
}

} // namespace end
