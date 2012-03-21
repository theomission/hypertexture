#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include "common.hh"

////////////////////////////////////////////////////////////////////////////////
// Hashing functions

// Default Hash
template< class T >
inline unsigned int MakeHash(const T& key) 
{
	unsigned int result = 0;
	int len = sizeof(T);

	// reinterpret the bytes as a string
	const char* p = reinterpret_cast<const char*>(&key);

	while(len--) 
		result += 261 * *p++;

	return result;
}

inline unsigned int MakeHash(const char* key) 
{
	unsigned int result = 0;
	const char* p = key;

	while(*p) result += 261 * *p++;
	return result;
}

inline unsigned int MakeHash(const std::string& str)
{
	return MakeHash(str.c_str());
}

////////////////////////////////////////////////////////////////////////////////	
// Resizing hash map
template< class K, class V>
class HashMap
{
	friend class iterator;
	friend class const_iterator;
public:
	class iterator;
	class const_iterator;

	struct Pair
	{
		Pair() : key(),value() {}
		K key;
		V value;
	};

	explicit HashMap(unsigned int size = 31) 
		: m_pairs(size)
		  , m_used(size)
		  , m_maxLoad(0.5f)
		  , m_grow(1.5f)
		  , m_numSetItems(0)
	{
		Initialize();
	}

	Pair* set(const K& key, const V& value);
	bool has(const K& key) const;
	const V& get(const K& key) const;
	V& get(const K& key);
	Pair* getpair(const K& key);
	const Pair* getpair(const K& key) const;
	void del(const K& key);

	const V& operator[](const K& key) const { return get(key); }
	V& operator[](const K& key) ;

	unsigned int capacity() const { return m_pairs.capacity(); }
	unsigned int size() const { return m_numSetItems; }

	void resize(unsigned int newSize);

	void clear();

	void swap(HashMap &other);

	iterator begin();
	iterator end();

	const_iterator begin() const;
	const_iterator end() const;

private:
	HashMap(const HashMap& );
	HashMap& operator=(const HashMap&);

	void Initialize();
	unsigned int FindPair(const K& key) const;
	unsigned int Probe(const K &key) const;

	std::vector<Pair> m_pairs;
	std::vector<bool> m_used;
	float m_maxLoad;
	float m_grow;
	unsigned int m_numSetItems;
};

////////////////////////////////////////////////////////////////////////////////	
// Impl
template< class K, class V>
typename HashMap<K,V>::Pair* HashMap<K,V>::set(const K& key, const V& value)
{					
	// grow if we need to.
	if( m_numSetItems > (unsigned int)(m_maxLoad * m_pairs.size()) )
	{
		resize(static_cast<unsigned int>(m_pairs.size()*m_grow));
	}

	unsigned int index = Probe(key);
	m_pairs[index].key = key;
	m_pairs[index].value = value;
	m_used[index] = true;
	++m_numSetItems;
	return &m_pairs[index];
}

template< class K, class V>
V& HashMap<K,V>::operator[](const K& key) 
{
	unsigned int index = Probe(key);
	if(m_used[index])
		return m_pairs[index].value;
	else return set(key,V())->value;
}

template< class K, class V>
bool HashMap<K,V>::has(const K& key) const
{
	unsigned int index = Probe(key);
	return (m_used[index]);
}

template< class K, class V>
const V& HashMap<K,V>::get(const K& key) const
{
	unsigned int index = Probe(key);
	ASSERT(m_used[index]);
	return m_pairs[index].value;
}

template< class K, class V>
V& HashMap<K,V>::get(const K& key) 
{
	unsigned int index = Probe(key);
	ASSERT(m_used[index]);
	return m_pairs[index].value;
}

template< class K, class V>
typename HashMap<K,V>::Pair* HashMap<K,V>::getpair(const K& key)
{
	unsigned int index = Probe(key);
	if(!m_used[index])
		return 0;
	Pair* pair = &m_pairs[index];
	return pair;
}

template< class K, class V>
const typename HashMap<K,V>::Pair* HashMap<K,V>::getpair(const K& key) const
{
	unsigned int index = Probe(key);
	if(!m_used[index])
		return 0;
	const Pair* pair = &m_pairs[index];
	return pair;
}

template< class K, class V>
void HashMap<K,V>::del(const K& key) 
{
	unsigned int delIndex = Probe(key);
	ASSERT(m_used[delIndex]);
	m_pairs[delIndex] = Pair();
	m_used[delIndex] = false;
	ASSERT(m_numSetItems > 0);
	--m_numSetItems;					

	// reinsert values after delIndex until they don't probe
	for(unsigned int i = 1, c = m_pairs.size(); i < c; ++i)
	{
		unsigned int probeIndex = (delIndex + i) % c;
		if(!m_used[probeIndex])
			break;
		unsigned int hashedIndex = (MakeHash(m_pairs[probeIndex].key) % c);
		if(hashedIndex != probeIndex)
		{
			Pair pair = m_pairs[probeIndex];
			m_pairs[probeIndex] = Pair();
			m_used[probeIndex] = false;
			set(pair.key, pair.value);
		}
	}
}

template< class K, class V>
void HashMap<K,V>::resize(unsigned int newSize)
{
	ASSERT(newSize >= m_numSetItems);

	HashMap newHash(newSize);

	for(unsigned int i = 0, c = m_pairs.size(); i < c; ++i)
	{
		if(m_used[i])
			newHash.set(m_pairs[i].key, m_pairs[i].value);
	}

	swap(newHash);
}

