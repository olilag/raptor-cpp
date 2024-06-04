
#ifndef ARRAY_HPP_
#define ARRAY_HPP_

#include <vector>
#include <iostream>
#include <cstdlib>
#include <initializer_list>
#include <exception>
#include <iterator>
#include <utility>
#include <type_traits>

namespace lib
{
	constexpr int DEFAULT_BLOCK_SIZE = 10;

	template<typename element>
	class Array
	{
	private:
		std::vector<element*> data_;
		size_t block_size_;
		size_t capacity_;
		size_t size_;
		void add_block();
		void remove_block();
		void clear_items();
		void add_items(const Array<element>& other);
	public:
		Array(size_t block_size = DEFAULT_BLOCK_SIZE);
		Array(std::initializer_list<element> items);
		Array(const Array<element>& other);
		Array(Array<element>&& other) noexcept;
		~Array() noexcept;
		size_t size() const;
		size_t capacity() const;
		void print(std::ostream& stream = std::cout) const;
		void push_back(const element& item);
		void push_back(element&& item);
		void pop_back();
		element& at(size_t index);
		const element& at(size_t index) const;
		element& operator[](size_t index);
		const element& operator[](size_t index) const;
		Array<element>& operator=(const Array<element>& other);
		Array<element>& operator=(Array<element>&& other) noexcept;
		template<bool constant>
		class iterator_base;
		using iterator = iterator_base<false>;
		using const_iterator = iterator_base<true>;
		iterator begin();
		iterator end();
		const_iterator begin() const;
		const_iterator end() const;
		const_iterator cbegin() const;
		const_iterator cend() const;
	};

	template<typename element>
	void Array<element>::clear_items()
	{
		while (size_ > 0)
		{
			pop_back();
		}
	}

	template<typename element>
	Array<element>::Array(size_t block_size) : block_size_(block_size), capacity_(0), size_(0) { }

	template<typename element>
	Array<element>::Array(const Array<element>& other) : block_size_(other.block_size_), capacity_(0), size_(0)
	{
		try
		{
			add_items(other);
		}
		catch (...)
		{
			clear_items();
			throw;
		}
	}

	template<typename element>
	Array<element>::Array(Array<element>&& other) noexcept : block_size_(DEFAULT_BLOCK_SIZE), capacity_(0), size_(0)
	{
		std::swap(block_size_, other.block_size_);
		std::swap(capacity_, other.capacity_);
		std::swap(size_, other.size_);
		std::swap(data_, other.data_);
	}

	template<typename element>
	inline size_t Array<element>::size() const
	{
		return size_;
	}

	template<typename element>
	inline size_t Array<element>::capacity() const
	{
		return capacity_;
	}

	template<typename element>
	void Array<element>::print(std::ostream& stream) const
	{
		stream << '[';
		bool first = true;
		for (size_t index = 0; index < size_; ++index)
		{
			if (!first)
			{
				stream << ", ";
			}
			stream << data_[index / block_size_][index % block_size_];
			first = false;
		}
		stream << ']';
	}

	template<typename element>
	std::ostream& operator<<(std::ostream& stream, const Array<element>& array)
	{
		array.print(stream);
		return stream;
	}

	template<typename element>
	void Array<element>::add_block()
	{
		element* block_ptr = (element*)std::malloc(sizeof(element) * block_size_);
		if (block_ptr == nullptr)
		{
			throw std::bad_alloc{};
		}
		try
		{
			data_.push_back(block_ptr);
		}
		catch (const std::bad_alloc&)
		{
			std::free(block_ptr);
			throw;
		}
		capacity_ += block_size_;
	}

	template<typename element>
	void Array<element>::remove_block()
	{
		element* block_ptr = data_.back();
		data_.pop_back();
		std::free(block_ptr);
		capacity_ -= block_size_;
	}

	template<typename element>
	void Array<element>::push_back(const element& item)
	{
		bool added_block = false;
		if (size_ + 1 > capacity_)
		{
			try
			{
				add_block();
				added_block = true;
			}
			catch (const std::bad_alloc&)
			{
				throw;
			}
		}
		try
		{
			new (data_[(size_) / block_size_] + (size_ % block_size_)) element(item);
		}
		catch (...)
		{
			if (added_block)
			{
				remove_block();
			}
			throw;
		}
		++size_;
	}

	template<typename element>
	void Array<element>::push_back(element&& item)
	{
		bool added_block = false;
		if (size_ + 1 > capacity_)
		{
			try
			{
				add_block();
			}
			catch (const std::bad_alloc&)
			{
				throw;
			}
			added_block = true;
		}
		try
		{
			new (data_[(size_) / block_size_] + (size_ % block_size_)) element(std::move(item));
		}
		catch (...)
		{
			if (added_block)
			{
				remove_block();
			}
			throw;
		}
		++size_;
	}

