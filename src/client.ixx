module;

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

export module sandbox.client;

import std;
import sandbox.camera;
import sandbox.dialog;
import sandbox.gl;
import sandbox.gl.buffer;
import sandbox.gl.descriptor;
import sandbox.gl.pipeline;
import sandbox.log;
import sandbox.timer;
import sandbox.window;

constexpr int WIDTH = 854;
constexpr int HEIGHT = 480;
constexpr bool VSYNC_ENABLED = false;
constexpr double MOUSE_SENSITIVITY = 0.5;

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

void errorCallback(int errorCode, const char* description) noexcept {
    sandbox::log::error(std::format("GLFW error {}: {}", errorCode, description));
}

void cursorCallback(const sandbox::Mouse*);

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
    glEnable(GL_DEBUG_OUTPUT);
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

namespace sandbox {
    export class Client final : public IMouseAcquirable {
        GLFWwindow* window_ = nullptr;
        Mouse* mouse_ = nullptr;
        int framebufferWidth_ = WIDTH;
        int framebufferHeight_ = HEIGHT;
        bool framebufferResized_ = false;

        timer::FixedUpdateTimer timer_{glfwGetTime, 20};
        std::uint64_t gameTicks_ = 0;

        Camera camera_{};

        gl::DescriptorSetLayoutBinding transformationSetLayoutBindings_[1] = {
            {
                .binding = 0,
                .type = gl::DescriptorType::UNIFORM_BUFFER
            }
        };
        gl::DescriptorSet transformationDescriptorSet_{
            gl::DescriptorSetLayoutInfo{
                .bindingCount = sizeof(transformationSetLayoutBindings_) / sizeof(gl::DescriptorSetLayoutBinding),
                .bindings = transformationSetLayoutBindings_
            }};
        gl::Buffer* transformationBuffer_ = nullptr;
        Transformation* transformationBufferData_ = nullptr;
        Transformation transformation_{};

        gl::GraphicsPipeline* pipeline_ = nullptr;
        gl::Buffer* vertexBuffer_ = nullptr;
        gl::Buffer* indexBuffer_ = nullptr;

        gl::CommandBuffer commandBuffer_{};

        Client() = default;
        ~Client() override = default;

        bool initGL() {
            glfwMakeContextCurrent(window_);
            gladLoadGLLoader([](const char* name) noexcept -> void* { return glfwGetProcAddress(name); });
            glfwSwapInterval(VSYNC_ENABLED ? 1 : 0);

#ifdef _DEBUG
            setupGLDebugCallback();
#endif

            transformationBuffer_ = new gl::Buffer();
            transformationBuffer_->storage(sizeof(Transformation), nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
            transformationBufferData_ = static_cast<Transformation*>(transformationBuffer_->map(GL_WRITE_ONLY));

            const gl::DescriptorBufferInfo bufferInfo{
                .buffer = transformationBuffer_,
                .offset = 0,
                .range = sizeof(Transformation),
            };
            const gl::WriteDescriptorSet write{
                .dstBinding = 0,
                .descriptorType = gl::DescriptorType::UNIFORM_BUFFER,
                .bufferInfo = &bufferInfo,
            };
            transformationDescriptorSet_.update(1, &write);

            constexpr auto bindingDescription = gl::BindingDescription{
                .binding = 0,
                .stride = sizeof(Vertex),
            };
            constexpr gl::AttributeDescription attributeDescriptions[] = {
                {0, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position)},
                {1, 0, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, color)},
            };
            pipeline_ = new gl::GraphicsPipeline({
                .vertexShaderFilename = "res/shader/shader.vert",
                .fragmentShaderFilename = "res/shader/shader.frag",
                .bindingDescriptionCount = 1,
                .bindingDescriptions = &bindingDescription,
                .attributeDescriptionCount = 2,
                .attributeDescriptions = attributeDescriptions,
            });
            if (!pipeline_->create()) {
                return false;
            }

            vertexBuffer_ = new gl::Buffer();
            indexBuffer_ = new gl::Buffer();

            vertexBuffer_->storage(sizeof(vertices), vertices.data(), 0);
            indexBuffer_->storage(sizeof(indices), indices.data(), 0);

            return true;
        }

