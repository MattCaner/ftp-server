#pragma once
#include"includes.hpp"


template <typename Out>
void split(const std::string &s, char delim, Out result) {
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
        *result++ = item;
    }
}

std::vector<std::string> split(const std::string &s, char delim);
bool endsWith (std::string const &fullString, std::string const &ending);
bool isSafePath(const fsys::path &root, const fsys::path &child);