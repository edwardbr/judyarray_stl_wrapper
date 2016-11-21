#include <Judy.h>

#ifndef JUDY_ERROR
#define JUDY_ERROR(JudyFunc, JudyErrno, JudyErrID) JUDYERROR(__FILE__, __LINE__, JudyFunc, JudyErrno, JudyErrID);
#define JUDY_REPORT_ERROR(e) JUDYERROR(__FILE__, __LINE__, e, 0, 0)
#define JUDY_REPORT_TRACE(e, var1) JUDYERROR(__FILE__, __LINE__, e, var1, 0)
#endif

namespace xt
{
	template<typename T>
	class judy_string_map
	{
	public:
		typedef size_t size_type;
		typedef judy_string_map judy_map;

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
				if (first != NULL)
				{
					free(&const_cast<char*>(first)[-1]);
					first = NULL;
				}
			}
			void copy_first(const key_value& other)
			{
				assert(m_p_set == other.m_p_set);
				if (other.first == NULL) //no data to copy
				{
					clear_first();
				}
				else if (other.first[-1] == 0) //data is a valid judy array item
				{
					if (first == NULL)
					{
						first = ((char*)calloc(other.m_p_set->m_buf_size + 2, 1)) + 1;
					}
					else if (first[-1] == 1)
					{
						first = ((char*)realloc(&const_cast<char*>(first)[-1], other.m_p_set->m_buf_size + 2)) + 1;
						const_cast<char*>(first)[-1] = 0;
					}
					strcpy(const_cast<char*>(first), other.first);
				}
				else //data is not a valid judy array item
				{
					size_t len = strlen(other.first);
					if (first == NULL)
					{
						first = ((char*)malloc(len + 2)) + 1;
					}
					else if (first[-1] == 1)
					{
						first = ((const char*)realloc(&const_cast<char*>(first)[-1], len + 2)) + 1;
					}
					strcpy(const_cast<char*>(first), other.first);
					const_cast<char*>(first)[-1] = 1;
				}
			}

			void assign_first(judy_map* p_set, const char* text, bool is_end)
			{
				if (text == NULL) //no data to copy
				{
					assert(is_end == true);
					clear_first();
				}
				else
				{
					size_t len = 0;
					len = strlen(text);
					if (is_end == true && p_set != NULL)
					{
						len = p_set->m_buf_size;
					}
					if (first == NULL)
					{
						first = ((char*)malloc(len + 2)) + 1;
					}
					else if (first[-1] == 1)
					{
						first = ((char*)realloc(&const_cast<char*>(first)[-1], len + 2)) + 1;
					}
					strcpy(const_cast<char*>(first), text);
					const_cast<char*>(first)[-1] = is_end ? 1 : 0;
				}
			}

			bool at_end() const  { return first == NULL || first[-1] == 1; }
			
			key_value() : 
				second(second_val), 
				first(NULL), 
				m_buf_size(0),
				second_val(),
				m_p_set(NULL)
			{}

			explicit key_value(judy_map* p_set) : 
				second(second_val), 
				first(NULL), 
				m_buf_size(p_set->m_buf_size),
				second_val(),
				m_p_set(p_set)
			{}

			key_value(const key_value& other) :
				second(other.second), 
				first(NULL),
				m_buf_size(other.m_buf_size),
				second_val(other.second_val),
				m_p_set(other.m_p_set)
			{
				if (&other.second == &other.second_val)
				{
					//repoint second to its own second_val store
					set_value(&second_val);
				}
				copy_first(other);
			}

			key_value(key_value&& other) : 
				second(other.second),
				first(other.first),
				m_buf_size(other.m_buf_size),
				second_val(other.second_val),
				m_p_set(other.m_p_set)
			{
				other.first = NULL;
				if (&other.second == &other.second_val)
				{
					set_value(&second_val);
				}
			}

		protected:
			explicit key_value(const char* f, T& s, judy_map* p_set, bool) : 
				second(s), 
				first(NULL),
				m_buf_size(p_set->m_buf_size),
				second_val(),
				m_p_set(p_set)
			{
				assign_first(p_set, f, false);
			}

			explicit key_value(const char* f, T& s, bool) : 
				second(second_val),
				first(NULL),
				m_buf_size(0),
				second_val(s),
				m_p_set(NULL)
			{
				assign_first(NULL, f, false);
			}

