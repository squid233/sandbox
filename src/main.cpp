#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

import std;
import sandbox.gl.buffer;
import sandbox.gl.descriptor;
import sandbox.gl.pipeline;
import sandbox.io;
import sandbox.log;
import sandbox.opengl;
import sandbox.tinyfd;

constexpr int WIDTH = 854;
constexpr int HEIGHT = 480;
GLFWwindow* window = nullptr;
int framebufferWidth = WIDTH;
int framebufferHeight = HEIGHT;
bool framebufferResized = false;

double lastFrameTime = 0;

glm::dvec3 previousPosition{};
glm::dvec3 position{};

struct Vertex {
    glm::vec3 position;
    glm::vec4 color;
};

constexpr std::array vertices = {
    Vertex{{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
    Vertex{{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
    Vertex{{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
    Vertex{{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
};
constexpr std::array indices = {
    0, 1, 2, 2, 3, 0
};

struct Transformation {
    glm::mat4 Projection;
    glm::mat4 View;
    glm::mat4 Model;
};

constexpr sandbox::gl::DescriptorSetLayoutBinding transformationSetLayoutBindings[] = {
    {
        .binding = 0,
        .type = sandbox::gl::DescriptorType::UNIFORM_BUFFER
    }
};
sandbox::gl::DescriptorSet transformationDescriptorSet{
    sandbox::gl::DescriptorSetLayoutInfo{
        .bindingCount = sizeof(transformationSetLayoutBindings) / sizeof(sandbox::gl::DescriptorSetLayoutBinding),
        .bindings = transformationSetLayoutBindings
    }};
sandbox::gl::Buffer* transformationBuffer = nullptr;
Transformation* transformationBufferData = nullptr;
Transformation transformation{};

sandbox::gl::GraphicsPipeline* pipeline = nullptr;
sandbox::gl::Buffer* vertexBuffer = nullptr;
sandbox::gl::Buffer* indexBuffer = nullptr;

sandbox::gl::CommandBuffer commandBuffer{};

void errorCallback(int errorCode, const char* description) noexcept {
    sandbox::log::error(std::format("GLFW error {}: {}", errorCode, description));
}

#ifdef _DEBUG
std::string debugSource(const GLenum source) {
    switch (source) {
        case GL_DEBUG_SOURCE_API: return "API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "Window System";
        case GL_DEBUG_SOURCE_SHADER_COMPILER: return "Shader Compiler";
        case GL_DEBUG_SOURCE_THIRD_PARTY: return "Third Party";
        case GL_DEBUG_SOURCE_APPLICATION: return "Application";
        case GL_DEBUG_SOURCE_OTHER: return "Other";
        default: return std::format("GLDebugSource {:#x}", source);
    }
}

std::string debugType(GLenum type) {
    switch (type) {
        case GL_DEBUG_TYPE_ERROR: return "Error";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "Deprecated Behavior";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "Undefined Behavior";
        case GL_DEBUG_TYPE_PORTABILITY: return "Portability";
        case GL_DEBUG_TYPE_PERFORMANCE: return "Performance";
        case GL_DEBUG_TYPE_OTHER: return "Other";
        default: return std::format("GLDebugType {:#x}", type);
    }
}

std::string debugSeverity(GLenum severity) {
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH: return "High";
        case GL_DEBUG_SEVERITY_MEDIUM: return "Medium";
        case GL_DEBUG_SEVERITY_LOW: return "Low";
        case GL_DEBUG_SEVERITY_NOTIFICATION: return "Notification";
        default: return std::format("GLDebugSeverity {:#x}", severity);
    }
}

void setupGLDebugCallback() {
    glDebugMessageCallback([](const GLenum source, const GLenum type, const GLuint id, const GLenum severity, GLsizei, const GLchar* message, const void*) {
        sandbox::log::func_t func;
        if (severity == GL_DEBUG_SEVERITY_HIGH) {
            func = sandbox::log::error;
        } else if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
            func = sandbox::log::warn;
        } else {
            func = sandbox::log::info;
        }
        func(std::format(R"(Debug message from OpenGL
Source: {}
Type: {}
ID: {:#x}
Severity: {}
Message: {})",
        debugSource(source),
        debugType(type),
        id,
        debugSeverity(severity),
        message));
    }, nullptr);
}
#endif

bool initGL() {
    glfwMakeContextCurrent(window);
    gladLoadGLLoader([](const char* name) noexcept -> void* { return glfwGetProcAddress(name); });

#ifdef _DEBUG
    setupGLDebugCallback();
#endif

    transformationBuffer = new sandbox::gl::Buffer();
    transformationBuffer->storage(sizeof(Transformation), nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    transformationBufferData = static_cast<Transformation*>(transformationBuffer->map(GL_WRITE_ONLY));

    const sandbox::gl::DescriptorBufferInfo bufferInfo{
        .buffer = transformationBuffer,
        .offset = 0,
        .range = sizeof(Transformation),
    };
    const sandbox::gl::WriteDescriptorSet write{
        .dstBinding = 0,
        .descriptorType = sandbox::gl::DescriptorType::UNIFORM_BUFFER,
        .bufferInfo = &bufferInfo,
    };
    transformationDescriptorSet.update(1, &write);

    constexpr auto bindingDescription = sandbox::gl::BindingDescription{
        .binding = 0,
        .stride = sizeof(Vertex),
    };
    constexpr sandbox::gl::AttributeDescription attributeDescriptions[] = {
        {0, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position)},
        {1, 0, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, color)},
    };
    pipeline = new sandbox::gl::GraphicsPipeline({
        .vertexShaderFilename = "res/shader/shader.vert",
        .fragmentShaderFilename = "res/shader/shader.frag",
        .bindingDescriptionCount = 1,
        .bindingDescriptions = &bindingDescription,
        .attributeDescriptionCount = 2,
        .attributeDescriptions = attributeDescriptions,
    });
    if (!pipeline->create()) {
        return false;
    }

    vertexBuffer = new sandbox::gl::Buffer();
    indexBuffer = new sandbox::gl::Buffer();

    vertexBuffer->storage(sizeof(vertices), vertices.data(), 0);
    indexBuffer->storage(sizeof(indices), indices.data(), 0);

    return true;
}

void update(const double deltaTime) {
    previousPosition = position;

    double deltaX = 0, deltaY = 0, deltaZ = 0;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) --deltaZ;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) ++deltaZ;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) --deltaX;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) ++deltaX;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) --deltaY;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) ++deltaY;
    const double speed = 2.0f * deltaTime;
    position += speed * glm::dvec3(deltaX, deltaY, deltaZ);
}

