#include <iostream>
#include <vector> 
#include <dirent.h>
#include <time.h>  

// OpenGL
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
// ospray
#include "ospray/ospray_cpp.h"
#include "ospray/ospray_cpp/ext/rkcommon.h"
// imgui 
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
// utils
#include "callbacks.h"
#include "shader.h"
#include "ArcballCamera.h"
#include "points_loader.h"
#include "parse_arguments.h"
#include "widget.h"

using namespace rkcommon::math;

const std::string fullscreen_quad_vs = R"(
#version 420 core
const vec4 pos[4] = vec4[4](
        vec4(-1, 1, 0.5, 1),
        vec4(-1, -1, 0.5, 1),
        vec4(1, 1, 0.5, 1),
        vec4(1, -1, 0.5, 1)
);
void main(void){
        gl_Position = pos[gl_VertexID];
}
)";

const std::string display_texture_fs = R"(
#version 420 core
layout(binding=0) uniform sampler2D img;
out vec4 color;
void main(void){ 
        ivec2 uv = ivec2(gl_FragCoord.xy);
        color = texelFetch(img, uv, 0);
})";

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int argc, const char **argv)
{
    // Parse Arguments
    Args args;
    parseArgs(argc, argv, args);
    // Load All Flow Maps
    std::vector<FlowMap> flowmaps;
    for(const auto &dir : args.flow_map_filenames){
        DIR *dp = opendir(dir.c_str());
        if (!dp) {
            throw std::runtime_error("failed to open directory: " + dir);
        }
        for(dirent *e = readdir(dp); e; e = readdir(dp)){
            std::string name = e ->d_name;
            std::string ext = getFileExt(name);
            if (ext == "raw"){
                const int timestep = std::stoi(name.substr(9, 11));
                std::cout << name << " " << timestep << std::endl;
                const std::string filename = dir + "/" + name;
                FlowMap flow_map = load_points_from_raw(filename, args.dtype);
                flow_map.time = timestep;
                flowmaps.push_back(flow_map);
            }

        }
    }
    std::sort(flowmaps.begin(), flowmaps.end(), sort_by_timestep());
    // for(int i = 0; i < flowmaps.size(); i++){
    //     std::cout << flowmaps[i].time << "\n";
    // }
    vec2i window_size{1024, 512};
    box3f worldBound = box3f{vec3f{0, 0, 10}, vec3f{2, 1, 10}};
    ArcballCamera arcballCamera(worldBound, window_size);
    std::shared_ptr<App> app;
    app = std::make_shared<App>(window_size, arcballCamera);
    Widget widget(0, flowmaps.size()-1, 0);

    
    // Initialize OSPRay
    OSPError initError = ospInit(&argc, argv);
    if (initError != OSP_NO_ERROR)
        throw std::runtime_error("OSPRay not initialized correctly!");
    OSPDevice device = ospGetCurrentDevice();
    if (!device)
        throw std::runtime_error("OSPRay device could not be fetched!");
    
    //!! Create GLFW Window
    GLFWwindow* window;
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()){
        std::cout << "Cannot Initialize GLFW!! " << std::endl;
        exit(EXIT_FAILURE);
    }else{
        std::cout << "Initialize GLFW!! " << std::endl;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

    window = glfwCreateWindow(window_size.x, window_size.y, "OSPRay Viewer", NULL, NULL);

    if (!window){
        std::cout << "Why not opening a window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }else{
        std::cout << "Aha! Window opens successfully!!" << std::endl;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (gl3wInit()) {
        fprintf(stderr, "failed to initialize OpenGL\n");
        return -1;
    }
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    Shader display_render(fullscreen_quad_vs, display_texture_fs);

    GLuint render_texture;
    glGenTextures(1, &render_texture);
    glBindTexture(GL_TEXTURE_2D, render_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, window_size.x, window_size.y);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLuint vao;
    glCreateVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DEPTH_TEST);
    
    {
        //! Create and Setup Camera
        ospray::cpp::Camera camera("perspective");
        camera.setParam("aspect", window_size.x / (float)window_size.y);
        camera.setParam("position", arcballCamera.eyePos());
        camera.setParam("direction", arcballCamera.lookDir());
        camera.setParam("up", arcballCamera.upDir());
        camera.commit(); // commit each object to indicate modifications are done

        // Create spheres
        ospray::cpp::Geometry sphereGeometry("sphere");
        sphereGeometry.setParam("sphere.position", ospray::cpp::CopiedData(flowmaps[0].points));
        sphereGeometry.setParam("radius", 0.01f);
        sphereGeometry.commit();

        ospray::cpp::GeometricModel model(sphereGeometry);
        model.setParam("color", ospray::cpp::CopiedData(vec4f(0.12f, 0.5f, 0.69f, 1.f)));
        model.commit();

        ospray::cpp::Group group;
        group.setParam("geometry", ospray::cpp::CopiedData(model));
        group.commit();

        ospray::cpp::Instance instance(group);
        instance.commit();

        //! Put the Instance in the World
        ospray::cpp::World world;
        world.setParam("instance", ospray::cpp::CopiedData(instance));
        
        // Lights
        std::vector<ospray::cpp::Light> lights;
        {
            ospray::cpp::Light light("ambient");
            light.setParam("intensity", 0.4f);
            // light.setParam("color", vec3f(1.f));
            // light.setParam("visible", true);
            light.commit();
            lights.push_back(light);
        }
        {
            ospray::cpp::Light light("distant");
            light.setParam("intensity", 0.4f);
            light.setParam("direction", vec3f(0.f));
            light.commit();
            lights.push_back(light);
        }

        world.setParam("light", ospray::cpp::CopiedData(lights));
        world.commit();

        //! Create Renderer, Choose Scientific Visualization Renderer
        ospray::cpp::Renderer renderer("scivis");

        //! Complete Setup of Renderer
        renderer.setParam("aoSamples", 1);
        renderer.setParam("shadows", true);
        renderer.setParam("backgroundColor", vec4f(0.f, 0.f, 0.f, 1.f)); // white, transparent
        renderer.setParam("pixelFilter", "gaussian");
        renderer.commit();

        //! Create and Setup Framebuffer
        ospray::cpp::FrameBuffer framebuffer(window_size.x, window_size.y, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM);
        framebuffer.clear();

        glfwSetWindowUserPointer(window, app.get());
        glfwSetCursorPosCallback(window, cursorPosCallback);
        while (!glfwWindowShouldClose(window))
        {
            app->isTimeSliderChanged = widget.changed();

            if (app ->isCameraChanged) {
                camera.setParam("position", app->camera.eyePos());
                camera.setParam("direction", app->camera.lookDir());
                camera.setParam("up", app->camera.upDir());
                // std::cout << "camera pos " << app->camera.eyePos() << std::endl;
                // std::cout << "camera look dir " << app->camera.lookDir() << std::endl;
                // std::cout << "camera up dir " << app->camera.upDir() << std::endl;
                camera.commit();
                framebuffer.clear();
                app ->isCameraChanged = false;
            }
            if(app->isTimeSliderChanged){
                std::cout << widget.current_time << "\n";
                sphereGeometry.setParam("sphere.position", ospray::cpp::CopiedData(flowmaps[widget.current_time].points));
                sphereGeometry.commit();
                model.commit();
                group.commit();
                instance.commit();
                world.commit();
                framebuffer.clear();
                app ->isTimeSliderChanged = false;
            }
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            if (ImGui::Begin("Control Panel")) {
                widget.draw();
            }
            ImGui::End();
            ImGui::Render();
            if (widget.animation){
                for(int i = 0; i < flowmaps.size(); i++){
                    ImGui_ImplOpenGL3_NewFrame();
                    ImGui_ImplGlfw_NewFrame();
                    ImGui::NewFrame();
                    if (ImGui::Begin("Control Panel")) {
                        widget.draw();
                    }
                    ImGui::End();
                    ImGui::Render();


                    widget.time_now = i;
                    std::cout << "flow map " << i << std::endl;
                    std::cout << "\n";
                    sphereGeometry.setParam("sphere.position", ospray::cpp::CopiedData(flowmaps[i].points));
                    sphereGeometry.commit();
                    model.commit();
                    group.commit();
                    instance.commit();
                    world.commit();
                    framebuffer.clear();
                    // render one frame
                    clock_t t0 = clock();
                    float t = 0;
                    while((t / CLOCKS_PER_SEC) < 3){
                        framebuffer.renderFrame(renderer, camera, world);
                        t = (float)(clock() - t0);
                        std::cout << "t " << t / CLOCKS_PER_SEC<< std::endl;
                        
                        uint32_t *fb = (uint32_t *)framebuffer.map(OSP_FB_COLOR);

                        glViewport(0, 0, window_size.x, window_size.y);
                        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, window_size.x, window_size.y, GL_RGBA, GL_UNSIGNED_BYTE, fb);
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                        glUseProgram(display_render.program);
                        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                        framebuffer.unmap(fb);
                        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                        glfwSwapBuffers(window);
                        glfwPollEvents();
                        if (!widget.animation){
                            break;
                        }
                    }

                }
                widget.animation = false;
                std::cout << "animation" << std::endl;
            }else{
                // // Start the Dear ImGui frame
                // ImGui_ImplOpenGL3_NewFrame();
                // ImGui_ImplGlfw_NewFrame();
                // ImGui::NewFrame();

                // if (ImGui::Begin("Control Panel")) {
                //     widget.draw();
                // }
                // ImGui::End();
                // ImGui::Render();
                // render one frame
                widget.time_now = widget.current_time;
                framebuffer.renderFrame(renderer, camera, world);
                uint32_t *fb = (uint32_t *)framebuffer.map(OSP_FB_COLOR);

                glViewport(0, 0, window_size.x, window_size.y);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, window_size.x, window_size.y, GL_RGBA, GL_UNSIGNED_BYTE, fb);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glUseProgram(display_render.program);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                framebuffer.unmap(fb);
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                glfwSwapBuffers(window);
                glfwPollEvents();
            }
        }
    }
    
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    ospShutdown();

    return 0;
}
