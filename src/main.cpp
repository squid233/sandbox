#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

import std;
import sandbox.file;
import sandbox.log;
import sandbox.tinyfd;

constexpr int WIDTH = 854;
constexpr int HEIGHT = 480;
GLFWwindow* window = nullptr;
bool framebufferResized = false;

GLuint program = 0;
GLuint vao = 0;
GLuint vbo = 0;
GLuint ibo = 0;

struct Vertex {
    glm::vec3 position;
    glm::vec4 color;
};

constexpr std::array vertices = {
    Vertex{{0.0f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.0f}},
    Vertex{{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}},
    Vertex{{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}},
};
constexpr std::array indices = {
    0, 1, 2
};

class GLShaderModule {
    GLenum type_;
    GLuint shader = 0;
public:
    explicit GLShaderModule(const GLenum type) : type_(type) {
        shader = glCreateShader(type);
    }

    ~GLShaderModule() {
        glDeleteShader(shader);
    }

    GLShaderModule(const GLShaderModule&) = delete;
    GLShaderModule& operator=(const GLShaderModule&) = delete;
    GLShaderModule(GLShaderModule&&) = delete;
    GLShaderModule& operator=(GLShaderModule&&) = delete;

    [[nodiscard]] GLuint handle() const {
        return shader;
    }

    [[nodiscard]] GLenum type() const {
        return type_;
    }
};

void errorCallback(int errorCode, const char* description) {
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
    glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
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

bool compileShader(const GLShaderModule& module, const std::string& filename) {
    const auto& optional = sandbox::file::readFile(filename);
    if (!optional.has_value()) {
        return false;
    }
    const auto& string = optional.value();
    const char* const cStr = string.c_str();

    const GLuint handle = module.handle();
    glShaderSource(handle, 1, &cStr, nullptr);
    glCompileShader(handle);
    int success;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &success);
    if (!success) {
        int infoLogLength;
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &infoLogLength);
        const auto infoLog = new char[infoLogLength + 1];
        glGetShaderInfoLog(handle, infoLogLength, &infoLogLength, infoLog);
        sandbox::log::error(std::format("Failed to compile {} shader {}: {}", module.type(), handle, infoLog));
        delete[] infoLog;
        return false;
    }
    return true;
}

template<typename T, GLsizeiptr count>
void bufferStorage(const GLuint buffer, std::array<T, count> data, const GLbitfield flags) {
    glNamedBufferStorage(buffer, count * sizeof(T), data.data(), flags);
}

void initGL() {
    glfwMakeContextCurrent(window);
    gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));

#ifdef _DEBUG
    setupGLDebugCallback();
#endif

    program = glCreateProgram();
    const GLShaderModule vertexShaderModule{GL_VERTEX_SHADER};
    const GLShaderModule fragmentShaderModule{GL_FRAGMENT_SHADER};
    compileShader(vertexShaderModule, "res/shader/shader.vert");
    compileShader(fragmentShaderModule, "res/shader/shader.frag");
    glAttachShader(program, vertexShaderModule.handle());
    glAttachShader(program, fragmentShaderModule.handle());
    glLinkProgram(program);
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        int infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        const auto infoLog = new char[infoLogLength + 1];
        glGetProgramInfoLog(program, infoLogLength, &infoLogLength, infoLog);
        sandbox::log::error(std::format("Failed to link program {}: {}", program, infoLog));
        delete[] infoLog;
    }

    glCreateVertexArrays(1, &vao);
    glCreateBuffers(1, &vbo);
    glCreateBuffers(1, &ibo);

    bufferStorage(vbo, vertices, 0);
    bufferStorage(ibo, indices, 0);

    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex));
    glVertexArrayAttribBinding(vao, 0, 0);
    glVertexArrayAttribBinding(vao, 1, 0);
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    glVertexArrayAttribFormat(vao, 1, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, color));
    glEnableVertexArrayAttrib(vao, 0);
    glEnableVertexArrayAttrib(vao, 1);
    glVertexArrayElementBuffer(vao, ibo);
}

void render() {
    if (framebufferResized) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        framebufferResized = false;
    }

    constexpr float clearColor[] = {0.4f, 0.6f, 0.9f, 1.0f};
    glClearNamedFramebufferfv(0, GL_COLOR, 0, clearColor);

    glUseProgram(program);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glUseProgram(0);

    glfwSwapBuffers(window);
}

void dispose() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);
    glDeleteProgram(program);

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
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height) {
        framebufferResized = true;
    });

    initGL();

    while (!glfwWindowShouldClose(window)) {
        render();
        glfwPollEvents();
    }

    dispose();
    return 0;
}
