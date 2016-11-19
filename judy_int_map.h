#include <Judy.h>

#ifndef JUDY_ERROR
#define JUDY_ERROR(JudyFunc, JudyErrno, JudyErrID) JUDYERROR(__LINE__, __FILE__, JudyFunc, JudyErrno, JudyErrID);
#define JUDY_REPORT_ERROR(e) report_error(__LINE__, __FILE__, e)
#define JUDY_REPORT_TRACE(e, var1) report_trace(__LINE__, __FILE__, e, var1)
#endif

namespace xt
{
	template<typename S, typename T>
	class judy_int_map
	{
	public:
		typedef size_t size_type;
		typedef judy_int_map judy_map;

		//we can use inplace values rather than pointers
		template <typename T, typename bool IsPOD = std::is_pod<T>::value && (sizeof(T) <= sizeof(void*))>
		struct value_base
		{
			typedef T* pointer_type;
			static T& get_val_ref(pointer_type p_val){ return *p_val; }
			static T*& get_val_pointer(pointer_type& p_val){ return p_val; }
			static void instanciate_second(T& dest)
			{
				if (std::is_pod<T>::value == false)
				{
					new(&dest) T;
				}
				else
				{
					memset(&dest, 0, sizeof(T));
				}
			}
			static void copy_second(pointer_type& dest, T& other)
			{
				if (std::is_pod<T>::value == false)
				{
					new(dest)T(other);
				}
				else
				{
					memcpy(dest, &other, sizeof(T));
				}
			}
			static void delete_holder(T*& pointer)
			{
				if (std::is_pod<T>::value == false)
				{
					pointer->~T();
				}
				memset(pointer, 0, sizeof(T));
				pointer = NULL;
			}
			void clear_holder(T& second_val)
			{
				//repoint second to its own second_val store
				if (std::is_pod<T>::value == false)
				{
					second_val = T();
				}
				else
				{
					memset(&second_val, 0, sizeof(second_val));
				}

				set_value(&second_val);
			}
			void set_value(T* val){ *reinterpret_cast<pointer_type*>(this) = val; }
		};

		//we can do malloc rather than in place pod subsitution
		template <typename T>
		struct value_base<T, false>
		{
		public:
			typedef T** pointer_type;
			static T& get_val_ref(pointer_type p_val){ return **p_val; }
			static T*& get_val_pointer(pointer_type p_val){ return *p_val; }
			static void instanciate_second(T*& dest)
			{
				if (std::is_pod<T>::value == false)
				{
					dest = new T();
				}
				else
				{
					dest = calloc(1, sizeof(T));
				}
			}
			static void copy_second(pointer_type& dest, T& other)
			{
				if (std::is_pod<T>::value == false)
				{
					*dest = new T(other);
				}
				else
				{
					*dest = (T*)calloc(1, sizeof(T));
				}
			}
			static void delete_holder(T*& pointer)
			{
				if (std::is_pod<T>::value == false)
				{
					delete pointer;
				}
				else
				{
					free(pointer);
				}
				pointer = NULL;
			}
			void clear_holder(T& second_val)
			{
				//repoint second to its own second_val store
				if (std::is_pod<T>::value == false)
				{
					second_val = T();
				}
				else
				{
					memset(&second_val, 0, sizeof(second_val));
				}
				set_value(&second_val);
			}
			void set_value(T* val)
			{
				*reinterpret_cast<pointer_type>(this) = val;
			}
		};

		template <typename T, typename bool IsPOD = std::is_pod<T>::value && (sizeof(T) <= sizeof(void*))>
		struct key_value : protected value_base<T, IsPOD>
		{
		public:
			void clear_first()
			{
				first = 0;
				m_at_end = true;
			}

			bool at_end() const  { return m_at_end; }
			
			key_value() : 
				second(second_val), 
				first(NULL), 
				second_val(),
				m_p_set(NULL),
				m_at_end(true)
			{}

