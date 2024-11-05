#pragma once 

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define arraySize(array) (sizeof(array) / sizeof(array[0]))

/// Must call free() on returned pointer. 
inline char* wcharToChar(const wchar_t* wstr) {

	size_t strLen = wcslen(wstr) + 1; 
	char* astr = (char*)malloc(strLen); 
	assert(astr != NULL); 
	wcstombs(astr, wstr, strLen); 
	return astr;

}
/// Must call free() on returned pointer. 
inline wchar_t* charToWchar(const char* astr) {

	size_t strLen = strlen(astr) + 1; 
	wchar_t* wstr = (wchar_t*)malloc(strLen * sizeof(wchar_t)); 
	assert(wstr != NULL); 
	mbstowcs(wstr, astr, strLen); 
	return wstr; 

}

/// Must call free() on returned pointer. 
inline char* concatA(const char* str1, const char* str2) {

	size_t str1Size = strlen(str1); 
	size_t str2Size = strlen(str2); 
	size_t newStrSize = str1Size + str2Size + 1; 
	char* newStr = (char*)malloc(newStrSize);
	assert(newStr != NULL); 
	memcpy(newStr, str1, str1Size); 
	memcpy(&newStr[str1Size], str2, str2Size); 
	newStr[str1Size + str2Size] = '\0'; 
	return newStr; 

}
/// Must call free() on returned pointer. 
inline wchar_t* concatW(const wchar_t* str1, const wchar_t* str2) {

	size_t str1Size = wcslen(str1);
	size_t str2Size = wcslen(str2);
	size_t newStrSize = str1Size + str2Size + 1;
	wchar_t* newStr = (wchar_t*)malloc(newStrSize * sizeof(wchar_t));
	assert(newStr != NULL);
	memcpy(newStr, str1, str1Size * sizeof(wchar_t));
	memcpy(&newStr[str1Size], str2, str2Size * sizeof(wchar_t));
	newStr[str1Size + str2Size] = '\0';
	return newStr; 
}

/// Must call free() on returned pointer. 
inline char* concatMultipleA(const char* strList[], int stringCount) {

	assert(stringCount > 0); 
	size_t newStrSize = 0; 
	for (int i = 0; i < stringCount; i++) { newStrSize += strlen(strList[i]); }
	newStrSize += 1; 
	char* newStr = (char*)malloc(newStrSize); 
	assert(newStr != NULL); 
	size_t newStrIndex = 0; 
	for (int i = 0; i < stringCount; i++) {
		size_t strSize = strlen(strList[i]);
		memcpy(&newStr[newStrIndex], strList[i], strSize);
		newStrIndex += strSize;

	}
	newStr[newStrSize - 1] = '\0';

	return newStr; 

}
/// Must call free() on returned pointer. 
inline wchar_t* concatMultipleW(const wchar_t* strList[], int stringCount) {

	assert(stringCount > 0); 
	size_t newStrSize = 0;
	for (int i = 0; i < stringCount; i++) { newStrSize += wcslen(strList[i]); }
	newStrSize += 1;
	wchar_t* newStr = (wchar_t*)malloc(newStrSize * sizeof(wchar_t));
	assert(newStr != NULL);
	size_t newStrIndex = 0;
	for (int i = 0; i < stringCount; i++) {
		size_t strSize = wcslen(strList[i]);
		memcpy(&newStr[newStrIndex], strList[i], strSize * sizeof(wchar_t)); 
		newStrIndex += strSize; 
	}
	newStr[newStrSize - 1] = L'\0';

	return newStr;

}