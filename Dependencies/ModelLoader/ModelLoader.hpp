#pragma once

#include "VertexMap.hpp"

namespace mload {

	enum Success {

		SUCCESS = 0, 
		WRONG_FILE_FORMAT, // Must be obj or stl. 
		COULD_NOT_OPEN_FILE, // fopen from cstdio returned nullptr (failed) 

	};

	/// @param  fileName filePath to open
	/// @param  vertexBuff
	/// @param  indexBuff
	/// @param  isAscii determines if the type of the file is encoded in text format
	/// @return view mload::success enum for possible return values; 
	Success openModel(const char* fileName, std::vector<Vertex>* vertexBuff, std::vector<uint32_t>* indexBuff, bool* isTextFormat);

}