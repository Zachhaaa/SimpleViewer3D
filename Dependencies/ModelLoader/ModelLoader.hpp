#pragma once

#include "VertexMap.hpp"

namespace mload {

	/// @param fileName filePath to open
	/// @param vertexBuff
	/// @param indexBuff
	/// @param isAscii determines if the type of the file is encoded in text format
	/// @return true = success, false = failure
	bool openModel(const char* fileName, std::vector<Vertex>* vertexBuff, std::vector<uint32_t>* indexBuff, bool* isTextFormat);

}