		public:
			explicit key_value(const char* f, T s, judy_map* p_set) : 
				second(second_val), 
				first(NULL),
				m_buf_size(p_set->m_buf_size),
				second_val(s),
				m_p_set(p_set)
			{
				//repoint second to its own second_val store
				set_value(&second_val);
				assign_first(NULL, f, false);
			}

			explicit key_value(const char* f, T s) : 
				second(second_val), 
				first(NULL),
				m_buf_size(0),
				second_val(s),
				m_p_set(NULL)
			{
				assign_first(NULL, f, false);
			}

			~key_value()
			{
				clear_first();
			}
			/*void swap(key_value& other)
			{
				ASSERT(m_p_set);
				ASSERT(other.m_p_set);

				const char* f = other.first;
				other.first = first;
				first = f;

				T* st = get_value();
				T* s0 = other.get_value();
				*reinterpret_cast<pointer_type*>(&other) = *reinterpret_cast<pointer_type*>(this);
				//repoint second to its own second_val store
				*reinterpret_cast<pointer_type*>(this) = s;
				size_t set = other.m_p_set;
				other.m_p_set = m_p_set;
				m_p_set = set;
			}*/
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
				copy_first(other);
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
				other.first = NULL;
				return (*this);
			}

			bool operator == (const key_value& other) const
			{
				if (first == NULL && other.first == NULL)
				{
					return true;
				}
				if (first == NULL || other.first == NULL)
				{
					return false;
				}

				bool is_same = strcmp(first, other.first) == 0;

				return is_same;
			}

		public:
			const T& second; //this comes first so we can change it by dereferencing the this pointer
			const char* first;
		private:
			T second_val;
			size_t m_buf_size;
			judy_map* m_p_set;
			friend judy_map;
		};

		typedef key_value<T> value_type;
		typedef typename key_value<T>::pointer_type pointer_type;

		explicit judy_string_map() : m_p_judy_array(0), m_buf_size(1), m_count(0)
		{}
		judy_string_map(judy_map& other) : m_p_judy_array(0), m_buf_size(other.m_buf_size), m_count(other.m_count)
		{
			*this = other;
		}
		judy_map& operator = (const judy_map& other)
		{
			JError_t J_Error;
			m_count = other.m_count;
			m_buf_size = other.m_buf_size;
			if (m_p_judy_array != 0)
			{
				clear();
			}
			uint8_t* start = (uint8_t*)alloca(m_buf_size + 1);
			memset(start, 0, m_buf_size + 1);
			pointer_type ret = (pointer_type)JudySLFirst(other.m_p_judy_array, start, &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudySLFirst", J_Error.je_Errno, J_Error.je_ErrID);
			};
			while (ret)
			{
				pointer_type inner_ret = (pointer_type)JudySLIns(&m_p_judy_array, start, &J_Error);
				if (inner_ret == (pointer_type)(~0ULL))
				{
					JUDY_ERROR("JudySLIns", J_Error.je_Errno, J_Error.je_ErrID);
				}
				else if (*inner_ret != 0)
				{
					JUDY_REPORT_ERROR("BUG - bit already set");
					break;
				}
				value_type::copy_second(inner_ret, value_type::get_val_ref(ret));
				ret = (pointer_type)JudySLNext(other.m_p_judy_array, start, &J_Error);
				if (ret == (pointer_type)(~0ULL))
				{
					JUDY_ERROR("JudySLNext", J_Error.je_Errno, J_Error.je_ErrID);
				};
			}
			return *this;
		}
		~judy_string_map()
		{
			clear();
		}

		template<bool is_forward, bool is_const>
		class base_iterator
			: public boost::iterator_facade <
			base_iterator<is_forward, is_const>
			, std::string
			, boost::bidirectional_traversal_tag
			, std::string
			>,
			public xt::judy_string_map<T>::value_type
		{
		public:
			typedef typename xt::judy_string_map<T>::value_type value_type;
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
			explicit base_iterator(judy_map* pSet, const char* p, T& val, bool) :
				value_type(p, val, pSet, true)
			{ }
			explicit base_iterator(const char* p, T& val, bool) :
				value_type(p, val, true)
			{ }
			explicit base_iterator(judy_map* pSet, const char* p, T val) :
				value_type(p, val, pSet)
			{ }
			explicit base_iterator(const char* p, T val) :
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
					return;
				}
				JError_t J_Error;
				pointer_type ret = NULL;

				//here to ensure that we do not get a buffer overrun
				if (m_buf_size < m_p_set->m_buf_size)
				{
					m_buf_size = m_p_set->m_buf_size;
					char end_ch = first[-1];
					first = ((char*)realloc(&const_cast<char*>(first)[-1], m_buf_size + 2)) + 1;
					const_cast<char*>(first)[-1] = end_ch;
				}

				if (is_forward == true)
				{
					ret = (pointer_type)JudySLNext(m_p_set->m_p_judy_array, (uint8_t*)first, &J_Error);
				}
				else
				{
					ret = (pointer_type)JudySLPrev(m_p_set->m_p_judy_array, (uint8_t*)first, &J_Error);
				}
				if (ret == (pointer_type)(~0ULL))
				{
					JUDY_ERROR("JudySLNext", J_Error.je_Errno, J_Error.je_ErrID);
				};
				if (ret == 0)
				{
					clear_first();
					clear_holder(second_val);
				}
				else
				{
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

				//here to ensure that we do not get a buffer overrun
				if (m_buf_size < m_p_set->m_buf_size)
				{
					m_buf_size = m_p_set->m_buf_size;
					char end_ch = first[-1];
					first = ((char*)realloc(&const_cast<char*>(first)[-1], m_buf_size + 2)) + 1;
					const_cast<char*>(first)[-1] = end_ch;
				}

				if (is_forward == true)
				{
					ret = (pointer_type)JudySLPrev(m_p_set->m_p_judy_array, (uint8_t*)first, &J_Error);
				}
				else
				{
					ret = (pointer_type)JudySLNext(m_p_set->m_p_judy_array, (uint8_t*)first, &J_Error);
				}
				if (ret == (pointer_type)(~0ULL))
				{
					JUDY_ERROR("JudySLPrev", J_Error.je_Errno, J_Error.je_ErrID);
				};
				if (ret == 0)
				{
					clear_first();
					clear_holder(second_val);
				}
				else
				{
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
			char* key = (char*)alloca(m_buf_size + 1);
			memset(key, 0, m_buf_size + 1);
			pointer_type ret = (pointer_type)JudySLFirst(m_p_judy_array, (uint8_t*)key, &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudySLFirst", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
					throw std::bad_alloc();
				}
				return iterator(this, key, value_type::get_val_ref(ret), false);
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
			char* key = (char*)alloca(m_buf_size + 1);
			memset(key, 0, m_buf_size + 1);
			pointer_type ret = (pointer_type)JudySLFirst(m_p_judy_array, (uint8_t*)key, &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudySLFirst", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
					throw std::bad_alloc();
				}
				return const_iterator(const_cast<judy_string_map*>(this), key, value_type::get_val_ref(ret), false);
			}
			return const_iterator(const_cast<judy_string_map*>(this));
		}
		const_iterator end() const
		{
			return const_iterator(const_cast<judy_map*>(this));
		}
		reverse_iterator rbegin()
		{
			JError_t J_Error;
			char* key = (char*)alloca(m_buf_size + 1);
			key[m_buf_size] = 0;
			memset(key, -1, m_buf_size);
			pointer_type ret = (pointer_type)JudySLLast(m_p_judy_array, key, &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudySLFirst", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
				}
				return reverse_iterator(this, key, value_type::get_val_ref(ret), false);
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
			char* key = (char*)alloca(m_buf_size + 1);
			key[m_buf_size] = 0;
			memset(key, -1, m_buf_size);
			pointer_type ret = (pointer_type)JudySLLast(m_p_judy_array, key, &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudySLFirst", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
				}
				return const_reverse_iterator(this, key, ret, false);
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
		std::pair<iterator, bool> insert(const value_type& val)
		{
			return inner_insert(val.first, val.second);
		}
		std::pair<iterator, bool> insert(const std::string& x, const T& val)
		{
			return inner_insert(x, val);
		}
		std::pair<iterator, bool> insert(const char* x, const T& val)
		{
			return inner_insert(x, val);
		}
		std::pair<iterator, bool> insert(const std::string& x, T&& val)
		{
			return inner_insert(x, val);
		}
		std::pair<iterator, bool> insert(const char* x, T&& val)
		{
			return inner_insert(x, val);
		}
		std::pair<iterator, bool> inner_insert(const std::string& x, const T& val)
		{
			if (x.length() > m_buf_size)
			{
				m_buf_size = x.length();
			}
			JError_t J_Error;
			pointer_type ret = (pointer_type)JudySLIns(&m_p_judy_array, (uint8_t*)x.data(), &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudySLIns", J_Error.je_Errno, J_Error.je_ErrID);
			};
			bool inserted = false;
			if (*ret == 0)
			{
				value_type::copy_second(ret, const_cast<T&>(val));
				m_count++;
				inserted = true;
			}
			return std::pair<iterator, bool>(iterator(this, x.data(), value_type::get_val_ref(ret), true), inserted);
		}
		std::pair<iterator, bool> inner_insert(const char* x, const T& val)
		{
			if (strlen(x) > m_buf_size)
			{
				m_buf_size = strlen(x);
			}
			JError_t J_Error;
			pointer_type ret = (pointer_type)JudySLIns(&m_p_judy_array, (uint8_t*)x, &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudySLIns", J_Error.je_Errno, J_Error.je_ErrID);
			};
			bool inserted = false;
			if (*ret == 0)
			{
				value_type::copy_second(ret, const_cast<T&>(val));
				inserted = true;
				m_count++;
			}
			return std::pair<iterator, bool>(iterator(this, x, value_type::get_val_ref(ret), true), inserted);
		}

		size_type erase(const char* x)
		{
			JError_t J_Error;
			pointer_type found = (pointer_type)JudySLGet(m_p_judy_array, (const uint8_t *)x, &J_Error);
			if (found == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudySLGet", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (found != 0)
			{
				value_type::delete_holder(value_type::get_val_pointer(found));
			}
			else
			{
				return 0;
			}

			int ret = JudySLDel(&m_p_judy_array, (const uint8_t *)x, &J_Error);
			if (ret == -1)
			{
				JUDY_ERROR("JudySLDel", J_Error.je_Errno, J_Error.je_ErrID);
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
		size_type erase(const std::string& x)
		{
			return erase(x.data());
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
				uint8_t* start = (uint8_t*)alloca(m_buf_size + 1);
				memset(start, 0, m_buf_size + 1);
				JError_t J_Error;
				pointer_type ret = (pointer_type)JudySLFirst(m_p_judy_array, start, &J_Error);
				if (ret == (pointer_type)(~0ULL))
				{
					JUDY_ERROR("JudySLFirst", J_Error.je_Errno, J_Error.je_ErrID);
				};
				while (ret)
				{
					value_type::delete_holder(value_type::get_val_pointer(ret));

					ret = (pointer_type)JudySLNext(m_p_judy_array, start, &J_Error);
					if (ret == (pointer_type)(~0ULL))
					{
						JUDY_ERROR("JudySLNext", J_Error.je_Errno, J_Error.je_ErrID);
					};
				}
				
				//now dispose of the array
				Word_t Bytes = JudySLFreeArray(&m_p_judy_array, &J_Error);
				if (Bytes == (Word_t)-1)
				{
					JUDY_ERROR("JudySLFreeArray", J_Error.je_Errno, J_Error.je_ErrID);
				};
			};
			m_p_judy_array = 0;
			m_count = 0;
		}
		std::pair<const_iterator, const_iterator> equal_range(const std::string& x) const
		{
			const_iterator it = find_nearest(x);
			if (it->first == NULL)
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
		std::pair<iterator, iterator> equal_range(const std::string& x)
		{
			iterator it = find_nearest(x);
			iterator next = it;
			if (it->first != NULL && it->first == x)
			{
				++next;
			}
			return std::pair<iterator, iterator>(it, next);
		}
		size_type count(const std::string& x) const
		{
			return find(x) == end() ? 0 : 1;
		}
		iterator find(const std::string& x)
		{
			JError_t J_Error;
			if (x.length() > m_buf_size)
			{
				//string too long anyway
				return iterator(this);
			}
			pointer_type ret = (pointer_type)JudySLGet(m_p_judy_array, (const uint8_t *)x.data(), &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudySLGet", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
					throw std::bad_alloc();
				}
				return iterator(this, x.data(), value_type::get_val_ref(ret), false);
			}
			return iterator(this);
		}
		const_iterator find(const std::string& x) const
		{
			JError_t J_Error;
			if (x.length() > m_buf_size)
			{
				//string too long anyway
				return const_iterator(const_cast<judy_map*>(this));
			}
			pointer_type ret = (pointer_type)JudySLGet(m_p_judy_array, (const uint8_t *)x.data(), &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudySLGet", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
					throw std::bad_alloc();
				}
				return const_iterator(const_cast<judy_map*>(this), x.data(), value_type::get_val_ref(ret), true);
			}
			return const_iterator(const_cast<judy_map*>(this));
		}

		iterator find_nearest(const std::string& x)
		{
			JError_t J_Error;
			pointer_type ret = (pointer_type)JudySLFirst(m_p_judy_array, (uint8_t *)x.data(), &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudySLFirst", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
					throw std::bad_alloc();
				}
				return iterator(this, x.data(), value_type::get_val_ref(ret), true);
			}
			return iterator(this);
		}
		const_iterator find_nearest(const std::string& x) const
		{
			JError_t J_Error;
			pointer_type ret = (pointer_type)JudySLFirst(m_p_judy_array, (uint8_t*)x.data(), &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudySLFirst", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
					throw std::bad_alloc();
				}
				return const_iterator(const_cast<judy_map*>(this), x.data(), value_type::get_val_ref(ret), true);
			}
			return const_iterator(const_cast<judy_map*>(this));
		}
		iterator upper_bound(const std::string& x)
		{
			JError_t J_Error;
			pointer_type ret = (pointer_type)JudySLGet(m_p_judy_array, (const uint8_t *)x.data(), &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudySLGet", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
					throw std::bad_alloc();
				}
				return ++iterator(this, x.data(), value_type::get_val_ref(ret), false);
			}
			return iterator(this);
		}
		iterator lower_bound(const std::string& x)
		{
			JError_t J_Error;
			pointer_type ret = (pointer_type)JudySLGet(m_p_judy_array, (const uint8_t *)x.data(), &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudySLGet", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
					throw std::bad_alloc();
				}
				return --iterator(this, x.data(), value_type::get_val_ref(ret), false);
			}
			return iterator(this);
		}
		const_iterator upper_bound(const std::string& x) const
		{
			JError_t J_Error;
			pointer_type ret = (pointer_type)JudySLGet(m_p_judy_array, (const uint8_t *)x.data(), &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudySLGet", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
					throw std::bad_alloc();
				}
				return ++const_iterator(const_cast<judy_map*>(this), x.data(), value_type::get_val_ref(ret), false);
			}
			return const_iterator(const_cast<judy_map*>(this));

		}
		const_iterator lower_bound(const std::string& x) const
		{
			JError_t J_Error;
			pointer_type ret = (pointer_type)JudySLGet(m_p_judy_array, (const uint8_t *)x.data(), &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudySLGet", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret != 0)
			{
				if (*ret == 0)
				{
					JUDY_REPORT_ERROR("value not assigned");
					throw std::bad_alloc();
				}
				return --const_iterator(const_cast<judy_map*>(this), x.data(), value_type::get_val_ref(ret), false);
			}
			return const_iterator(const_cast<judy_map*>(this));
		}
		T& operator[](const std::string& val)
		{
			JError_t J_Error;
			pointer_type ret = (pointer_type)JudySLGet(m_p_judy_array, (const uint8_t *)val.data(), &J_Error);
			if (ret == (pointer_type)(~0ULL))
			{
				JUDY_ERROR("JudySLGet", J_Error.je_Errno, J_Error.je_ErrID);
			};
			if (ret == 0)
			{
				ret = (pointer_type)JudySLIns(&m_p_judy_array, (uint8_t*)val.data(), &J_Error);
				if (ret == (pointer_type)(~0ULL))
				{
					JUDY_ERROR("JudySLIns", J_Error.je_Errno, J_Error.je_ErrID);
				};
				value_type::instanciate_second(ret);
			}
			return value_type::get_val_ref(ret);
		};
	private:
		void set_buf_size(size_t size)
		{
			m_buf_size = size;
		}
		size_type get_buf_size() const
		{
			return m_buf_size;
		}
		
		void* m_p_judy_array;
		size_type m_buf_size; //this count does not include the null terminator
		size_t m_count;
	};
}

