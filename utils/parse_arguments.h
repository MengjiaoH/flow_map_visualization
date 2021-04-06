#pragma once
#include <iostream>
#include <string>
#include <vector>

#include "rkcommon/math/vec.h"

using namespace rkcommon::math;

struct Args
{
    std::vector<std::string> flow_map_filenames;
    std::string dtype;
};

void parseArgs(int argc, const char **argv, Args &args);

std::string getFileExt(const std::string& s);