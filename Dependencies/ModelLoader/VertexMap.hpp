#pragma once

#include <vector>
#include <cstdint>

namespace mload {

	struct vec3 {
		float x, y, z;

		bool operator==(const vec3& other) const {
			return x == other.x && y == other.y && z == other.z;
		}
	};

	struct Vertex {
		vec3 pos;
		vec3 normal;

		Vertex() {}
		Vertex(float c) : pos({ c, c, c }), normal({ c, c, c }) {} 
		Vertex(const vec3& pos, const vec3& normal) : pos(pos), normal(normal) {}
		bool operator==(const Vertex& other) const {
			return pos == other.pos && normal == other.normal;
		}
	};
	struct ObjVertexIndex {
		uint32_t posIndex;
		uint32_t normalIndex; 

		ObjVertexIndex() {}
		bool operator==(const ObjVertexIndex& other) const { return *(uint64_t*)this == *(uint64_t*)&other; }

	};

	template<typename K, typename V> 
	class Map {
	private: 

		struct LinkedListItem {
			K key;
			V value;
			LinkedListItem* pNext;
			LinkedListItem() { }
		};

	public:

		Map(size_t m_bucketCount, size_t predictedElementCount);
		Map(const Map&) = delete; 
		void operator=(const Map&) = delete;

		V* getKeyValue(const K& key, bool* itemAlreadyExists);

		~Map();

	private: 

		LinkedListItem**             m_buckets     = nullptr; 
		size_t                       m_bucketCount; 
		std::vector<LinkedListItem*> m_elementAllocs; 

		LinkedListItem*              m_freeElement = nullptr;
		size_t                       m_size        = 0;
		size_t                       m_capacity    = 0; 

	};

}

#include "VertexMap.inl"