			explicit key_value(judy_map* p_set) :
				second(second_val), 
				first(NULL), 
				second_val(),
				m_p_set(p_set),
				m_at_end(true)
			{}

			key_value(const key_value& other) :
				second(other.second), 
				first(other.first),
				second_val(other.second_val), 
				m_p_set(other.m_p_set),
				m_at_end(other.m_at_end)
			{
				if (&other.second == &other.second_val)
				{
					//repoint second to its own second_val store
					set_value(&second_val);
				}
			}

			key_value(key_value&& other) :
				second(other.second),
				first(other.first),
				second_val(other.second_val),
				m_p_set(other.m_p_set),
				m_at_end(other.m_at_end)
			{
				if (&other.second == &other.second_val)
				{
					set_value(&second_val);
				}
			}

		protected:
			explicit key_value(S val, T& s, judy_map* p_set, bool) :
				second(s), 
				first(val),
				second_val(),
				m_p_set(p_set),
				m_at_end(false)
			{}

			explicit key_value(S val, T& s, bool) :
				second(second_val),
				first(val),
				second_val(s),
				m_p_set(NULL),
				m_at_end(true)
			{}

		public:
			explicit key_value(S val, T s, judy_map* p_set) :
				second(second_val), 
				first(val),
				second_val(s),
				m_p_set(p_set),
				m_at_end(false)
			{
				//repoint second to its own second_val store
				set_value(&second_val);
			}

			explicit key_value(S val, T s) :
				second(second_val), 
				first(val),
				second_val(s),
				m_p_set(NULL),
				m_at_end(true)
			{}

			~key_value()
			{
				clear_first();
			}
			key_value& operator=(const key_value& other)
			{
				second_val = other.second_val;
				if (&other.second == &other.second_val)
				{
					set_value(const_cast<T*>(&other.second_val));
				}
				else
				{
					set_value(const_cast<T*>(&other.second));
				}

				m_p_set = other.m_p_set;
				first = other.first;
				m_at_end = other.m_at_end;
				return (*this);
			}
			key_value& operator=(key_value&& other)
			{
				if (&other.second == &other.second_val)
				{
					//repoint second to its own second_val store
					set_value(&second_val);
				}
				else
				{
					set_value(const_cast<T*>(&other.second));
				}
				second_val = other.second_val;
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
			T& second; //this comes first so we can change it by dereferencing the this pointer
			S first;
		private:
			T second_val;
			judy_map* m_p_set;
			bool m_at_end;
			friend judy_map;
		};

		typedef key_value<T> value_type;
		typedef typename key_value<T>::pointer_type pointer_type;

		explicit judy_int_map() : m_p_judy_array(0), m_count(0)
		{}
		judy_int_map(judy_map& other) : m_p_judy_array(0), m_count(other.m_count)
		{
			*this = other;
		}
		judy_map& operator = (const judy_map& other)
		{
			JError_t J_Error;
			m_count = other.m_count;
			if (m_p_judy_array != 0)
			{
				clear();
			}
			Word_t start = 0;
			pointer_type ret = (pointer_type)JudyLFirst(other.m_p_judy_array, &start, &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudyLFirst", J_Error.je_Errno, J_Error.je_ErrID);
			};
			while (ret)
			{
				pointer_type inner_ret = (pointer_type)JudyLIns(&m_p_judy_array, start, &J_Error);
				if (inner_ret == (pointer_type)(~0ULL))
				{
					JUDY_ERROR("JudyLIns", J_Error.je_Errno, J_Error.je_ErrID);
				}
				else if (*inner_ret != 0)
				{
					JUDY_REPORT_ERROR("BUG - bit already set");
					break;
				}
				value_type::copy_second(inner_ret, value_type::get_val_ref(ret));
				ret = (pointer_type)JudyLNext(other.m_p_judy_array, &start, &J_Error);
				if (ret == (pointer_type)(~0ULL))
				{
					JUDY_ERROR("JudyLNext", J_Error.je_Errno, J_Error.je_ErrID);
				};
			}
			return *this;
		}
		~judy_int_map()
		{
			clear();
		}

