#include<glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <cuda_runtime.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <string>
#include <vector>

#include "renderer/FrameBuffer.h"
#include "renderer/ShaderProgram.h"
#include "renderer/CudaInterop.h"
#include "renderer/sphere.h"

void EnableDockSpace() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBackground;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f,0.0f));
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
    dockspace_flags |= ImGuiDockNodeFlags_PassthruCentralNode;
    bool always_open = true;
    ImGui::Begin("Entire",&always_open,window_flags);
    ImGuiID dockspace_id = ImGui::GetID("DockSpace ");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    ImGui::PopStyleVar(3);
    ImGui::End();
}
void init_world(std::vector<sphere_info>& h_world) {
    sphere_info tempsphere;
    tempsphere.pos = glm::vec3(0.0f, 0.0f, -1.0f);
    tempsphere.radious = 0.2f;
    tempsphere.mat = 0;
    tempsphere.alb = glm::vec3(0.8f, 0.3f, 0.3f);
    tempsphere.fuz = 0.1f;

    h_world.push_back(tempsphere);

    tempsphere.pos = glm::vec3(1.0f, 0.0f, -1.0f);
    tempsphere.radious = 0.2f;
    tempsphere.mat = 1;
    tempsphere.alb = glm::vec3(0.8f, 0.8f, 0.8f);
    tempsphere.fuz = 0.1f;
    h_world.push_back(tempsphere);

    tempsphere.pos = glm::vec3(0.0f, -100.5f, -1.0f);
    tempsphere.radious = 100.0f;
    tempsphere.mat = 0;
    tempsphere.alb = glm::vec3(0.8f, 0.6f, 0.2f);
    tempsphere.fuz = 0.1f;
    h_world.push_back(tempsphere);
}
void interact_with_world(std::vector<sphere_info>& h_world, const char* items[], CudaInterop &interop) {
    bool anychange = false;
    bool temp = false;
    ImGui::Begin("Hierarchy");
    for (int id = 0; id < h_world.size(); id++) {
        ImGui::PushID(id);
        std::string name = "Sphere "+ std::to_string(id);
        if (ImGui::TreeNode(name.c_str())){
            temp = ImGui::DragFloat3("Position: ", &h_world[id].pos[0], 0.1f);
            anychange = anychange || temp;
            temp = ImGui::DragFloat("Radius: ", &h_world[id].radious, 0.1f);
            anychange = anychange || temp;
            if (ImGui::BeginCombo("Material: ", items[h_world[id].mat])){
                for (int i = 0; i < 2; i++){
                    bool is_selected = (h_world[id].mat == i);
                    if (ImGui::Selectable(items[i], is_selected)) {
                        h_world[id].mat = i;
                        anychange = true;
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            temp = ImGui::ColorEdit3("Albedo: ", &h_world[id].alb[0]);
            anychange = anychange || temp;
            if (h_world[id].mat == 1) {
                temp = ImGui::DragFloat("fuzzy: ", &h_world[id].fuz, 0.1f);
                anychange = anychange || temp;
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    ImGui::Separator();
    if (ImGui::Button("Add sphere", ImVec2(-1, 25))) {
        sphere_info tempsphere;
        tempsphere.pos = glm::vec3(0.5f, 0.5f, -1.0f);
        tempsphere.radious = 0.2f;
        tempsphere.mat = 1;
        tempsphere.alb = glm::vec3(0.8f, 0.8f, 0.8f);
        tempsphere.fuz = 0.1f;
        h_world.push_back(tempsphere);
        anychange = true;
    }
    if (anychange) interop.recreate(h_world);
    ImGui::End();
}

int main(void)
{   
    //define rendering resolution
    int width = 1400;
    int height = 700;

    if (!glfwInit())
        return -1;

    //  Using OpenGL core profile 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(640, 480, "Cuda Renderer", NULL, NULL);
    if (!window){
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    Framebuffer sceneFramebuffer(width, height);
    ShaderProgram shaderProgram("src/shaders/default.vert", "src/shaders/default.frag");
    shaderProgram.Activate();

    float vertices[] = {
    //position(x,y) texture(x,y)
    -1.0f, -1.0f, 0.0f, 0.0f, // left down  
     1.0f, -1.0f, 1.0f, 0.0f, // right down
    -1.0f,  1.0f, 0.0f, 1.0f, // left up  
     1.0f,  1.0f, 1.0f, 1.0f  // right up  
    };
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    double lastTime = glfwGetTime();
    int nbFrames = 0;
    float fps = 0;
    float midTime = 1;

    double currentTime;
    double timeDiff = 0;
    bool vSync = false;
    bool changed_vsync = true;

    CudaInterop interop(width,height);

    std::vector <sphere_info> h_world;
    init_world(h_world);

    interop.recreate(h_world);

    const char* items[] = { "lambertian", "Metal" };

    glfwSwapInterval(0);

    while (!glfwWindowShouldClose(window)){
        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        EnableDockSpace();
        
        currentTime = glfwGetTime();
        timeDiff = currentTime - lastTime;
        nbFrames++;

        ImGui::Begin("FPS");
        if (timeDiff >= 1.0f / 5.0f) {
            fps = (float)(1.0f / timeDiff) * nbFrames;
            midTime = (fps > 0) ? 1.0f / fps : 1.0f ;
            nbFrames = 0;
            lastTime = currentTime;
        }
        ImGui::Text("fps: %.2f", fps);
        ImGui::Text("midTime: %.3f ms", midTime*1000);
        if (changed_vsync) {
            if (vSync) glfwSwapInterval(1);
            else glfwSwapInterval(0);
        }
        changed_vsync = ImGui::Checkbox("v-sync", &vSync);

        ImGui::Text("Number of accumulated: %d",interop.num_frame);
        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Scene");
        ImVec2 availableSpace = ImGui::GetContentRegionAvail();
        sceneFramebuffer.rescale((int)availableSpace.x, (int)availableSpace.y);
        ImGui::Image(sceneFramebuffer.textureID, ImGui::GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::End();
        ImGui::PopStyleVar();

        sceneFramebuffer.bind();

        //draw
        glClear(GL_COLOR_BUFFER_BIT);
        sceneFramebuffer.viewPort();
        interop.renderToTexture();
        interop.bindTexture();
        shaderProgram.Activate();
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        sceneFramebuffer.unBind();

        interact_with_world(h_world, items, interop);

        // after all opengl functions and imgui functions 
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    shaderProgram.~ShaderProgram();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}