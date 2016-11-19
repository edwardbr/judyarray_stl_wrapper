#include <Judy.h>

#ifndef JUDY_ERROR
#define JUDY_ERROR(JudyFunc, JudyErrno, JudyErrID) JUDYERROR(__FILE__, __LINE__, JudyFunc, JudyErrno, JudyErrID);
#define JUDY_REPORT_ERROR(e) report_error(__FILE__, __LINE__, e)
#define JUDY_REPORT_TRACE(e, var1) report_trace(__FILE__, __LINE__, e, var1)
#endif

namespace xt
{
	template<typename T>
	class judy_int_set
	{
	public:
		typedef size_t size_type;

		struct key_value
		{
		public:
			void clear_first()
			{
				first = 0;
				m_at_end = true;
			}

			bool at_end() const  { return m_at_end; }

			key_value() :
				first(NULL),
				m_p_set(NULL),
				m_at_end(true)
			{}

			explicit key_value(judy_int_set* p_set) :
				first(NULL),
				m_p_set(p_set),
				m_at_end(true)
			{}

			key_value(const key_value& other) :
				first(other.first),
				m_p_set(other.m_p_set),
				m_at_end(other.m_at_end)
			{}

			key_value(key_value&& other) :
				first(other.first),
				m_p_set(other.m_p_set),
				m_at_end(other.m_at_end)
			{}

		protected:
			explicit key_value(T val, judy_int_set* p_set, bool) :
				first(val),
				m_p_set(p_set),
				m_at_end(false)
			{}

			explicit key_value(T val, bool) :
				first(val),
				m_p_set(NULL),
				m_at_end(true)
			{}

		public:
			explicit key_value(T val, judy_int_set* p_set) :
				first(val),
				m_p_set(p_set),
				m_at_end(false)
			{}

			explicit key_value(T val) :
				first(val),
				m_p_set(NULL),
				m_at_end(true)
			{}

			~key_value()
			{
				clear_first();
			}
			key_value& operator=(const key_value& other)
			{
				m_p_set = other.m_p_set;
				first = other.first;
				m_at_end = other.m_at_end;
				return (*this);
			}
			key_value& operator=(key_value&& other)
			{
				m_p_set = other.m_p_set;
				first = other.first;
				m_at_end = other.m_at_end;
				return (*this);
			}

			bool operator == (const key_value& other) const
			{
				if (m_p_set != NULL && m_at_end == true && other.m_p_set != NULL && other.m_at_end == true)
				{
					return true;
				}
				if ((m_p_set != NULL && m_at_end == true) || (other.m_p_set != NULL && other.m_at_end == true))
				{
					return false;
				}
				return first == other.first;
			}

		public:
			T first;
		private:
			judy_int_set* m_p_set;
			bool m_at_end;
			friend judy_int_set;
		};

		typedef key_value value_type;

		explicit judy_int_set() : m_p_judy_array(0)
		{}
		judy_int_set(const judy_int_set& other) : m_p_judy_array(0)
		{
			*this = other;
		}
		judy_int_set& operator = (const judy_int_set& other)
		{
			JError_t J_Error;
			if (m_p_judy_array != NULL)
			{
				Word_t Bytes = Judy1FreeArray(&m_p_judy_array, &J_Error);
				if (Bytes == -1)
				{
					JUDY_ERROR("Judy1FreeArray", J_Error.je_Errno, J_Error.je_ErrID);
				};
				m_p_judy_array = NULL;
			}
			Word_t start = 0;
			int found = Judy1First(other.m_p_judy_array, &start, &J_Error);
			if (found == -1)
			{
				JUDY_ERROR("Judy1First", J_Error.je_Errno, J_Error.je_ErrID);
			};
			while (found)
			{
				found = Judy1Set(&m_p_judy_array, start, &J_Error);
				if (found == -1)
				{
					JUDY_ERROR("Judy1Set", J_Error.je_Errno, J_Error.je_ErrID);
				}
				else if (found == 0)
				{
					JUDY_REPORT_ERROR("BUG - bit already set");
					break;
				}
				else if (found != 1)
				{
					JUDY_REPORT_ERROR("unexpected value");
					break;
				}
				found = Judy1Next(other.m_p_judy_array, &start, &J_Error);
				if (found == -1)
				{
					JUDY_ERROR("Judy1Next", J_Error.je_Errno, J_Error.je_ErrID);
				};
			};
			return *this;
		}
		~judy_int_set()
		{
			clear();
		}

