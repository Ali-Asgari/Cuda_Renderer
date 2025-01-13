#pragma once

#include <glad/glad.h>

class Framebuffer {
public:
    Framebuffer(int width, int height);
    ~Framebuffer();
    void rescale(int width, int height);
    void inline bind() const { glBindFramebuffer(GL_FRAMEBUFFER, framebufferID); }
    void inline unBind() const { glBindFramebuffer(GL_FRAMEBUFFER, 0); }
    void viewPort();
    GLuint textureID;
private:
    GLuint framebufferID;
    GLuint renderbufferID;

    int width, height;
    float ratio;
};