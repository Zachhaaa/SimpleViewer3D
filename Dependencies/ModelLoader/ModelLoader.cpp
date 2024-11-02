#include "ModelLoader.hpp"

#include <cstdio>
#include <memory>
#include <cassert> 

#include <glm/glm.hpp>

inline void skipLine(const char*& pC, const char* end) {
	
	for (; pC < end; pC++) {
		if (*pC == '\n') { pC++; break; }
	}

}
inline void skipWhitespace(const char*& pC) {
	for (; *pC == ' '; pC++) {}
}
inline bool charIsDigit(const char c) { return c >= '0' && c <= '9'; } 
static void objGetVec3FromText(const char*& pC, const char* end, float* v) {
	for (int componentIndex = 0; componentIndex < 3; componentIndex++) { // componentIndex == 0: x, componentIndex == 1: y, componentIndex == 2: z

		for (; *pC != '.' && *pC != ' '; pC++) {}
		float placeHolder = 1.0f;
		float& value = v[componentIndex];
		value = 0.0f; 
		bool negative = false; 
		for (const char* c = pC - 1; *c != ' '; c--, placeHolder *= 10.0f) {
			if (*c == '-') { negative = true; break; };
			value += (*c - '0') * placeHolder;
		}
		if (*pC == ' ') { pC++; continue; }
		pC++;

		placeHolder = 0.1f;
		for (; charIsDigit(*pC) && pC < end; pC++, placeHolder /= 10.0f) {
			value += (*pC - '0') * placeHolder;
		}
		if (negative) value = -value; 
		pC++; 

	}
}
/// @param pOnesPlace Pointer to the ones place of the unsiged integer to read. 
static uint32_t getUint32FromText(const char* pOnesPlace) {
	uint32_t placeHolder = 1;
	uint32_t value = 0;
	for (const char* c = pOnesPlace; *c != ' ' && *c != '/'; c--, placeHolder *= 10) { value += (*c - '0') * placeHolder; }
	return value;
}
static uint32_t getVertexIndexFromVertexReference(const char*& pC) {
	for (; charIsDigit(*pC); pC++) {}
	uint32_t vertexIndex = getUint32FromText(pC - 1);
	for (; *pC != ' ' && *pC != '\n'; pC++) {} // skip texture coord index 
	return vertexIndex; 
}
static void objGetIndexFromText(const char*& pC, const char* end, mload::ObjVertexIndex* v) {

	for (; *pC != ' ' && *pC != '/'; pC++) {}

	v->posIndex = getUint32FromText(pC - 1);

	// Skip the next two slashes. 
	for (; *pC != '/'; pC++) {} 
	pC++;
	for (; *pC!= '/'; pC++) {} 
	pC++;

	for (; charIsDigit(*pC); pC++) {}
	v->normalIndex = getUint32FromText(pC - 1);

}
static void addVertex(const mload::Vertex& v, mload::Map<mload::Vertex, uint32_t>& uniqueVertices, std::vector<mload::Vertex>& vertexBuff, std::vector<uint32_t>& indexBuff) {

	bool keyExists;
	uint32_t* pIndex = uniqueVertices.getKeyValue(v, &keyExists);
	if (!keyExists) {
		*pIndex = (uint32_t)vertexBuff.size();
		vertexBuff.push_back(v);
	}
	indexBuff.push_back(*pIndex);

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

	uint32_t indexElementsCapacity  = 0; 
	uint32_t vertexElementsCapacity = 0; // .obj use only
	uint32_t normalsCapacity        = 0; // .obj use only
	// Get file data counts to presize buffers
	if (stlFile) {
		uint32_t facetCount = 0; 
		if (memcmp(fData.get(), "solid", 5) == 0) { // if ascii
			facetCount = (fileSize / 258 + 1);
			*isTextFormat = true;
		}
		else { // if binary
			facetCount = *(uint32_t*)&fData[80];
			*isTextFormat = false;
		}
		indexElementsCapacity = 3 * facetCount; 
	}
	// If (objFile)
	else {
		*isTextFormat = true; 
		for (const char* c = &fData[0], *end = &fData[fileSize]; c < end;) {
			
			if(*c == 'v') {
				c++; 
				if      (*c == ' ') vertexElementsCapacity++;
				else if (*c == 'n') normalsCapacity++;
				c += 6; // 6 = minimum line length after 'v' so we jump it.
				skipLine(c, end); 
			}
			else if (*c == 'f') {  
				c++; 
				for (uint32_t indexCount = 0; *c != '\n' && c < end; c++) {
					if (*c == ' ' && charIsDigit(c[1])) {
						indexCount++;  
      					indexElementsCapacity += indexCount > 3 ? 3 : 1;
					}
				}
			}
			else {
				skipLine(c, end); 
			}

		}
	}

	if (indexElementsCapacity == 0) return Success::NO_DATA_FROM_FILE; 

	uint32_t predictedUniqueVertexCount = (uint32_t)(0.9f * indexElementsCapacity); 
	indexBuff->reserve(indexElementsCapacity); 
	vertexBuff->reserve(predictedUniqueVertexCount);

	// Parsing / reading
	if (stlFile) {
		Map<Vertex, uint32_t> uniqueVertices((size_t)(1.5 * indexElementsCapacity), predictedUniqueVertexCount);
		if (*isTextFormat) {
			constexpr size_t floatsPerFacet = 12;
			float facet[floatsPerFacet];
			uint32_t floatIndex = 0;
			for (char* c = &fData[0], *end = &fData[fileSize]; c < end; c++) {
				if (*c != '.') continue;

				// first decimal place
				float value = (float)(c[-1] - '0');
				// handle negative
				float negative = c[-2] == '-' ? -1.0f : 1.0f;
				c++;
				// Convert significand to float
				for (float place = 0.1f; *c != 'e'; place *= 0.1f, c++) {
					value += place * (*c - '0');
				}
				value *= negative;

				// Apply exponent
				c += 4;
				for (; charIsDigit(*c); c++);
				int exponent = 0;
				uint32_t digitBase = 1;
				char* ex_c = &c[-1];
				for (; *ex_c != '-' && *ex_c != '+'; --ex_c, digitBase *= 10) {
					exponent += digitBase * (*ex_c - '0');
				}
				exponent *= *ex_c == '-' ? -1 : 1;
				value *= powf(10.0f, (float)exponent);

				facet[floatIndex] = value;
				floatIndex++;
				if (floatIndex < floatsPerFacet) continue;

				floatIndex = 0;

				Vertex v(*(vec3*)&facet[3], *(vec3*)&facet[0]);
				addVertex(v, uniqueVertices, *vertexBuff, *indexBuff);

				v.pos = *(vec3*)&facet[6];
				addVertex(v, uniqueVertices, *vertexBuff, *indexBuff);

				v.pos = *(vec3*)&facet[9];
				addVertex(v, uniqueVertices, *vertexBuff, *indexBuff);

			}
		} 
		// binary STL
		else {
			for (char* pFacet = &fData[84], *end = &fData[fileSize]; pFacet < end; pFacet += 50) {

				Vertex v(*(vec3*)&pFacet[12], *(vec3*)&pFacet[0]);
				addVertex(v, uniqueVertices, *vertexBuff, *indexBuff);

				v.pos = *(vec3*)&pFacet[24];
				addVertex(v, uniqueVertices, *vertexBuff, *indexBuff);

				v.pos = *(vec3*)&pFacet[36];
				addVertex(v, uniqueVertices, *vertexBuff, *indexBuff);

			}
		}
	}
	// If (objFile)
	else {

		vec3* const vertexPositions           = (vec3*)malloc(sizeof(vec3) * vertexElementsCapacity); assert(vertexPositions != nullptr); 
		vec3*       vertexPositionsNewElement = vertexPositions; 
		vec3* const vertexNormals             = normalsCapacity > 0 ? (vec3*)malloc(sizeof(vec3) * normalsCapacity) : nullptr; 
		vec3*       vertexNormalsNewElement   = vertexNormals; 

		for (const char* c = &fData[0], *end = &fData[fileSize]; c < end;) {
			if (*c != 'v') { skipLine(c, end); continue; }

			c++;
			if (*c == ' ') { 
				c++; 
				skipWhitespace(c); 
				objGetVec3FromText(c, end, (float*)vertexPositionsNewElement);
				vertexPositionsNewElement++; 
			}
			else if (*c == 'n') {
				c += 2; 
				skipWhitespace(c); 
				objGetVec3FromText(c, end, (float*)vertexNormalsNewElement);
  				vertexNormalsNewElement++; 
			}

		}
		if (normalsCapacity > 0) {

			Map<ObjVertexIndex, uint32_t> uniqueVertices((size_t)(1.5 * indexElementsCapacity), predictedUniqueVertexCount);

			for (const char* c = &fData[0], *end = &fData[fileSize]; c < end;) {
				if (*c != 'f') { skipLine(c, end); continue; }

				c += 2;
				skipWhitespace(c);
				uint32_t vertexCountInFacet = 0;
				for (; c < end && charIsDigit(*c); c++) {
					mload::ObjVertexIndex vertexIndex;
					objGetIndexFromText(c, end, &vertexIndex);
					bool keyExists;
					uint32_t* pIndex = uniqueVertices.getKeyValue(vertexIndex, &keyExists);
					if (!keyExists) {
						*pIndex = (uint32_t)vertexBuff->size();
						// index - 1 to convert to 0 based indexing (.obj format doesn't use 0 based indexing)
						vertexBuff->emplace_back(vertexPositions[vertexIndex.posIndex - 1], vertexNormals[vertexIndex.normalIndex - 1]);
					}
					vertexCountInFacet++;
					if (vertexCountInFacet > 3) {
						indexBuff->push_back((*indexBuff)[indexBuff->size() - 3 * (vertexCountInFacet - 3)]);
						indexBuff->push_back((*indexBuff)[indexBuff->size() - 2]);
					}
					indexBuff->push_back(*pIndex);

				}
			}

		}
		else {

			Map<Vertex, uint32_t> uniqueVertices((size_t)(1.5 * indexElementsCapacity), predictedUniqueVertexCount);

			for (const char* c = &fData[0], *end = &fData[fileSize]; c < end;) {
				if (*c != 'f') { skipLine(c, end); continue; }

				c += 2;
				uint32_t facetIndices[3]; 
				for (int facetIndex = 0; facetIndex < 3; facetIndex++) {
					skipWhitespace(c);
					facetIndices[facetIndex] = getVertexIndexFromVertexReference(c); 
				}
				const glm::vec3& p1 = *(glm::vec3*)&vertexPositions[facetIndices[0] - 1];
				const glm::vec3& p2 = *(glm::vec3*)&vertexPositions[facetIndices[1] - 1];
				const glm::vec3& p3 = *(glm::vec3*)&vertexPositions[facetIndices[2] - 1];
				Vertex v; 
				v.pos    = *(vec3*)&p1;
				v.normal = *(vec3*)&glm::normalize(glm::cross(p2 - p1, p3 - p1)); 
				addVertex(v, uniqueVertices, *vertexBuff, *indexBuff); 

				v.pos = *(vec3*)&p2;
				addVertex(v, uniqueVertices, *vertexBuff, *indexBuff);

				v.pos = *(vec3*)&p3;
				addVertex(v, uniqueVertices, *vertexBuff, *indexBuff);
				
				// if there are more than 3 vertex references in a facet
				for (int fanCenterIndexOffset = 3; *c == ' ' && charIsDigit(c[1]); fanCenterIndexOffset += 3) {
					c++;
					uint32_t vertexIndex = getVertexIndexFromVertexReference(c); 

					v.pos = vertexPositions[vertexIndex - 1];
					bool keyExists; 
					uint32_t* pIndex = uniqueVertices.getKeyValue(v, &keyExists);
					if (!keyExists) {
						*pIndex = (uint32_t)vertexBuff->size();
						vertexBuff->push_back(v);
					}
					indexBuff->push_back((*indexBuff)[indexBuff->size() - fanCenterIndexOffset]);
					indexBuff->push_back((*indexBuff)[indexBuff->size() - 2]);
					indexBuff->push_back(*pIndex);
				
				}

			}

		}

		free(vertexPositions); 
		if (vertexNormals != nullptr) free(vertexNormals); 

	}

	return Success::SUCCESS;
}