		template<bool is_forward, bool is_const>
		class base_iterator : 
			public boost::iterator_facade <
				base_iterator<
					is_forward, 
					is_const>, 
				Word_t, 
				boost::bidirectional_traversal_tag, 
				Word_t>,
				protected xt::judy_int_set<T>::value_type
		{
		public:
			typedef typename xt::judy_int_set<T>::value_type value_type;
			typedef typename std::conditional<is_const, const value_type, value_type>::type ret_val;

			explicit base_iterator()
			{ }
			explicit base_iterator(const value_type& other) :
				value_type(other)
			{}
			explicit base_iterator(value_type&& other) :
				value_type(std::move(other))
			{}

		private:
			explicit base_iterator(judy_int_set* pSet, T& p, bool) :
				value_type(p, pSet, true)
			{ }
			explicit base_iterator(T& p, bool) :
				value_type(p, true)
			{ }
			explicit base_iterator(judy_int_set* pSet, T& p) :
				value_type(p, pSet)
			{ }
			explicit base_iterator(T& p) :
				value_type(p)
			{ }
			base_iterator(judy_int_set* pSet) : value_type(pSet) {}

		public:
			base_iterator& operator = (value_type& other)
			{
				(value_type&)(*this) = other;
				return *this;
			}
			base_iterator& operator = (value_type&& other)
			{
				((value_type&)*this) = std::move((value_type&&)other);
				return *this;
			}
			template<bool X>
			base_iterator& operator = (base_iterator<X, false>& other)
			{
				(value_type&)(*this) = other;
				return *this;
			}
			template<bool X>
			base_iterator& operator = (base_iterator<X, false>&& other)
			{
				((value_type&)*this) = std::move((value_type&&)other);
				return *this;
			}

			T operator *() const { return first; }

			template<bool X, bool Y>
			bool operator == (const base_iterator<X, Y>& other) const
			{
				const value_type& vthis = *this;
				return vthis == other;
			}

		private:
			void increment()
			{
				if (at_end() == true)
				{
					JUDY_REPORT_ERROR("incrementing an iterator at the end");
				}
				JError_t J_Error;
				int ret = NULL;
				Word_t val = reinterpret_cast<Word_t>(first);
				if (is_forward == true)
				{
					ret = Judy1Next(m_p_set->m_p_judy_array, &val, &J_Error);
					if (ret == -1)
					{
						clear_first();
						JUDY_ERROR("Judy1Next", J_Error.je_Errno, J_Error.je_ErrID);
					}
				}
				else
				{
					ret = Judy1Prev(m_p_set->m_p_judy_array, &val, &J_Error);
					if (ret == -1)
					{
						clear_first();
						JUDY_ERROR("Judy1Prev", J_Error.je_Errno, J_Error.je_ErrID);
					};
				}
				if (ret == 0)
				{
					//we are beyond the end of the collection set end state
					clear_first();
				}
				else
				{
					first = (T)val;
				}
			}
			void decrement()
			{
				if (at_end() == true)
				{
					JUDY_REPORT_ERROR("decrementing an iterator at the end");
				}
				JError_t J_Error;
				int ret = NULL;
				Word_t val = reinterpret_cast<Word_t>(first);
				if (is_forward == true)
				{
					ret = Judy1Prev(m_p_set->m_p_judy_array, &val, &J_Error);
					if (ret == -1)
					{
						clear_first();
						JUDY_ERROR("Judy1Prev", J_Error.je_Errno, J_Error.je_ErrID);
					}
					else if (ret == 0)
					{
						JUDY_REPORT_ERROR("iterating before the start");
					}
				}
				else
				{
					ret = Judy1Next(m_p_set->m_p_judy_array, &val, &J_Error);
					if (ret == -1)
					{
						clear_first();
						JUDY_ERROR("Judy1Next", J_Error.je_Errno, J_Error.je_ErrID);
					}
					else if (ret == 0)
					{
						JUDY_REPORT_ERROR("iterating before the start");
					}
				}
				first = (T)val;
			}
			bool equal(base_iterator const& other) const
			{
				if (at_end() == true && other.at_end() == true)
				{
					return true;
				}
				else if (at_end() == true || other.at_end() == true)
				{
					return false;
				}
				return (value_type&)(*this) == (value_type&)(other);
			}
			ret_val& dereference(){ return *this; }
			friend class boost::iterator_core_access;
			friend class judy_int_set;
		};

		typedef base_iterator<true, false> iterator;
		typedef base_iterator<true, true> const_iterator;
		typedef base_iterator<false, false> reverse_iterator;
		typedef base_iterator<false, true> const_reverse_iterator;

