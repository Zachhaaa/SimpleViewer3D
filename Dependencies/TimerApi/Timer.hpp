#pragma once

#include <chrono>
#include <thread>
/*
*
* Problem: I hate using the chrono cpp header in practice. 
*   all these namespaces, templates, and constructors when all I want to know is how many seconds has passed between two points in time.
* Solution: Create a wrapper function around that uses the chrono api to get the time. Lol
*
*/

// @return seconds since the start of the program.
double getTime();
inline void sleepFor(double seconds) { if(seconds <= 0.0) return; std::this_thread::sleep_for(std::chrono::nanoseconds(uint64_t(seconds * 1e9))); }