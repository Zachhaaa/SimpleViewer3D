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
/// Get the next 3 float values in a string from an obj file
inline bool charIsDigit(const char c) { return c >= '0' && c <= '9'; } 
/// @param str string to read floats from
/// @param i pointer to the index in the data. 
/// @param v pointer to the first element(x) in a vec3
static void objGetVec3FromText(const char* str, uint32_t* i, uint32_t fileSize, float* v) {
	for (int componentIndex = 0; componentIndex < 3; componentIndex++) { // componentIndex == 0: x, componentIndex == 1: y, componentIndex == 2: z

		bool endOfNumber;
		while (true) {
			endOfNumber = str[*i] == ' ' || str[*i] == '\n' || str[*i] == '\r';
			if (endOfNumber || str[*i] == '.') break;
			(*i)++; 
		}
		float placeHolder = 1.0f;
		float& value = v[componentIndex];
		value = 0.0f; 
		for (const char* c = &str[*i - 1]; *c != ' '; c--, placeHolder *= 10.0f) {
			if (*c == '-') { value = -value ; break; }
			value += (*c - '0') * placeHolder;
		}
		if (str[*i] == ' ') { (*i)++; continue; }
		else if (endOfNumber) continue; 
		(*i)++;

		placeHolder = value > 0 ? 0.1f : -0.1f;
		for (; charIsDigit(str[*i]) && *i < fileSize; (*i)++, placeHolder /= 10.0f) {
			value += (str[*i] - '0') * placeHolder;
		}
		(*i)++; 

	}
}
/// @param pOnesPlace Pointer to the ones place of the unsiged integer to read. 
static uint32_t getUint32FromText(const char* pOnesPlace) {
	uint32_t placeHolder = 1;
	uint32_t value = 0;
	for (const char* c = pOnesPlace; *c != ' ' && *c != '/'; c--, placeHolder *= 10) { value += (*c - '0') * placeHolder; }
	return value;
}
static void objGetIndexFromText(const char* str, uint32_t* i, uint32_t fileSize, mload::ObjVertexIndex* v) {

	for (; str[*i] != ' ' && str[*i] != '/'; (*i)++) {}

	v->posIndex = getUint32FromText(&str[*i - 1]);
	if (str[*i] == ' ') {
		v->normalIndex = 1; // TODO: I need to handle no vertex normal index differently.
	}
	else {

		// Skip the next two slashes. 
		for (; str[*i] != '/'; (*i)++) {} 
		(*i)++;
		for (; str[*i] != '/'; (*i)++) {} 
		(*i)++;

		for (; charIsDigit(str[*i]); (*i)++) {}
		v->normalIndex = getUint32FromText(&str[*i - 1]); 
	}


}

