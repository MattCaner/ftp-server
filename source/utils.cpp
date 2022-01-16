#include"utils.h"

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

bool endsWith (std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

bool isSafePath(const fsys::path &root, const fsys::path &child) {
    auto const normRoot = fsys::canonical(root);
    auto const normChild = fsys::canonical(child);
    
    auto itr = std::search(normChild.begin(), normChild.end(), 
                           normRoot.begin(), normRoot.end());
    
    return itr == normChild.begin();
}