void render() {
    if (framebufferResized) {
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        glViewport(0, 0, framebufferWidth, framebufferHeight);
        framebufferResized = false;
    }

    transformation.Projection = glm::perspective(
        glm::radians(70.0f),
        static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight),
        0.01f,
        100.0f
    );
    transformation.View = glm::lookAt(
        glm::vec3(position),
        glm::vec3(position) - glm::vec3(0, 0, 1),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    transformation.Model = glm::identity<glm::mat4>();
    std::memcpy(transformationBufferData, &transformation, sizeof(Transformation));

    sandbox::gl::RenderingAttachmentInfo colorAttachments[] = {
        {
            .format = sandbox::gl::Format::B8G8R8A8_UNORM,
            .loadOp = sandbox::gl::AttachmentLoadOp::CLEAR,
            .clearValue = {{{0.4f, 0.6f, 0.9f, 1.0f}}}
        }
    };
    sandbox::gl::RenderingAttachmentInfo depthAttachment = {
        .format = sandbox::gl::Format::D24_UNORM_S8_UINT,
        .loadOp = sandbox::gl::AttachmentLoadOp::CLEAR,
        .clearValue = { .depthStencil = { .depth = 1.0f } }
    };
    commandBuffer.beginRenderPass({
       .colorAttachmentCount = 1,
       .colorAttachments = colorAttachments,
       .depthAttachment = &depthAttachment,
       .stencilAttachment = nullptr
    });
    commandBuffer.bindGraphicsPipeline(pipeline);
    commandBuffer.bindDescriptorSet(transformationDescriptorSet);
    commandBuffer.bindVertexBuffer(0, *vertexBuffer, 0);
    commandBuffer.bindIndexBuffer(*indexBuffer);
    commandBuffer.drawIndexed(GL_TRIANGLES, static_cast<const GLsizei>(indices.size()), GL_UNSIGNED_INT);
    commandBuffer.endRenderPass();

    glfwSwapBuffers(window);
}

void dispose() {
    transformationBuffer->unmap();
    delete transformationBuffer;
    delete vertexBuffer;
    delete indexBuffer;
    delete pipeline;

    glfwDestroyWindow(window);
    glfwTerminate();
}

#if !defined(_WIN32) || defined(_DEBUG)
int main(int argc, char* argv[])
#else
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd
)
#endif
{
    sandbox::log::configure();

    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) {
        const char* description;
        int error = glfwGetError(&description);
        sandbox::tinyfd::errorMessageBox(std::format(R"(Failed to initialize GLFW
GLFW error {}: {})", error, description).c_str());
        return -1;
    }
    if (const auto & videoMode = glfwGetVideoMode(glfwGetPrimaryMonitor()); videoMode != nullptr) {
        glfwWindowHint(GLFW_POSITION_X, (videoMode->width - WIDTH) / 2);
        glfwWindowHint(GLFW_POSITION_Y, (videoMode->height - HEIGHT) / 2);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Sandbox", nullptr, nullptr);
    if (window == nullptr) {
        const char* description;
        int error = glfwGetError(&description);
        sandbox::tinyfd::errorMessageBox(std::format(R"(Failed to create GLFW window
GLFW error {}: {})", error, description).c_str());
        glfwTerminate();
        return -1;
    }
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int, int) noexcept {
        framebufferResized = true;
    });

    if (initGL()) {
        lastFrameTime = glfwGetTime();

        while (!glfwWindowShouldClose(window)) {
            const double currentTime = glfwGetTime();
            const double deltaTime = currentTime - lastFrameTime;
            lastFrameTime = currentTime;
            glfwPollEvents();
            update(deltaTime);
            if (glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
                continue;
            }
            render();
        }
    }

    dispose();
    return 0;
}