		iterator begin()
		{
			JError_t J_Error;
			Word_t start = 0;
			int found = Judy1First(m_p_judy_array, &start, &J_Error);
			if (found == -1)
			{
				JUDY_ERROR("Judy1First", J_Error.je_Errno, J_Error.je_ErrID);
			}
			else if (found != 0)
			{
				Word_t& wval = start;
				T& val = reinterpret_cast<T&>(wval);
				return iterator(this, val, false);
			}
			return iterator(this);
		}
		iterator end()
		{
			return iterator(this);
		}
		const_iterator begin() const
		{
			JError_t J_Error;
			Word_t start = 0;
			int found = Judy1First(m_p_judy_array, &start, &J_Error);
			if (found == -1)
			{
				JUDY_ERROR("Judy1First", J_Error.je_Errno, J_Error.je_ErrID);
			}
			else if (found != 0)
			{
				Word_t& wval = start;
				T& val = reinterpret_cast<T&>(wval);
				return const_iterator(const_cast<judy_int_set*>(this), val, false);
			}
			return const_iterator(const_cast<judy_int_set*>(this));
		}
		const_iterator end() const
		{
			return const_iterator(const_cast<judy_int_set*>(this));
		}
		reverse_iterator rbegin()
		{
			JError_t J_Error;
			Word_t start = -1;
			int found = Judy1Last(m_p_judy_array, &start, &J_Error);
			if (found == -1)
			{
				JUDY_ERROR("Judy1Last", J_Error.je_Errno, J_Error.je_ErrID);
			}
			else if (found != 0)
			{
				return reverse_iterator(this, reinterpret_cast<T>(start), false);
			}
			return reverse_iterator(this);
		}
		reverse_iterator rend()
		{
			return reverse_iterator(this);
		}
		const_reverse_iterator rbegin() const
		{
			JError_t J_Error;
			Word_t start = -1;
			int found = Judy1Last(m_p_judy_array, &start, &J_Error);
			if (found == -1)
			{
				JUDY_ERROR("Judy1Last", J_Error.je_Errno, J_Error.je_ErrID);
			}
			else if (found != 0)
			{
				Word_t& wval = start;
				T& val = reinterpret_cast<T&>(wval);
				return const_reverse_iterator(const_cast<judy_int_set*>(this), val, false);
			}
			return const_reverse_iterator(const_cast<judy_int_set*>(this));
		}
		const_reverse_iterator rend() const
		{
			return const_reverse_iterator(const_cast<judy_int_set*>(this));
		}
		bool empty() const
		{
			return m_p_judy_array == NULL;
		}
		size_type size() const
		{
			JError_t error;
			Word_t count = Judy1Count(m_p_judy_array, 0, -1, &error);
			if (count == 0 && ((&error)->je_Errno) != JU_ERRNO_NONE)
			{
				if (((&error)->je_Errno) == JU_ERRNO_FULL)
				{
					JUDY_REPORT_ERROR("Judy1 array population == 2^32");
				}
				else if (((&error)->je_Errno) == JU_ERRNO_NULLPPARRAY)
				{
					JUDY_REPORT_ERROR("Judy array is null");
				}
				else if (((&error)->je_Errno) > JU_ERRNO_NFMAX)
				{
					JUDY_REPORT_ERROR("Judy array is corrupt");
				}
			}
			return count;
		}

		template<bool X, bool Y>
		std::pair<iterator, bool> insert(base_iterator<X,Y>& val)
		{
			return insert(val.first);
		}
		std::pair<iterator, bool> insert(T key)
		{
			JError_t J_Error;
			Word_t start = reinterpret_cast<Word_t>(key);
			int ret = Judy1Set(&m_p_judy_array, start, &J_Error);
			if (ret == -1)
			{
				JUDY_ERROR("Judy1Set", J_Error.je_Errno, J_Error.je_ErrID);
			}
			return std::pair<iterator, bool>(iterator(this, key, true), ret != 0);
		}

		template<bool X, bool Y>
		void insert(base_iterator<X, Y>& begin, base_iterator<X, Y>& end)
		{
			JError_t J_Error;
			base_iterator<X, Y> sit(begin);
			for (; sit != end;sit++)
			{
				Word_t start = reinterpret_cast<Word_t>(sit.first);
				int ret = Judy1Set(&m_p_judy_array, start, &J_Error);
				if (ret == -1)
				{
					JUDY_ERROR("Judy1Set", J_Error.je_Errno, J_Error.je_ErrID);
				}
			}
		}

