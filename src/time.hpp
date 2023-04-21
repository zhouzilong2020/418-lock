#ifndef __MY_TIME_HPP__
#define __MY_TIME_HPP__
#include <chrono>
#include <ctime>
#include <iostream>

std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    return std::ctime(&now_time);
}
#endif