		template<bool is_forward, bool is_const>
		class base_iterator
			: public boost::iterator_facade <
			base_iterator<is_forward, is_const>
			, S
			, boost::bidirectional_traversal_tag
			, S
			>,
			protected xt::judy_int_map<S, T>::value_type
		{
		public:
			typedef typename xt::judy_int_map<S, T>::value_type value_type;
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
			explicit base_iterator(judy_map* pSet, S p, T& val, bool) :
				value_type(p, val, pSet, true)
			{ }
			explicit base_iterator(S p, T& val, bool) :
				value_type(p, val, true)
			{ }
			explicit base_iterator(judy_map* pSet, S p, T val) :
				value_type(p, val, pSet)
			{ }
			explicit base_iterator(S p, T val) :
				value_type(p, val)
			{ }
			base_iterator(judy_map* pSet) : value_type(pSet) {}

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

			ret_val* operator ->() const { return this; }
			ret_val& operator *() const { return *this; }
			ret_val* operator ->(){ return this; }
			ret_val& operator *(){ return *this; }
			
			template<bool X, bool Y>
			bool operator == (const base_iterator<X,Y>& other) const
			{
				const value_type& vthis = *this;
				return vthis == other;
			}
			
		private:
			void increment()
			{
				if (at_end() == true)
				{
					return;
				}
				JError_t J_Error;
				pointer_type ret = NULL;
				Word_t start = first;
				if (is_forward == true)
				{
					ret = (pointer_type)JudyLNext(m_p_set->m_p_judy_array, &start, &J_Error);
				}
				else
				{
					ret = (pointer_type)JudyLPrev(m_p_set->m_p_judy_array, &start, &J_Error);
				}
				if (ret == (pointer_type)(~0ULL))
				{
					JUDY_ERROR("JudyLNext", J_Error.je_Errno, J_Error.je_ErrID);
				};
				if (ret == 0)
				{
					clear_first();
					clear_holder(second_val);
				}
				else
				{
					first = (S)start;
					set_value(value_type::get_val_pointer(ret));
				}
			}
			void decrement()
			{
				if (at_end() == true)
				{
					return;
				}
				JError_t J_Error;
				pointer_type ret = NULL;
				Word_t start = first;
				if (is_forward == true)
				{
					ret = (pointer_type)JudyLPrev(m_p_set->m_p_judy_array, &start, &J_Error);
				}
				else
				{
					ret = (pointer_type)JudyLNext(m_p_set->m_p_judy_array, &start, &J_Error);
				}
				if (ret == (pointer_type)(~0ULL))
				{
					JUDY_ERROR("JudyLPrev", J_Error.je_Errno, J_Error.je_ErrID);
				};
				if (ret == 0)
				{
					clear_first();
					clear_holder(second_val);
				}
				else
				{
					first = (S)start;
					set_value(value_type::get_val_pointer(ret));
				}
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
			friend class judy_map;
		};

		typedef base_iterator<true, false> iterator;
		typedef base_iterator<true, true> const_iterator;
		typedef base_iterator<false, false> reverse_iterator;
		typedef base_iterator<false, true> const_reverse_iterator;

		iterator begin()
		{
			JError_t J_Error;
			Word_t start = 0;
			pointer_type ret = (pointer_type)JudyLFirst(m_p_judy_array, &start, &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudyLFirst", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
					throw std::bad_alloc();
				}
				return iterator(this, (S)start, value_type::get_val_ref(ret), false);
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
			pointer_type ret = (pointer_type)JudyLFirst(m_p_judy_array, &start, &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudyLFirst", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
					throw std::bad_alloc();
				}
				return const_iterator(this, start, ret, false);
			}
			return const_iterator(this);
		}
		const_iterator end() const
		{
			return const_iterator(const_cast<judy_map*>(this));
		}
		reverse_iterator rbegin()
		{
			JError_t J_Error;
			Word_t start = -1;
			pointer_type ret = (pointer_type)JudyLLast(m_p_judy_array, &start, &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudyLFirst", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
				}
				return reverse_iterator(this, start, ret, false);
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
			pointer_type ret = (pointer_type)JudyLLast(m_p_judy_array, &start, &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudyLFirst", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
				}
				return const_reverse_iterator(this, start, ret, false);
			}
			return const_reverse_iterator(this);
		}
		const_reverse_iterator rend() const
		{
			return const_reverse_iterator(this);
		}
		bool empty() const
		{
			return m_count == 0;
		}
		size_type size() const
		{
			return m_count;
		}
		std::pair<iterator, bool> insert(value_type& val)
		{
			return insert(val.first, const_cast<T&>(val.second));
		}
		std::pair<iterator, bool> insert(S key, T& val)
		{
			JError_t J_Error;
			pointer_type ret = (pointer_type)JudyLIns(&m_p_judy_array, key, &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudyLIns", J_Error.je_Errno, J_Error.je_ErrID);
			};
			bool inserted = false;
			if (*ret == 0)
			{
				value_type::copy_second(ret, val);
				m_count++;
				inserted = true;
			}
			iterator it(this, key, value_type::get_val_ref(ret), true);
			return std::pair<iterator, bool>(std::move(it), inserted);
		}
		
