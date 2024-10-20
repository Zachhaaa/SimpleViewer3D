#include "ModelLoader.hpp"

#include <cstdio>
#include <memory>
#include <cassert> 
#include <unordered_map>

namespace std {
	template<> struct hash<mload::Vertex> {
		size_t operator()(mload::Vertex const& v) const {
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
	};
}

bool mload::openModel(const char* fileName, std::vector<Vertex>* vertexBuff, std::vector<uint32_t>* indexBuff, bool* isTextFormat) {

	FILE* file = fopen(fileName, "rb");
	if (file == nullptr) return false; 

	fseek(file, 0, SEEK_END); 
	uint32_t fileSize = (uint32_t)ftell(file);
	fseek(file, 0, SEEK_SET);

	std::unique_ptr<char[]> fData(new char[fileSize]);
	fread(fData.get(), 1, fileSize, file);

	uint32_t facetCount;
	if (memcmp(fData.get(), "solid", 5) == 0) {
		facetCount = (fileSize / 258 + 1);
		*isTextFormat = true;
	}
	else {
		facetCount = *(uint32_t*)&fData[80];
		*isTextFormat = false;
	}

	indexBuff->reserve(3 * facetCount); 
	vertexBuff->reserve((size_t)(0.9f * indexBuff->capacity()));
	VertexMap uniqueVertices((size_t)(1.5 * indexBuff->capacity()), vertexBuff->capacity()); 

	if (*isTextFormat) {
		constexpr size_t floatsPerFacet = 12;
		float facet[floatsPerFacet];
		uint32_t floatIndex = 0;
		for (char* c = fData.get(), *end = c + fileSize; c < end; ++c) {
			if (*c != '.') continue;

			// first decimal place
			float value = (float)(c[-1] - '0');
			// handle negative
			float negative = c[-2] == '-' ? -1.0f : 1.0f;
			++c;
			// Convert significand to float
			for (float place = 0.1f; *c != 'e'; place *= 0.1f, ++c) {
				value += place * (*c - '0');
			}
			value *= negative;

			// Apply exponent
			c += 4;
			for (; *c > '0' - 1 && *c < '9' + 1; ++c);
			int exponent = 0;
			uint32_t digitBase = 1;
			char* ex_c = &c[-1];
			for (; *ex_c != '-' && *ex_c != '+'; --ex_c, digitBase *= 10) {
				exponent += digitBase * (*ex_c - '0');
			}
			exponent *= *ex_c == '-' ? -1 : 1;
			value *= powf(10.0f, (float)exponent);

			facet[floatIndex] = value;
			++floatIndex;
			if (floatIndex < floatsPerFacet) continue;

			floatIndex = 0;

			Vertex v(*(vec3*)&facet[3], *(vec3*)&facet[0]);
			bool keyExisted;
			uint32_t* pIndex = uniqueVertices.getKeyValue(v, &keyExisted);
			if (!keyExisted) {
				*pIndex = (uint32_t)vertexBuff->size();
				vertexBuff->push_back(v);
			}
			indexBuff->push_back(*pIndex);

			v.pos = *(vec3*)&facet[6];
			pIndex = uniqueVertices.getKeyValue(v, &keyExisted);
			if (!keyExisted) {
				*pIndex = (uint32_t)vertexBuff->size();
				vertexBuff->push_back(v);
			}
			indexBuff->push_back(*pIndex);

			v.pos = *(vec3*)&facet[9];
			pIndex = uniqueVertices.getKeyValue(v, &keyExisted);
			if (!keyExisted) {
				*pIndex = (uint32_t)vertexBuff->size();
				vertexBuff->push_back(v);
			}
			indexBuff->push_back(*pIndex);

		}
	} 
	// binary STL
	else {
		for (char* pFacet = &fData[84], *end = &fData[fileSize]; pFacet < end; pFacet += 50) {

			Vertex v(*(vec3*)&pFacet[12], *(vec3*)&pFacet[0]);
			bool keyExisted; 
			uint32_t* pIndex = uniqueVertices.getKeyValue(v, &keyExisted); 
			if (!keyExisted) {
				*pIndex = (uint32_t)vertexBuff->size(); 
				vertexBuff->push_back(v);
			}
			indexBuff->push_back(*pIndex);

			v.pos = *(vec3*)&pFacet[24];
			pIndex = uniqueVertices.getKeyValue(v, &keyExisted);
			if (!keyExisted) {
				*pIndex = (uint32_t)vertexBuff->size();
				vertexBuff->push_back(v);
			}
			indexBuff->push_back(*pIndex);

			v.pos = *(vec3*)&pFacet[36];
			pIndex = uniqueVertices.getKeyValue(v, &keyExisted);
			if (!keyExisted) {
				*pIndex = (uint32_t)vertexBuff->size();
				vertexBuff->push_back(v);
			}
			indexBuff->push_back(*pIndex);

		}
	}
	
	return true;

}