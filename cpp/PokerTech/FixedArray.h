#pragma once
#include <assert.h>

// Very simple fixed sized array with a templated size. This is to avoid std::vector allocations
template <typename T, int SIZE>
struct FixedArray
{
	FixedArray() : m_Count(0)
	{
	}
	int m_Count;

	T m_Data[SIZE];

	T &operator[](int index)
	{
		return m_Data[index];
	}
	void Initialize(const T *ptr, int size)
	{
		for (int i = 0; i < size; i++)
			push_back(*(ptr++));
	}
	int size() { return m_Count; }
	void clear() { m_Count = 0; }
	void resize_uninitialized(int size)
	{
		assert(size <= SIZE);
		m_Count = size;
	}

	void push_back(const T &data)
	{
		assert(m_Count < SIZE);
		m_Data[m_Count++] = data;
	}
};