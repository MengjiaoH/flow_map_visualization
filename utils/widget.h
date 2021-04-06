#pragma once 

#include <iostream>
#include <mutex>
#include <vector> 

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

class Widget{
    int start_time = 0;
    int end_time = 0;
    int pre_time = 0;
    bool time_changed = false;
    std::mutex lock; 

    public: 
        float time_now = 0;
        int pre_time_now = 0;
        bool animation_time_changed = false;
        int current_time = 0;
        bool animation = false;
        Widget(int begin, int end, int default_start);
        void draw();
        bool changed();
};