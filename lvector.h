#pragma once

// just a light wrapper around the Godot vector until we get the allocation issues sorted
#include "core/vector.h"
#include <assert.h>

template <class T> class LVector
{
public:

	// array subscript access
	T& operator[](unsigned int ui)
	{
		assert (ui < m_iSize);
		return m_Vec[ui];
	}
	const T& operator[](unsigned int ui) const
	{
		assert (ui < m_iSize);
		return m_Vec[ui];
	}

	void clear(bool bCompact = false)
	{
		m_iSize = 0;
		if (bCompact)
			compact();
	}

	void compact()
	{
		m_Vec.resize(m_iSize);
	}

	void reserve(int s)
	{
		m_Vec.resize(s);
	}

	void set(unsigned int ui, const T &t)
	{
		assert (ui < m_iSize);
		m_Vec.set(ui, t);
	}

	void push_back(const T &t)
	{
		int size_p1 = m_iSize+1;

		if (size_p1 < m_Vec.size())
		{
			int size = m_iSize;
			m_iSize = size_p1;
			m_Vec.set(size, t);
		}
		else
		{
			// need more space
			reserve(m_Vec.size() * 2);

			// call recursive
			push_back(t);
		}
	}

	void copy_from(const LVector<T> &o)
	{
		// make sure enough space
		if (o.size() > m_Vec.size())
		{
			reserve(o.size());
		}

		clear();
		m_iSize = o.size();

		for (int n=0; n<o.size(); n++)
		{
			set(n, o[n]);
		}
	}

	void copy_from(const Vector<T> &o)
	{
		// make sure enough space
		if (o.size() > m_Vec.size())
		{
			reserve(o.size());
		}

		clear();
		m_iSize = o.size();

		for (int n=0; n<o.size(); n++)
		{
			set(n, o[n]);
		}
	}

	LVector()
	{
		m_iSize = 0;
	}

	int size() const {return m_iSize;}

private:
	Vector<T> m_Vec;

	// working size
	int m_iSize;
};