		size_type erase(S key)
		{
			JError_t J_Error;
			pointer_type found = (pointer_type)JudyLGet(m_p_judy_array, key, &J_Error);
			if (found == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudyLGet", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (found != 0)
			{
				value_type::delete_holder(value_type::get_val_pointer(found));
			}
			else
			{
				return 0;
			}

			int ret = JudyLDel(&m_p_judy_array, key, &J_Error);
			if (ret == -1)
			{
				JUDY_ERROR("JudyLDel", J_Error.je_Errno, J_Error.je_ErrID);
			}
			else if (ret == 0)
			{
				return 0;
			}
			else if (ret == 1)
			{
				m_count--;
				return 1;
			}
			JUDY_REPORT_ERROR("unexpected judy value");
			return 0;
		}
		size_type erase(iterator& x)
		{
			return erase(x->first);
		}
		size_type erase(const_iterator& x)
		{
			return erase(x->first);
		}
		void clear()
		{
			if (m_p_judy_array != 0)
			{
				Word_t start = 0;
				JError_t J_Error;
				pointer_type ret = (pointer_type)JudyLFirst(m_p_judy_array, &start, &J_Error);
				if (ret == (pointer_type)(~0ULL))
				{
					JUDY_ERROR("JudyLFirst", J_Error.je_Errno, J_Error.je_ErrID);
				};
				while (ret)
				{
					value_type::delete_holder(value_type::get_val_pointer(ret));

					ret = (pointer_type)JudyLNext(m_p_judy_array, &start, &J_Error);
					if (ret == (pointer_type)(~0ULL))
					{
						JUDY_ERROR("JudyLNext", J_Error.je_Errno, J_Error.je_ErrID);
					};
				}
				
				//now dispose of the array
				Word_t Bytes = JudyLFreeArray(&m_p_judy_array, &J_Error);
				if (Bytes == (Word_t)-1)
				{
					JUDY_ERROR("JudyLFreeArray", J_Error.je_Errno, J_Error.je_ErrID);
				};
			};
			m_p_judy_array = 0;
			m_count = 0;
		}
		std::pair<const_iterator, const_iterator> equal_range(S start) const
		{
			const_iterator it = find_nearest(x);
			if (it->m_at_end == true)
			{
				return std::pair<const_iterator, const_iterator>(it, it);
			}
			const_iterator next = it;
			if (it->first == x)
			{
				++next;
			}
			return std::pair<const_iterator, const_iterator>(it, next);
		}
		std::pair<iterator, iterator> equal_range(S start)
		{
			iterator it = find_nearest(start);
			iterator next = it;
			if (it->m_at_end != true)
			{
				++next;
			}
			return std::pair<iterator, iterator>(it, next);
		}
		size_type count(S start) const
		{
			return find(start) == end() ? 0 : 1;
		}
		iterator find(S start)
		{
			JError_t J_Error;
			pointer_type ret = (pointer_type)JudyLGet(m_p_judy_array, start, &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudyLGet", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
					throw std::bad_alloc();
				}
				return iterator(this, start, value_type::get_val_ref(ret), false);
			}
			return iterator(this);
		}
		const_iterator find(S start) const
		{
			JError_t J_Error;
			pointer_type ret = (pointer_type)JudyLGet(m_p_judy_array, start, &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudyLGet", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
					throw std::bad_alloc();
				}
				return const_iterator(const_cast<judy_map*>(this), start, value_type::get_val_ref(ret), true);
			}
			return const_iterator(const_cast<judy_map*>(this));
		}

