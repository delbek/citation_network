#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <algorithm>
#include "omp.h"

#define DIRECTORY "/arf/scratch/delbek/414/"

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
