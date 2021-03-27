#pragma once
#include <ntddk.h>

/*
 * Represent traits for valid function deleter type.
 * This type invoked whenever the release function is invoked, in cases like: destructor, reset(), release().
 * @note You can override the default logic by creating your own type inheriting this PoolTraits type.
 */
struct PoolTraits {
	void operator()(void* p) const
	{
		ExFreePool(p);
	}
};

template<typename T = void, typename Deleter = PoolTraits>
struct UniquePtr
{
	explicit UniquePtr(T* object = nullptr)
		: m_data(object)
	{
	}

	~UniquePtr()
	{
		this->release();
	}

	// Delete: copy constructor, assignment operator, move constructor, move operator:
	UniquePtr(const UniquePtr& other) = delete;
	UniquePtr& operator=(const UniquePtr& other) = delete;
	UniquePtr(UniquePtr&& other) = delete;
	UniquePtr& operator=(UniquePtr&& other) = delete;

	/*
	 * Return pointer to the data.
	 */
	operator T* ()
	{
		return this->m_data;
	}

	/*
	 * Check if there is any data.
	 */
	operator bool() const
	{
		return nullptr != this->m_data;
	}

	/*
	 * Set this type to hold new data and release the previous one.
	 */
	void reset(T& p)
	{
		NT_ASSERT(nullptr == this->m_data);

		this->release();
		this->m_data = p;
	}

	/*
	 * Release the data.
	 */
	void release()
	{
		if (nullptr != this->m_data)
		{
			Deleter(this->m_data);
			this->m_data = nullptr;
		}
	}

private:
	T* m_data;
};