		iterator find_nearest(S key)
		{
			JError_t J_Error;
			Word_t start = key;
			pointer_type ret = (pointer_type)JudyLFirst(m_p_judy_array, &start, &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudyLFirst", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
					throw std::bad_alloc();
				}
				return iterator(this, (S)start, value_type::get_val_ref(ret), false);
			}
			return iterator(this);
		}
		const_iterator find_nearest(S key) const
		{
			JError_t J_Error;
			Word_t start = key;
			pointer_type ret = (pointer_type)JudyLFirst(m_p_judy_array, &start, &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudyLFirst", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
					throw std::bad_alloc();
				}
				return const_iterator(const_cast<judy_map*>(this), (S)start, value_type::get_val_ref(ret), true);
			}
			return const_iterator(const_cast<judy_map*>(this));
		}
		iterator upper_bound(S key)
		{
			JError_t J_Error;
			Word_t start = key;
			pointer_type ret = (pointer_type)JudyLGet(m_p_judy_array, &start, &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudyLGet", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
					throw std::bad_alloc();
				}
				return ++iterator(this, (S)start, value_type::get_val_ref(ret), false);
			}
			return iterator(this);
		}
		iterator lower_bound(S key)
		{
			JError_t J_Error;
			Word_t start = key;
			pointer_type ret = (pointer_type)JudyLGet(m_p_judy_array, &start, &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudyLGet", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
					throw std::bad_alloc();
				}
				return --iterator(this, (S)start, value_type::get_val_ref(ret), false);
			}
			return iterator(this);
		}
		const_iterator upper_bound(S key) const
		{
			JError_t J_Error;
			Word_t start = key;
			pointer_type ret = (pointer_type)JudyLGet(m_p_judy_array, &start, &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudyLGet", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
					throw std::bad_alloc();
				}
				return ++const_iterator(const_cast<judy_map*>(this), (S)start, value_type::get_val_ref(ret), false);
			}
			return const_iterator(const_cast<judy_map*>(this));

		}
		const_iterator lower_bound(S key) const
		{
			JError_t J_Error;
			Word_t start = key;
			pointer_type ret = (pointer_type)JudyLGet(m_p_judy_array, &start, &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudyLGet", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
					throw std::bad_alloc();
				}
				return --const_iterator(const_cast<judy_map*>(this), (S)start, value_type::get_val_ref(ret), false);
			}
			return const_iterator(const_cast<judy_map*>(this));
		}
		T& operator[](S val)
		{
			JError_t J_Error;
			pointer_type ret = (pointer_type)JudyLGet(m_p_judy_array, val, &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudyLGet", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret == 0)
			{
				ret = (pointer_type)JudyLIns(&m_p_judy_array, val, &J_Error);
				if (ret == (pointer_type)(~0ULL))
				{
					JUDY_ERROR("JudyLIns", J_Error.je_Errno, J_Error.je_ErrID);
				};
				value_type::instanciate_second(*ret);
			}
			return value_type::get_val_ref(ret);
		};
	private:
		void* m_p_judy_array;
		size_t m_count;
	};
}

