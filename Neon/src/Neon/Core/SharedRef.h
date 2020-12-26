#pragma once

// Intrinsic ref counting system
namespace Neon
{
	class RefCounted
	{
	public:
		void IncRefCount() const
		{
			m_RefCount++;
		}
		void DecRefCount() const
		{
			m_RefCount--;
		}

		uint32_t GetRefCount() const
		{
			return m_RefCount;
		}

	private:
		mutable uint32_t m_RefCount = 0;
	};

	template<typename T>
	class SharedRef
	{
	public:
		SharedRef()
			: m_Ptr(nullptr)
		{
		}

		SharedRef(std::nullptr_t n)
			: m_Ptr(nullptr)
		{
		}

		SharedRef(T* ptr)
			: m_Ptr(ptr)
		{
			static_assert(std::is_base_of<RefCounted, T>::value, "Class is not RefCounted!");
			IncRef();
		}

		template<typename T2>
		SharedRef(const SharedRef<T2>& other)
		{
			m_Ptr = (T*)other.m_Ptr;
			IncRef();
		}

		template<typename T2>
		SharedRef(SharedRef<T2>&& other)
		{
			m_Ptr = (T*)other.m_Ptr;
			other.m_Ptr = nullptr;
		}

		~SharedRef()
		{
			DecRef();
		}

		SharedRef(const SharedRef<T>& other)
			: m_Ptr(other.m_Ptr)
		{
			IncRef();
		}

		SharedRef& operator=(std::nullptr_t)
		{
			DecRef();
			m_Ptr = nullptr;
			return *this;
		}

		SharedRef& operator=(const SharedRef<T>& other)
		{
			other.IncRef();
			DecRef();

			m_Ptr = other.m_Ptr;
			return *this;
		}

		template<typename T2>
		SharedRef& operator=(const SharedRef<T2>& other)
		{
			other.IncRef();
			DecRef();

			m_Ptr = other.m_Ptr;
			return *this;
		}

		template<typename T2>
		SharedRef& operator=(SharedRef<T2>&& other)
		{
			DecRef();

			m_Ptr = other.m_Ptr;
			other.m_Ptr = nullptr;
			return *this;
		}

		operator bool()
		{
			return m_Ptr != nullptr;
		}
		operator bool() const
		{
			return m_Ptr != nullptr;
		}

		T* operator->()
		{
			return m_Ptr;
		}
		const T* operator->() const
		{
			return m_Ptr;
		}

		T& operator*()
		{
			return *m_Ptr;
		}
		const T& operator*() const
		{
			return *m_Ptr;
		}

		T* Raw()
		{
			return m_Ptr;
		}
		const T* Raw() const
		{
			return m_Ptr;
		}

		void Reset(T* ptr = nullptr)
		{
			DecRef();
			m_Ptr = ptr;
		}

		template<typename T2>
		SharedRef<T2> As()
		{
			return SharedRef<T2>(*this);
		}

		template<typename... Args>
		static SharedRef<T> Create(Args&&... args)
		{
			return SharedRef<T>(new T(std::forward<Args>(args)...));
		}

		template<class T2>
		friend class SharedRef;

	private:
		void IncRef() const
		{
			if (m_Ptr)
			{
				m_Ptr->IncRefCount();
			}	
		}

		void DecRef() const
		{
			if (m_Ptr)
			{
				m_Ptr->DecRefCount();
				if (m_Ptr->GetRefCount() == 0)
				{
					delete m_Ptr;
				}
			}
		}

		T* m_Ptr;
	};
} // namespace Neon
