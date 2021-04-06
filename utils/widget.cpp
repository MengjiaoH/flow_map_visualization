#include "widget.h"

Widget::Widget(int begin, int end, int default_start)
    :start_time{begin}, end_time(end), current_time(default_start)
{}

void Widget::draw()
{
    ImGui::SliderInt("Time Slider", &current_time, start_time, end_time); 
    
    if(current_time != pre_time){
        time_changed = true;
    }else{
        // std::cout << "current time step " << currentTimeStep << " and pre time step " << preTimeStep << std::endl; 
        time_changed = false;
    }

    if(time_changed && lock.try_lock()){
        // std::cout << "current time step " << currentTimeStep << " and pre time step " << preTimeStep << std::endl; 
        pre_time = current_time;
        lock.unlock();
    }

    ImGui::Checkbox("Animation", &animation); 
    ImGui::SameLine(100); 
    ImGui::InputFloat("Time: ", &time_now);
    if(time_now != pre_time_now){
        animation_time_changed = true;
    }else{
        animation_time_changed = false;
    }

    if(animation_time_changed && lock.try_lock()){
        // std::cout << "current time step " << currentTimeStep << " and pre time step " << preTimeStep << std::endl; 
        pre_time_now = time_now;
        lock.unlock();
    }
}

bool Widget::changed(){
    return time_changed;
}