        void movePlayer() {
            previousPosition_ = position_;

            double deltaX = 0, deltaY = 0, deltaZ = 0;
            if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS) --deltaZ;
            if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS) ++deltaZ;
            if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS) --deltaX;
            if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS) ++deltaX;
            if (glfwGetKey(window_, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) --deltaY;
            if (glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS) ++deltaY;

            const double speed = glfwGetKey(window_, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ? 0.7 : 0.3;
            const double yawRadians = glm::radians(rotation_.y);
            const glm::dvec3 forward{std::cos(yawRadians), 0.0, std::sin(yawRadians)};
            const glm::dvec3 right = glm::normalize(glm::dvec3(std::sin(yawRadians), 0.0, -std::cos(yawRadians)));
            position_ += forward * deltaZ * speed + right * deltaX * speed + glm::dvec3(0.0, deltaY * speed, 0.0);
        }

        void tick() {
            movePlayer();
        }

        void render(const double partialTick) {
            if (framebufferResized_) {
                glfwGetFramebufferSize(window_, &framebufferWidth_, &framebufferHeight_);
                glViewport(0, 0, framebufferWidth_, framebufferHeight_);
                framebufferResized_ = false;
            }

            const auto& lerpPosition = glm::vec3(glm::mix(previousPosition_, position_, partialTick));
            camera_.moveToEntity(lerpPosition, rotation_);

            transformation_.Projection = glm::perspective(
                glm::radians(70.0f),
                static_cast<float>(framebufferWidth_) / static_cast<float>(framebufferHeight_),
                0.01f,
                100.0f
            );
            transformation_.View = camera_.viewMat();
            transformation_.Model = glm::identity<glm::mat4>();
            std::memcpy(transformationBufferData_, &transformation_, sizeof(Transformation));

            gl::RenderingAttachmentInfo colorAttachments[] = {
                {
                    .format = gl::Format::B8G8R8A8_UNORM,
                    .loadOp = gl::AttachmentLoadOp::CLEAR,
                    .clearValue = {{{0.4f, 0.6f, 0.9f, 1.0f}}}
                }
            };
            gl::RenderingAttachmentInfo depthAttachment = {
                .format = gl::Format::D24_UNORM_S8_UINT,
                .loadOp = gl::AttachmentLoadOp::CLEAR,
                .clearValue = { .depthStencil = { .depth = 1.0f } }
            };
            commandBuffer_.beginRenderPass({
               .colorAttachmentCount = 1,
               .colorAttachments = colorAttachments,
               .depthAttachment = &depthAttachment,
               .stencilAttachment = nullptr
            });
            commandBuffer_.bindGraphicsPipeline(pipeline_);
            commandBuffer_.bindDescriptorSet(transformationDescriptorSet_);
            commandBuffer_.bindVertexBuffer(0, *vertexBuffer_, 0);
            commandBuffer_.bindIndexBuffer(*indexBuffer_);
            commandBuffer_.drawIndexed(GL_TRIANGLES, static_cast<const GLsizei>(indices.size()), GL_UNSIGNED_INT);
            commandBuffer_.endRenderPass();

            glfwSwapBuffers(window_);
        }

    public:
        glm::dvec3 previousPosition_{};
        glm::dvec3 position_{0, 0, 3};
        glm::dvec2 rotation_{0, 90};

        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;
        Client(Client&&) = delete;
        Client& operator=(Client&&) = delete;

        [[nodiscard]] static Client& getInstance() {
            static Client instance;
            return instance;
        }

        [[nodiscard]] bool initialize() {
            log::configure();

            glfwSetErrorCallback(errorCallback);
            if (!glfwInit()) {
                const char* description;
                int error = glfwGetError(&description);
                dialog::errorMessageBox(std::format(R"(Failed to initialize GLFW
GLFW error {}: {})", error, description).c_str());
                return false;
            }

            if (const auto& videoMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
                videoMode != nullptr) {
                glfwWindowHint(GLFW_POSITION_X, (videoMode->width - WIDTH) / 2);
                glfwWindowHint(GLFW_POSITION_Y, (videoMode->height - HEIGHT) / 2);
            }
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#ifdef _DEBUG
            glfwWindowHint(GLFW_CONTEXT_DEBUG, GLFW_TRUE);
#endif
            window_ = glfwCreateWindow(WIDTH, HEIGHT, "Sandbox", nullptr, nullptr);
            if (window_ == nullptr) {
                const char* description;
                int error = glfwGetError(&description);
                dialog::errorMessageBox(std::format(R"(Failed to create GLFW window
GLFW error {}: {})", error, description).c_str());
                glfwTerminate();
                return false;
            }
            glfwSetWindowUserPointer(window_, this);

            mouse_ = new Mouse(window_);
            mouse_->setCursorCallback(cursorCallback);
            glfwSetFramebufferSizeCallback(window_, [](GLFWwindow* window, int, int) noexcept {
                const auto& self = static_cast<Client*>(glfwGetWindowUserPointer(window));
                self->framebufferResized_ = true;
            });

            return initGL();
        }

        void run() {
            timer_.advanceTime();
            while (!glfwWindowShouldClose(window_)) {
                timer_.advanceTime();
                for (std::uint32_t i = 0, ticks = timer_.tickCount(); i < ticks; ++i) {
                    tick();
                    ++gameTicks_;
                }
                glfwPollEvents();
                if (!glfwGetWindowAttrib(window_, GLFW_ICONIFIED)) {
                    render(timer_.partialTick());
                    timer_.calculateFps();
                }
            }
        }

        void dispose() {
            transformationBuffer_->unmap();
            delete transformationBuffer_;
            delete vertexBuffer_;
            delete indexBuffer_;
            delete pipeline_;

            mouse_ = nullptr;
            glfwDestroyWindow(window_);
            glfwTerminate();
        }

        Mouse* mouse() override { return mouse_; }
    };
}

void cursorCallback(const sandbox::Mouse* mouse) {
    if (mouse->getButton(GLFW_MOUSE_BUTTON_RIGHT)) {
        auto& client = sandbox::Client::getInstance();
        double pitch = client.rotation_.x - mouse->deltaY() * MOUSE_SENSITIVITY;
        double yaw = client.rotation_.y + mouse->deltaX() * MOUSE_SENSITIVITY;
        pitch = std::clamp(pitch, -90.0, 90.0);
        yaw = std::fmod(yaw, 360.0);
        client.rotation_ = {pitch, yaw};
    }
}