	template<typename element>
	void Array<element>::pop_back()
	{
#ifdef __DEBUG__
		if (size_ == 0)
		{
			throw std::invalid_argument("Empty array");
		}
#endif // __DEBUG__
		element* e_ptr = data_[(size_ - 1) / block_size_] + ((size_ - 1) % block_size_);
		e_ptr->~element();
		--size_;
		if (size_ % block_size_ == 0)
		{
			remove_block();
		}
	}

	template<typename element>
	Array<element>::~Array() noexcept
	{
		clear_items();
	}

	template<typename element>
	Array<element>::Array(std::initializer_list<element> items) : block_size_(10), capacity_(0), size_(0)
	{
		for (auto&& item : items)
		{
			try
			{
				push_back(item);
			}
			catch (...)
			{
				clear_items();
				throw;
			}
		}
	}

	template<typename element>
	element& Array<element>::at(size_t index)
	{
#ifdef __DEBUG__
		if (index >= size_)
		{
			throw std::out_of_range("Invalid index");
		}
#endif // __DEBUG__
		return *(data_[(index) / block_size_] + (index % block_size_));
	}

	template<typename element>
	const element& Array<element>::at(size_t index) const
	{
#ifdef __DEBUG__
		if (index >= size_)
		{
			throw std::out_of_range("Invalid index");
		}
#endif // __DEBUG__
		return *(data_[(index) / block_size_] + (index % block_size_));
	}

	template<typename element>
	element& Array<element>::operator[](size_t index)
	{
#ifdef __DEBUG__
		if (index >= size_)
		{
			throw std::out_of_range("Invalid index");
		}
#endif // __DEBUG__
		return at(index);
	}

	template<typename element>
	const element& Array<element>::operator[](size_t index) const
	{
#ifdef __DEBUG__
		if (index >= size_)
		{
			throw std::out_of_range("Invalid index");
		}
#endif // __DEBUG__
		return at(index);
	}

	template<typename element>
	void Array<element>::add_items(const Array<element>& other)
	{
		for (size_t index = 0; index < other.size(); ++index)
		{
			push_back(other[index]);
		}
	}

	template<typename element>
	Array<element>& Array<element>::operator=(const Array<element>& other) {
		clear_items();
		block_size_ = other.block_size_;
		try
		{
			add_items(other);
		}
		catch (...)
		{
			clear_items();
			throw;
		}
		return *this;
	}

	template<typename element>
	Array<element>& Array<element>::operator=(Array<element>&& other) noexcept
	{
		if (this == &other)
		{
			return *this;
		}
		clear_items();
		std::swap(block_size_, other.block_size_);
		std::swap(capacity_, other.capacity_);
		std::swap(size_, other.size_);
		std::swap(data_, other.data_);
		return *this;
	}

	template<typename element>
	template<bool constant>
	class Array<element>::iterator_base
	{
	private:
		friend Array<element>;
		using array_ptr_t = std::conditional_t<constant, const Array<element>*, Array<element>*>;
		array_ptr_t array_ptr_;
		size_t position_;
		iterator_base(array_ptr_t array_ptr, size_t position);
	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = std::conditional_t<constant, const element, element>;
		using pointer = std::conditional_t<constant, const element*, element*>;
		using reference = std::conditional_t<constant, const element&, element&>;
		using difference_type = std::ptrdiff_t;
		bool operator==(const iterator_base& other) const;
		bool operator!=(const iterator_base& other) const;
		iterator_base<constant>& operator++();
		iterator_base<constant> operator++(int);
		iterator_base<constant>& operator--();
		iterator_base<constant> operator--(int);
		reference operator*() const;
		pointer operator->() const;
		iterator_base<constant> operator+(difference_type n) const;
		iterator_base<constant> operator-(difference_type n) const;
		difference_type operator-(const iterator_base& other) const;
		iterator_base<constant>& operator+=(difference_type n);
		iterator_base<constant>& operator-=(difference_type n);
		reference operator[](difference_type n) const;
		bool operator<(const iterator_base& other) const;
		bool operator<=(const iterator_base& other) const;
		bool operator>(const iterator_base& other) const;
		bool operator>=(const iterator_base& other) const;
		operator iterator_base<true>() const;
	};

	template<typename element>
	template<bool constant>
	Array<element>::iterator_base<constant>::iterator_base(array_ptr_t array_ptr, size_t position) : array_ptr_(array_ptr), position_(position) { }

	template<typename element>
	template<bool constant>
	inline bool Array<element>::iterator_base<constant>::operator==(const iterator_base& other) const
	{
		return position_ == other.position_;
	}

	template<typename element>
	template<bool constant>
	inline bool Array<element>::iterator_base<constant>::operator!=(const iterator_base& other) const
	{
		return !(*this == other);
	}

