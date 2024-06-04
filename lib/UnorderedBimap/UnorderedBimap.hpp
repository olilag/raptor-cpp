#ifndef UNORDERED_BIMAP_HPP_
#define UNORDERED_BIMAP_HPP_

#include <type_traits>
#include <unordered_map>
#include <utility>
#include <Array.hpp>

/**
 * @brief A simple bidirectional unordered map
 *
 * Requires `std::hash` specializations for `K1` and `K2`
 * Requires `K1` must be different type than 'K2'
 * 
 * @tparam K1 Type of first key
 * @tparam K2 Type of second key
 */
template<typename K1, typename K2> requires (!std::is_same_v<K1, K2>)
class UnorderedBimap
{
private:
	// doesn't reallocate elements after push_back
	lib::Array<std::pair<K1, K2>> data_;
	std::unordered_map<K1, K2*> k1ToK2_;
	std::unordered_map<K2, K1*> k2ToK1_;
public:
	UnorderedBimap();
	UnorderedBimap(const UnorderedBimap<K1, K2>& other);
	UnorderedBimap(UnorderedBimap<K1, K2>&& other) noexcept;

	UnorderedBimap<K1, K2>& operator=(const UnorderedBimap<K1, K2>& other);
	UnorderedBimap<K1, K2>& operator=(UnorderedBimap<K1, K2>&& other) noexcept;

	/**
	 * @brief Get number of elements
	 * 
	 * @return Number of elements
	 */
	size_t size() const;

	/**
	 * @brief Insert pair `<K1, K2>`
	 * 
	 * @param key1 First key
	 * @param key2 Second key
	 * @return true Pair added to mapping
	 * @return false Pair not added to mapping
	 */
	bool insert(const K1& key1, const K2& key2);
	bool insert(K1&& key1, K2&& key2);

	/**
	 * @brief Insert pair `<K2, K1>`
	 * 
	 * @param key1 First key
	 * @param key2 Second key
	 * @return true Pair added to mapping
	 * @return false Pair not added to mapping 
	 */
	bool insert(const K2& key1, const K1& key2);
	bool insert(K2&& key1, K1&& key2);
	
	const K1& at(const K2& key) const;
	const K1& operator[](const K2& key) const;

	const K2& at(const K1& key) const;
	const K2& operator[](const K1& key) const;
};

template<typename K1, typename K2> requires (!std::is_same_v<K1, K2>)
UnorderedBimap<K1, K2>::UnorderedBimap() : data_(), k1ToK2_(), k2ToK1_() { }

template<typename K1, typename K2> requires (!std::is_same_v<K1, K2>)
UnorderedBimap<K1, K2>::UnorderedBimap(const UnorderedBimap<K1, K2>& other)
{
	data_ = other.data_;
	for (auto&& [key1, key2] : data_)
	{
		insert(key1, key2);
	}
}

template<typename K1, typename K2> requires (!std::is_same_v<K1, K2>)
UnorderedBimap<K1, K2>::UnorderedBimap(UnorderedBimap<K1, K2>&& other) noexcept : data_(), k1ToK2_(), k2ToK1_()
{
	std::swap(data_, other.data_);
	std::swap(k1ToK2_, other.k1ToK2_);
	std::swap(k2ToK1_, other.k2ToK1_);
}

template<typename K1, typename K2> requires (!std::is_same_v<K1, K2>)
UnorderedBimap<K1, K2>& UnorderedBimap<K1, K2>::operator=(const UnorderedBimap<K1, K2>& other)
{
	if (this == &other)
	{
		return *this;
	}
	data_ = other.data_;
	for (auto&& [key1, key2] : data_)
	{
		insert(key1, key2);
	}
	return *this;
}

template<typename K1, typename K2> requires (!std::is_same_v<K1, K2>)
UnorderedBimap<K1, K2>& UnorderedBimap<K1, K2>::operator=(UnorderedBimap<K1, K2>&& other) noexcept
{
	if (this == &other)
	{
		return *this;
	}
	std::swap(data_, other.data_);
	std::swap(k1ToK2_, other.k1ToK2_);
	std::swap(k2ToK1_, other.k2ToK1_);
	return *this;
}

template<typename K1, typename K2> requires (!std::is_same_v<K1, K2>)
inline size_t UnorderedBimap<K1, K2>::size() const
{
	return data_.size();
}

template<typename K1, typename K2> requires (!std::is_same_v<K1, K2>)
bool UnorderedBimap<K1, K2>::insert(const K1& key1, const K2& key2)
{
	if (k1ToK2_.contains(key1) || k2ToK1_.contains(key2))
	{
		return false;
	}
	data_.push_back(std::make_pair(key1, key2));
	auto&& last_pair = data_[data_.size() - 1];
	k1ToK2_.emplace(last_pair.first, &last_pair.second);
	k2ToK1_.emplace(last_pair.second, &last_pair.first);
	return true;
}

template<typename K1, typename K2> requires (!std::is_same_v<K1, K2>)
bool UnorderedBimap<K1, K2>::insert(K1&& key1, K2&& key2)
{
	if (k1ToK2_.contains(key1) || k2ToK1_.contains(key2))
	{
		return false;
	}
	data_.push_back(std::make_pair(std::move(key1), std::move(key2)));
	auto&& last_pair = data_[data_.size() - 1];
	k1ToK2_.emplace(last_pair.first, &last_pair.second);
	k2ToK1_.emplace(last_pair.second, &last_pair.first);
	return true;
}

template<typename K1, typename K2> requires (!std::is_same_v<K1, K2>)
bool UnorderedBimap<K1, K2>::insert(const K2& key1, const K1& key2)
{
	return insert(key2, key1);
}

template<typename K1, typename K2> requires (!std::is_same_v<K1, K2>)
bool UnorderedBimap<K1, K2>::insert(K2&& key1, K1&& key2)
{
	return insert(std::move(key2), std::move(key1));
}

template<typename K1, typename K2> requires (!std::is_same_v<K1, K2>)
inline const K1& UnorderedBimap<K1, K2>::at(const K2& key) const
{
	return *k2ToK1_.at(key);
}

template<typename K1, typename K2> requires (!std::is_same_v<K1, K2>)
inline const K1& UnorderedBimap<K1, K2>::operator[](const K2& key) const
{
	return at(key);
}

template<typename K1, typename K2> requires (!std::is_same_v<K1, K2>)
inline const K2& UnorderedBimap<K1, K2>::at(const K1& key) const
{
	return *k1ToK2_.at(key);
}

template<typename K1, typename K2> requires (!std::is_same_v<K1, K2>)
inline const K2& UnorderedBimap<K1, K2>::operator[](const K1& key) const
{
	return at(key);
}

#endif // !UNORDERED_BIMAP_HPP_