		size_type erase(T key)
		{
			JError_t J_Error;
			Word_t start = reinterpret_cast<Word_t>(key);
			int ret = Judy1Unset(&m_p_judy_array, start, &J_Error);
			if (ret == -1)
			{
				JUDY_ERROR("Judy1UnSet", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				return 1;
			}
			else
			{
				return 0;
			}
		}
		template<bool X, bool Y>
		size_type erase(base_iterator<X,Y>& x)
		{
			return erase(*x);
		}
		void clear()
		{
			if (m_p_judy_array != 0)
			{
				JError_t J_Error;
				Word_t Rc_int = Judy1FreeArray(&m_p_judy_array, &J_Error);
				if (Rc_int == -1)
				{
					JUDY_ERROR("Judy1FreeArray", J_Error.je_Errno, J_Error.je_ErrID);
				};
			};
			m_p_judy_array = 0;
		}
		std::pair<const_iterator, const_iterator> equal_range(T start) const
		{
			const_iterator it = find_nearest(start);
			if (it.m_at_end == true)
			{
				return std::pair<const_iterator, const_iterator>(it, it);
			}
			const_iterator next = it;
			if (*it == start)
			{
				++next;
			}
			return std::pair<const_iterator, const_iterator>(it, next);
		}
		std::pair<iterator, iterator> equal_range(T start)
		{
			iterator it = find_nearest(start);
			iterator next = it;
			if (it.m_at_end != true)
			{
				++next;
			}
			return std::pair<iterator, iterator>(it, next);
		}
		size_type count(T start) const
		{
			return find(start) == end() ? 0 : 1;
		}
		iterator find(const T& start)
		{
			JError_t J_Error;
			int ret = Judy1Test(m_p_judy_array, start, &J_Error);
			if (ret == -1)
			{
				JUDY_ERROR("Judy1Test", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				return iterator(this, start, false);
			}
			return iterator(this);
		}
		const_iterator find(const T& key) const
		{
			JError_t J_Error;
			Word_t start = reinterpret_cast<Word_t>(key);
			int ret = Judy1Test(m_p_judy_array, start, &J_Error);
			if (ret == -1)
			{
				JUDY_ERROR("Judy1Test", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				Word_t& wval = start;
				T& val = reinterpret_cast<T&>(start);
				return const_iterator(const_cast<judy_int_set*>(this), val, false);
			}
			return const_iterator(const_cast<judy_int_set*>(this));
		}

		iterator find_nearest(T start)
		{
			JError_t J_Error;
			int ret = Judy1First(m_p_judy_array, &start, &J_Error);
			if (ret == -1)
			{
				JUDY_ERROR("JudyLFirst", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				Word_t& wval = start;
				T& val = reinterpret_cast<T&>(wval);
				return iterator(this, val, false);
			}
			return iterator(this);
		}
		const_iterator find_nearest(T start) const
		{
			JError_t J_Error;
			int ret = Judy1First(m_p_judy_array, &start, &J_Error);
			if (ret == -1)
			{
				JUDY_ERROR("JudyLFirst", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				Word_t& wval = const_cast<Word_t>(start);
				T& val = reinterpret_cast<T&>(wval);
				return const_iterator(const_cast<judy_int_set*>(this), val, true);
			}
			return const_iterator(const_cast<judy_int_set*>(this));
		}
		iterator upper_bound(T start)
		{
			JError_t J_Error;
			int ret = Judy1First(m_p_judy_array, &start, &J_Error);
			if (ret == -1)
			{
				JUDY_ERROR("JudyLGet", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				Word_t& wval = start;
				T& val = reinterpret_cast<T&>(wval);
				return ++iterator(this, val, false);
			}
			return iterator(this);
		}
		iterator lower_bound(T start)
		{
			JError_t J_Error;
			int ret = Judy1First(m_p_judy_array, &start, &J_Error);
			if (ret == -1)
			{
				JUDY_ERROR("JudyLGet", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				Word_t& wval = start;
				T& val = reinterpret_cast<T&>(wval);
				return --iterator(this, val, false);
			}
			return iterator(this);
		}
		const_iterator upper_bound(T start) const
		{
			JError_t J_Error;
			int ret = Judy1First(m_p_judy_array, &start, &J_Error);
			if (ret == -1)
			{
				JUDY_ERROR("JudyLGet", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				Word_t& wval = start;
				T& val = reinterpret_cast<T&>(wval);
				return ++const_iterator(const_cast<judy_int_set*>(this), val, false);
			}
			return const_iterator(const_cast<judy_int_set*>(this));

		}
		const_iterator lower_bound(T start) const
		{
			JError_t J_Error;
			int ret = Judy1First(m_p_judy_array, &start, &J_Error);
			if (ret == -1)
			{
				JUDY_ERROR("JudyLGet", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				Word_t& wval = start;
				T& val = reinterpret_cast<T&>(wval);
				return --const_iterator(const_cast<judy_int_set*>(this), val, false);
			}
			return const_iterator(const_cast<judy_int_set*>(this));
		}
		
		void swap(judy_int_set& other)
		{
			void* p_judy_array = other.m_p_judy_array;
			other.m_p_judy_array = m_p_judy_array;
			m_p_judy_array = p_judy_array;
		}
	private:
		void* m_p_judy_array;
	};
}

