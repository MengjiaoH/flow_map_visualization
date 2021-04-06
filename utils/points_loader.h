#pragma once 
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "rkcommon/math/vec.h"
using namespace rkcommon::math;

struct FlowMap{
    int time = 0;
    std::vector<vec3f> points;
    // TODO: Add other attributes 
};

FlowMap load_points_from_raw(const std::string filename, const std::string point_type);

struct sort_by_timestep
{
    inline bool operator() (const FlowMap &a, const FlowMap &b) {
        return a.time < b.time;
    }
};