template< class K, class V>
void HashMap<K,V>::clear()
{
	for(unsigned int i = 0, c = m_pairs.size(); i < c; ++i)
		m_pairs[i] = Pair();
	m_numSetItems = 0;
}

template< class K, class V>
void HashMap<K,V>::swap(HashMap &other)
{
	std::swap(m_maxLoad, other.m_maxLoad);
	std::swap(m_grow, other.m_grow);
	std::swap(m_numSetItems, other.m_numSetItems);
	m_pairs.swap(other.m_pairs);
	m_used.swap(other.m_used);
}


template< class K, class V>
typename HashMap<K,V>::iterator HashMap<K,V>::begin()
{
	for(unsigned int i = 0, c = m_pairs.size(); i < c; ++i)
	{
		if(m_used[i])
			return iterator(this, i);
	}
	return end();
}

template< class K, class V>
typename HashMap<K,V>::iterator HashMap<K,V>::end()
{
	return iterator(this, m_pairs.size());
}


template< class K, class V>
typename HashMap<K,V>::const_iterator HashMap<K,V>::begin() const
{
	for(unsigned int i = 0, c = m_pairs.size(); i < c; ++i)
	{
		if(m_used[i])
			return const_iterator(this, i);
	}
	return end();
}

template< class K, class V>
typename HashMap<K,V>::const_iterator HashMap<K,V>::end() const
{
	return const_iterator(this, m_pairs.size());
}

////////////////////////////////////////////////////////////////////////////////
// Internal impl
template< class K, class V>
void HashMap<K,V>::Initialize()
{
	m_maxLoad = 0.5f;
	m_grow = 1.5f;
	m_pairs.resize(m_pairs.capacity());
	m_used.resize(m_pairs.capacity());
	m_numSetItems = 0;
}

template< class K, class V>
unsigned int HashMap<K,V>::Probe(const K& key) const
{
	unsigned int keyhash = MakeHash(key) ;
	const unsigned int pairsSize = m_pairs.size();
	for(unsigned int probe = 0; probe < pairsSize; ++probe)
	{
		unsigned int index = (keyhash + probe) % pairsSize;
		if(!m_used[index])
		{
			return index;
		}
		else if(m_pairs[index].key == key)
		{
			return index;
		}
	}
	// Key can't be in the hash!
	ASSERT(false);
	return (unsigned int)(-1);
}

////////////////////////////////////////////////////////////////////////////////
template< class K, class V>
class HashMap<K,V>::iterator
{
	friend class HashMap<K, V>;
	public:
	iterator() : m_owner(0), m_index(static_cast<unsigned int>(-1)) {}
	iterator(const iterator& other) : m_owner(other.m_owner), m_index(other.m_index) {}
	iterator& operator=(const iterator& other)
	{
		m_owner = other.m_owner;
		m_index = other.m_index;
		return *this;
	}

	HashMap<K, V>::Pair& operator*() {
		return m_owner.m_pairs[m_index];
	}

	HashMap<K, V>::Pair* operator->() {
		return &m_owner.m_pairs[m_index];
	}

	iterator& operator++() {
		unsigned int cur = m_index + 1;
		while(cur < m_owner->m_pairs.size() && !m_owner->m_used[cur])
			++cur;
		m_index = cur;
		return *this;
	}

	iterator operator++(int) {
		iterator temp = *this;
		unsigned int cur = m_index + 1;
		while(cur < m_owner->m_pairs.size() && !m_owner->m_used[cur])
			++cur;
		m_index = cur;
		return temp;
	}

	bool operator==(const iterator& o) const {
		return m_owner == o.m_owner &&
			m_index == o.m_index;
	}

	bool operator!=(const iterator& o) const {
		return m_owner != o.m_owner ||
			m_index != o.m_index;
	}
	protected:
	iterator(HashMap *owner, unsigned int idx) : m_owner(owner), m_index(idx) {}
	private:
	HashMap* m_owner;
	unsigned int m_index;
};

template< class K, class V>
class HashMap<K,V>::const_iterator
{
	friend class HashMap<K, V>;
	public:
	const_iterator() : m_owner(0), m_index(static_cast<unsigned int>(-1)) {}
	const_iterator(const const_iterator& other) : m_owner(other.m_owner), m_index(other.m_index) {}
	const_iterator& operator=(const const_iterator& other)
	{
		m_owner = other.m_owner;
		m_index = other.m_index;
		return *this;
	}

	const HashMap<K, V>::Pair& operator*() {
		return m_owner->m_pairs[m_index];
	}

	const HashMap<K, V>::Pair* operator->() {
		return &m_owner->m_pairs[m_index];
	}

	const_iterator& operator++() {
		unsigned int cur = m_index + 1;
		while(cur < m_owner->m_pairs.size() && !m_owner->m_used[cur])
			++cur;
		m_index = cur;
		return *this;
	}

	const_iterator operator++(int) {
		const_iterator temp = *this;
		unsigned int cur = m_index + 1;
		while(cur < m_owner->m_pairs.size() && !m_owner->m_used[cur])
			++cur;
		m_index = cur;
		return temp;
	}

	bool operator==(const const_iterator& o) const {
		return m_owner == o.m_owner &&
			m_index == o.m_index;
	}

	bool operator!=(const const_iterator& o) const {
		return m_owner != o.m_owner ||
			m_index != o.m_index;
	}
	protected:
	const_iterator(const HashMap *owner, unsigned int idx) : m_owner(owner), m_index(idx) {}
	private:
	const HashMap* m_owner;
	unsigned int m_index;
};

