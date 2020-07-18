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


// just a light wrapper around a vector until we get the Godot vector allocation issues sorted
#include "core/vector.h"
#include <assert.h>
#include <vector>

template <class T> class LVector
{
public:

	// array subscript access
	// note this is not available in Godot Vector
	T& operator[](unsigned int ui)
	{
#ifdef DEBUG_ENABLED
		assert (ui < (unsigned int) m_iSize);
#endif
		return m_Vec[ui];
	}

	const T& operator[](unsigned int ui) const
	{
#ifdef DEBUG_ENABLED
		assert (ui < (unsigned int) m_iSize);
#endif
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
		m_iSize = 0;
	}

	void resize(int s, bool bCompact = false)
	{
		// new size
		m_iSize = s;

		// if compacting is not desired, no need to shrink vector
		if (m_iSize < (int) m_Vec.size())
		{
			if (!bCompact)
			{
				return;
			}
		}

		m_Vec.resize(s);
	}

	void set(unsigned int ui, const T &t)
	{
#ifdef DEBUG_ENABLED
		assert (ui < (unsigned int) m_iSize);
#endif
		m_Vec[ui] = t;
	}

	// efficient unsorted
	void remove_unsorted(unsigned int ui)
	{
		// just swap the end element and decrement count
		m_Vec[ui] = m_Vec[m_iSize-1];
		m_iSize--;
	}

	void remove_last()
	{
		if (m_iSize)
			m_iSize--;
	}

	T * request()
	{
		m_iSize++;
		if (m_iSize >=(int) m_Vec.size())
			grow();

		return &m_Vec[m_iSize-1];
	}

	void grow()
	{
		int new_size = m_Vec.size() * 2;
		if (!new_size) new_size = 1;

		int s = m_iSize;
		resize(new_size);
		m_iSize = s;
	}


	void push_back(const T &t)
	{
		int size_p1 = m_iSize+1;

		if (size_p1 < (int) m_Vec.size())
		{
			int size = m_iSize;
			m_iSize = size_p1;
			set(size,  t);
		}
		else
		{
			// need more space
			grow();

			// call recursive
			push_back(t);
		}
	}

	void copy_from(const LVector<T> &o)
	{
		// make sure enough space
		if (o.size() > (int) m_Vec.size())
		{
			resize(o.size());
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
		if (o.size() > (int) m_Vec.size())
		{
			resize(o.size());
		}

		clear();
		m_iSize = o.size();

		for (int n=0; n<o.size(); n++)
		{
			set(n, o[n]);
		}
	}

	void insert(int i, const T &val)
	{
		m_Vec.insert(m_Vec.begin() + i, val);
		m_iSize++;
	}

	int find(const T &val)
	{
		for (int n=0; n<size(); n++)
		{
			if (m_Vec[n] == val)
				return n;
		}

		return -1; // not found
	}

	void delete_items_first(unsigned int uiNumItems)
	{
		if (uiNumItems < size())
		{
			unsigned int num_to_move = size() - uiNumItems;

			if (num_to_move)
			{
				memmove(&m_Vec[0], &m_Vec[uiNumItems], num_to_move * sizeof (T));
			}
			m_iSize -= uiNumItems;
		}
		else
		{
			if (uiNumItems == size())
			{
				clear();
			}
			else
			{
				assert (0 && "delete_items_first : Not enough items");
			}
		}

	}


	LVector()
	{
		m_iSize = 0;
	}


	int size() const {return m_iSize;}

private:
	std::vector<T> m_Vec;

	// working size
	int m_iSize;
};

