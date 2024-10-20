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

		Vertex() {};
		Vertex(const vec3& pos, const vec3& normal) : pos(pos), normal(normal) {}
		bool operator==(const Vertex& other) const {
			return pos == other.pos && normal == other.normal;
		}
	};
}

class VertexMap {
public:

	VertexMap(size_t m_bucketCount, size_t predictedElementCount);

	uint32_t* getKeyValue(const mload::Vertex& key, bool* itemAlreadyExists);

	~VertexMap();

private: 

	struct LinkedListItem {
		mload::Vertex  key;
		uint32_t       value;
		LinkedListItem* pNext; 
		LinkedListItem() { }
	};

	LinkedListItem**             m_buckets     = nullptr; 
	size_t                       m_bucketCount; 
	std::vector<LinkedListItem*> m_elementAllocs; 

	LinkedListItem*              m_freeElement = nullptr;
	size_t                       m_size        = 0;
	size_t                       m_capacity    = 0; 

};