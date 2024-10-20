#include "VertexMap.hpp"

#include <cassert>

VertexMap::VertexMap(size_t _bucketCount, size_t predictedElementCount) 
: m_bucketCount(_bucketCount), m_capacity(predictedElementCount) 
{
	
	m_buckets = new LinkedListItem*[_bucketCount]; 
	memset(m_buckets, 0, sizeof(LinkedListItem*) * _bucketCount); 
	
	m_freeElement = new LinkedListItem[predictedElementCount]; 
	m_elementAllocs.push_back(m_freeElement); 

}

size_t hashFunc(const mload::Vertex& v) {
	size_t h1 = std::hash<float>{}(v.pos.x);
	size_t h2 = std::hash<float>{}(v.pos.y);
	size_t h3 = std::hash<float>{}(v.pos.z);
	size_t h4 = std::hash<float>{}(v.normal.x);
	size_t h5 = std::hash<float>{}(v.normal.y);
	size_t h6 = std::hash<float>{}(v.normal.z);

	size_t result = h1;
	result ^= h2 + 0x9e3779b9 + (result << 6) + (result >> 2);
	result ^= h3 + 0x9e3779b9 + (result << 6) + (result >> 2);
	result ^= h4 + 0x9e3779b9 + (result << 6) + (result >> 2);
	result ^= h5 + 0x9e3779b9 + (result << 6) + (result >> 2);
	result ^= h6 + 0x9e3779b9 + (result << 6) + (result >> 2);

	return result;
}

uint32_t* VertexMap::getKeyValue(const mload::Vertex& key, bool *itemAlreadyExists) {

	size_t bucketIndex = hashFunc(key) % m_bucketCount; 

	LinkedListItem**  item = &m_buckets[bucketIndex];
	if (*item != nullptr) {
		for (; *item != nullptr; item = &(*item)->pNext) {
			if ((*item)->key == key) {
				*itemAlreadyExists = true;
				return &(*item)->value;
			}
		}
	}

	*itemAlreadyExists = false;
	(*item) = m_freeElement;
	(*item)->key   = key;
	(*item)->pNext = nullptr;
	
	++m_size;
	if (m_size < m_capacity) ++m_freeElement;
	else {

		const size_t mallocElementCount = 200;
		m_freeElement = new LinkedListItem[mallocElementCount];
		m_elementAllocs.push_back(m_freeElement); 
		m_capacity += mallocElementCount;

	}

	return &(*item)->value;

}

VertexMap::~VertexMap() {
	
	delete[] m_buckets; 
	for (LinkedListItem* alloc : m_elementAllocs) 
		delete[] alloc;

}