	template<typename element>
	template<bool constant>
	Array<element>::iterator_base<constant>& Array<element>::iterator_base<constant>::operator++()
	{
		++position_;
		return *this;
	}

	template<typename element>
	template<bool constant>
	Array<element>::iterator_base<constant> Array<element>::iterator_base<constant>::operator++(int)
	{
		return Array<element>::iterator_base<constant>(array_ptr_, position_++);
	}

	template<typename element>
	template<bool constant>
	Array<element>::iterator_base<constant>& Array<element>::iterator_base<constant>::operator--()
	{
		--position_;
		return *this;
	}

	template<typename element>
	template<bool constant>
	Array<element>::iterator_base<constant> Array<element>::iterator_base<constant>::operator--(int)
	{
		return Array<element>::iterator_base<constant>(array_ptr_, position_--);
	}

	template<typename element>
	template<bool constant>
	Array<element>::iterator_base<constant> Array<element>::iterator_base<constant>::operator+(difference_type n) const
	{
		return Array<element>::iterator_base<constant>(array_ptr_, position_ + n);
	}

	template<typename element>
	template<bool constant>
	Array<element>::iterator_base<constant> Array<element>::iterator_base<constant>::operator-(difference_type n) const
	{
		return Array<element>::iterator_base<constant>(array_ptr_, position_ - n);
	}

	template<typename element, bool constant>
	Array<element>::iterator_base<constant> operator+(typename Array<element>::template iterator_base<constant>::difference_type n,
		typename Array<element>::template iterator_base<constant>& it)
	{
		return it + n;
	}

	template<typename element, bool constant>
	Array<element>::iterator_base<constant> operator-(typename Array<element>::template iterator_base<constant>::difference_type n,
		typename Array<element>::template iterator_base<constant>& it)
	{
		return it - n;
	}

	template<typename element>
	template<bool constant>
	Array<element>::iterator_base<constant>::reference Array<element>::iterator_base<constant>::operator*() const
	{
		return (*array_ptr_)[position_];
	}

	template<typename element>
	template<bool constant>
	Array<element>::iterator_base<constant>::pointer Array<element>::iterator_base<constant>::operator->() const
	{
		return &(*array_ptr_)[position_];
	}

	template<typename element>
	template<bool constant>
	Array<element>::iterator_base<constant>::difference_type Array<element>::iterator_base<constant>::operator-(const iterator_base& other) const
	{
		return position_ > other.position_ ? (position_ - other.position_) : (other.position_ - position_);
	}

	template<typename element>
	template<bool constant>
	Array<element>::iterator_base<constant>& Array<element>::iterator_base<constant>::operator+=(difference_type n)
	{
		position_ += n;
		return *this;
	}

	template<typename element>
	template<bool constant>
	Array<element>::iterator_base<constant>& Array<element>::iterator_base<constant>::operator-=(difference_type n)
	{
		position_ -= n;
		return *this;
	}

	template<typename element>
	template<bool constant>
	Array<element>::iterator_base<constant>::reference Array<element>::iterator_base<constant>::operator[](difference_type n) const
	{
		return (*array_ptr_)[position_ + n];
	}

	template<typename element>
	template<bool constant>
	inline bool Array<element>::iterator_base<constant>::operator<(const iterator_base& other) const
	{
		return position_ < other.position_;
	}

	template<typename element>
	template<bool constant>
	inline bool Array<element>::iterator_base<constant>::operator<=(const iterator_base& other) const
	{
		return position_ <= other.position_;
	}

	template<typename element>
	template<bool constant>
	inline bool Array<element>::iterator_base<constant>::operator>(const iterator_base& other) const
	{
		return position_ > other.position_;
	}

	template<typename element>
	template<bool constant>
	inline bool Array<element>::iterator_base<constant>::operator>=(const iterator_base& other) const
	{
		return position_ >= other.position_;
	}

	template<typename element>
	template<bool constant>
	Array<element>::iterator_base<constant>::operator Array<element>::iterator_base<true>() const
	{
		return const_iterator(array_ptr_, position_);
	}

	template<typename element>
	Array<element>::iterator Array<element>::begin()
	{
		return iterator(this, 0);
	}

	template<typename element>
	Array<element>::iterator Array<element>::end()
	{
		return iterator(this, size_);
	}

	template<typename element>
	Array<element>::const_iterator Array<element>::begin() const
	{
		return const_iterator(this, 0);
	}

	template<typename element>
	Array<element>::const_iterator Array<element>::end() const
	{
		return const_iterator(this, size_);
	}

	template<typename element>
	Array<element>::const_iterator Array<element>::cbegin() const
	{
		return const_iterator(this, 0);
	}

	template<typename element>
	Array<element>::const_iterator Array<element>::cend() const
	{
		return const_iterator(this, size_);
	}
};

#endif // !ARRAY_HPP_