mload::Success mload::openModel(const char* fileName, std::vector<Vertex>* vertexBuff, std::vector<uint32_t>* indexBuff, bool* isTextFormat) {
	
	size_t fileNameLen = strlen(fileName); 
	bool objFile = strcmp(&fileName[fileNameLen - 4], ".obj") == 0;
	bool stlFile = strcmp(&fileName[fileNameLen - 4], ".stl") == 0; 
	if (!(objFile || stlFile)) return Success::WRONG_FILE_FORMAT;

	FILE* file = fopen(fileName, "rb");
	if (file == nullptr) return Success::COULD_NOT_OPEN_FILE; 

	fseek(file, 0, SEEK_END); 
	uint32_t fileSize = (uint32_t)ftell(file);
	fseek(file, 0, SEEK_SET);
	std::unique_ptr<char[]> fData(new char[fileSize]);
	fread(fData.get(), 1, fileSize, file);

	uint32_t vertexElementsCapacity = 0; 
	uint32_t indexElementsCapacity  = 0; // .obj use only
	uint32_t normalsCapacity        = 0; // .obj use only

	if (stlFile) {
		uint32_t facetCount = 0; 
		if (memcmp(fData.get(), "solid", 5) == 0) {
			facetCount = (fileSize / 258 + 1);
			*isTextFormat = true;
		}
		else {
			facetCount = *(uint32_t*)&fData[80];
			*isTextFormat = false;
		}
		indexElementsCapacity = 3 * facetCount; 
	}
	// If (objFile)
	else {
		*isTextFormat = true; 
		for (uint32_t i = 0; i < fileSize; i++) {
			
			if(fData[i] == 'v') {
				i++;
				if      (fData[i] == ' ') vertexElementsCapacity++;
				else if (fData[i] == 'n') normalsCapacity++;
				i+= 6; // 6 = minimum line length after 'v' so we jump it.
				for (; i < fileSize; i++) { if (fData[i] == '\n') break; }
			}
			else if (fData[i] == 'f') {  
				i++;
				for (uint32_t indexCount = 0; i < fileSize - 1; i++) {
					if (fData[i] == '\n') {
						if(fData[i] )
						break;
					}
					if (fData[i] == ' ' && charIsDigit(fData[i + 1])) {
						indexCount++; // BOOKMARK: fix the space before the newline counting as an additional index.  
      					indexElementsCapacity += indexCount > 3 ? 3 : 1;
					}
				}
			}
			else {
				for (; i < fileSize; i++) { if (fData[i] == '\n') break; }
			}

		}
	}

	// TODO: clean up the nesting in the stl code.
	indexBuff->reserve(indexElementsCapacity); 
	vertexBuff->reserve((size_t)(0.9f * indexElementsCapacity));
	if (stlFile) {
		Map<Vertex, uint32_t> uniqueVertices((size_t)(1.5 * indexElementsCapacity), vertexElementsCapacity);
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
				for (; charIsDigit(*c); ++c);
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
	}
	// If (objFile)
	else {
		// TODO: run different code if normalsCapacity == 0. Normals are not required in the obj format
		assert(normalsCapacity > 0); 
		vec3* const vertexPositions           = (vec3*)malloc(sizeof(vec3) * vertexElementsCapacity); 
		vec3*       vertexPositionsNewElement = vertexPositions; 
		vec3* const vertexNormals             = normalsCapacity > 0 ? (vec3*)malloc(sizeof(vec3) * normalsCapacity) : nullptr; 
		vec3*       vertexNormalsNewElement   = vertexNormals; 
		Map<ObjVertexIndex, uint32_t> uniqueVertices((size_t)(1.5 * indexElementsCapacity), vertexElementsCapacity);

		uint32_t facetSectionIndex = 0;
		for (uint32_t i = 0; i < fileSize; i++) {
			// TODO: try this to see if it is faster at parsing if (*(int16_t*)&fData[i] == ('v' | ' ' << 8))
			if (fData[i] == 'v') {
				i++;
				if (fData[i] == ' ') { 
					do { i++; } while (fData[i] == ' ');
					objGetVec3FromText(fData.get(), &i, fileSize, (float*)vertexPositionsNewElement);
					vertexPositionsNewElement++; 
				}
				else if (fData[i] == 'n') {
					i += 2; 
					for (; fData[i] == ' '; i++) {}
					objGetVec3FromText(fData.get(), &i, fileSize, (float*)vertexNormalsNewElement);
  					vertexNormalsNewElement++; // bookmark: check normal parsing and then facet parsing. 
				}
			}
			// break if the vertex position and normals have been filled.
			else if (fData[i] == 'f' && vertexPositions != vertexPositionsNewElement && vertexNormals != vertexNormalsNewElement) { 
				facetSectionIndex = i; 
				break; 
			}
			else {
				for (; i < fileSize; i++) { if (fData[i] == '\n') break; }
			}
		}
		for (uint32_t i = facetSectionIndex; i < fileSize;) {
			uint32_t vertexCountInFacet = 0;
			for (; i < fileSize; i++) { if (fData[i-1] == '\n' && fData[i] == 'f') break; }
			do { i++; } while (fData[i] == ' '); // jump whitespace
			for (; i < fileSize && charIsDigit(fData[i]); i++) {
				mload::ObjVertexIndex vertexIndex;
				objGetIndexFromText(fData.get(), &i, fileSize, &vertexIndex);
				bool keyExists;
				uint32_t* pIndex = uniqueVertices.getKeyValue(vertexIndex, &keyExists);
				if (!keyExists) {
					*pIndex = (uint32_t)vertexBuff->size(); 
					// - 1 to convert to 0 based indexing (.obj format doesn't use 0 based indexing)
					vertexBuff->emplace_back(vertexPositions[vertexIndex.posIndex - 1], vertexNormals[vertexIndex.normalIndex - 1]);
				}
				vertexCountInFacet++;
				if (vertexCountInFacet > 3) {
					indexBuff->push_back((*indexBuff)[indexBuff->size() - vertexCountInFacet + 1]);
					indexBuff->push_back((*indexBuff)[indexBuff->size() - 2]);
				}
				indexBuff->push_back(*pIndex);

			}
		}
		free(vertexPositions); 
		if (vertexNormals != nullptr) free(vertexNormals); 

	}

	return Success::SUCCESS;
}