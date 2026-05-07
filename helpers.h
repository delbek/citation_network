#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <algorithm>
#include <limits>
#include <atomic>

struct Author
{
    std::string omid;
    std::string fullName;
};

struct Paper
{
    std::string omid;
    std::string title;
    std::vector<Author> authors;
    std::string pubDate;
    std::string venue;
    std::string type;
    std::string publisher;
};

inline bool isAuthorOf(const Paper& paper, std::string fullName)
{
    for (const auto& author: paper.authors)
    {
        if (author.fullName == fullName)
        {
            return true;
        }
    }
    return false;
}
