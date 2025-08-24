module;

#include <assert.h>
#include <glad/glad.h>

export module sandbox.opengl;

import std;
import sandbox.file;
import sandbox.log;

namespace sandbox::gl {
    export struct BindingDescription {
        GLuint binding;
        GLsizei stride;
    };

    export struct AttributeDescription {
        GLuint location;
        GLuint binding;
        GLint size;
        GLenum type;
        GLboolean normalized;
        GLuint offset;
    };

    export class ShaderModule {
        GLuint handle_;
        GLenum type_;
    public:
        explicit ShaderModule(const GLenum type) : handle_(glCreateShader(type)), type_(type) {}
        ~ShaderModule() {
            glDeleteShader(handle_);
        }

        ShaderModule(const ShaderModule& other) = delete;
        ShaderModule& operator=(const ShaderModule& other) = delete;
        ShaderModule(ShaderModule&& other) = delete;
        ShaderModule& operator=(ShaderModule&& other) = delete;

        bool compile(const std::string& filename) const {
            const auto& optional = file::readFile(filename);
            if (!optional.has_value()) {
                return false;
            }
            const auto& string = optional.value();
            const char* cStr = string.c_str();

            glShaderSource(handle_, 1, &cStr, nullptr);
            glCompileShader(handle_);

            int success;
            glGetShaderiv(handle_, GL_COMPILE_STATUS, &success);
            if (!success) {
                int infoLogLength;
                glGetShaderiv(handle_, GL_INFO_LOG_LENGTH, &infoLogLength);
                const auto infoLog = new char[infoLogLength + 1];
                glGetShaderInfoLog(handle_, infoLogLength, &infoLogLength, infoLog);
                log::error(std::format("Failed to compile {} shader {}: {}", type_, handle_, infoLog));
                delete[] infoLog;
                return false;
            }

            return true;
        }

        GLuint handle() const { return handle_; }
        GLenum type() const { return type_; }
    };

    export struct PipelineInfo {
        std::string vertexShaderFilename;
        std::string fragmentShaderFilename;
        size_t bindingDescriptionCount;
        const BindingDescription* bindingDescriptions;
        size_t attributeDescriptionCount;
        const AttributeDescription* attributeDescriptions;
    };

    export class GraphicsPipeline {
        PipelineInfo* pipelineInfo_;
        GLuint handle_;
        GLuint vertexArray_;
    public:
        explicit GraphicsPipeline(const PipelineInfo& pipelineInfo) :
            handle_(glCreateProgram()) {
            glCreateVertexArrays(1, &vertexArray_);

            pipelineInfo_ = new PipelineInfo();
            pipelineInfo_->vertexShaderFilename = pipelineInfo.vertexShaderFilename;
            pipelineInfo_->fragmentShaderFilename = pipelineInfo.fragmentShaderFilename;
            pipelineInfo_->bindingDescriptionCount = pipelineInfo.bindingDescriptionCount;
            pipelineInfo_->bindingDescriptions = new BindingDescription[pipelineInfo.bindingDescriptionCount];
            std::memcpy(const_cast<BindingDescription *>(pipelineInfo_->bindingDescriptions),
                        pipelineInfo.bindingDescriptions,
                        pipelineInfo.bindingDescriptionCount * sizeof(BindingDescription));
            pipelineInfo_->attributeDescriptionCount = pipelineInfo.attributeDescriptionCount;
            pipelineInfo_->attributeDescriptions = new AttributeDescription[pipelineInfo.attributeDescriptionCount];
            std::memcpy(const_cast<AttributeDescription *>(pipelineInfo_->attributeDescriptions),
                        pipelineInfo.attributeDescriptions,
                        pipelineInfo.attributeDescriptionCount * sizeof(AttributeDescription));
        }

        ~GraphicsPipeline() {
            glDeleteProgram(handle_);
            glDeleteVertexArrays(1, &vertexArray_);
            delete pipelineInfo_->bindingDescriptions;
            delete pipelineInfo_->attributeDescriptions;
            delete pipelineInfo_;
        }

        GraphicsPipeline(const GraphicsPipeline& other) = delete;
        GraphicsPipeline& operator=(const GraphicsPipeline& other) = delete;
        GraphicsPipeline(GraphicsPipeline&& other) = delete;
        GraphicsPipeline& operator=(GraphicsPipeline&& other) = delete;

        bool create() {
            const ShaderModule vertexShader{GL_VERTEX_SHADER};
            if (!vertexShader.compile(pipelineInfo_->vertexShaderFilename)) {
                return false;
            }
            const ShaderModule fragmentShader{GL_FRAGMENT_SHADER};
            if (!fragmentShader.compile(pipelineInfo_->fragmentShaderFilename)) {
                return false;
            }
            glAttachShader(handle_, vertexShader.handle());
            glAttachShader(handle_, fragmentShader.handle());
            glLinkProgram(handle_);

            int success;
            glGetProgramiv(handle_, GL_LINK_STATUS, &success);
            if (!success) {
                int infoLogLength;
                glGetProgramiv(handle_, GL_INFO_LOG_LENGTH, &infoLogLength);
                const auto infoLog = new char[infoLogLength + 1];
                glGetProgramInfoLog(handle_, infoLogLength, &infoLogLength, infoLog);
                log::error(std::format("Failed to link program {}: {}", handle_, infoLog));
                delete[] infoLog;
            }

            glDetachShader(handle_, vertexShader.handle());
            glDetachShader(handle_, fragmentShader.handle());

            for (size_t i = 0; i < pipelineInfo_->attributeDescriptionCount; ++i) {
                const auto& [location, binding, size, type, normalized, offset] =
                    pipelineInfo_->attributeDescriptions[i];
                glVertexArrayAttribBinding(vertexArray_, location, binding);
                glVertexArrayAttribFormat(vertexArray_, location, size, type, normalized, offset);
                glEnableVertexArrayAttrib(vertexArray_, location);
            }

            return true;
        }

        void bind() const {
            glUseProgram(handle_);
            glBindVertexArray(vertexArray_);
        }

        const PipelineInfo* pipelineInfo() const { return pipelineInfo_; }
        GLuint handle() const { return handle_; }
        GLuint vertexArray() const { return vertexArray_; }
    };

    export union ClearColorValue {
        GLfloat float32[4];
        GLint int32[4];
        GLuint uint32[4];
    };

    export union ClearDepthStencilValue {
        GLfloat depth;
        GLuint stencil;
    };

    export union ClearValue {
        ClearColorValue color;
        ClearDepthStencilValue depthStencil;
    };

    export struct RenderPassBeginInfo {
        GLuint framebuffer;
        size_t clearValueCount;
        const ClearValue* clearValues;
    };

    export using ClearBuffer = std::uint32_t;
    export constexpr ClearBuffer CLEAR_BUFFER_COLOR_BIT = 0b1;
    export constexpr ClearBuffer CLEAR_BUFFER_DEPTH_STENCIL_BIT = 0b10;

    export enum class ClearColorFormat {
        FLOAT = 0,
        INT32 = 1,
        UINT32 = 2,
    };

    export struct AttachmentDescription {
        ClearBuffer clearBuffer;
        ClearColorFormat clearColorFormat;
        bool shouldClear;
    };

    export struct RenderPassInfo {
        size_t attachmentCount;
        const AttachmentDescription* attachments;
    };

    export class CommandBuffer {
        bool isInRenderPass_ = false;
        GraphicsPipeline* graphicsPipeline_ = nullptr;
    public:
        void beginRenderPass(const RenderPassBeginInfo& beginInfo, const RenderPassInfo& renderPassInfo) {
            assert(!isInRenderPass_);
            isInRenderPass_ = true;
            for (size_t i = 0; i < beginInfo.clearValueCount; ++i) {
                const auto& clearValue = beginInfo.clearValues[i];
                if (const auto& attachmentDescription = renderPassInfo.attachments[i]; attachmentDescription.shouldClear) {
                    if ((attachmentDescription.clearBuffer & CLEAR_BUFFER_COLOR_BIT) != 0) {
                        switch (attachmentDescription.clearColorFormat) {
                            case ClearColorFormat::FLOAT:
                                glClearNamedFramebufferfv(beginInfo.framebuffer, GL_COLOR, 0, clearValue.color.float32);
                                break;
                            case ClearColorFormat::INT32:
                                glClearNamedFramebufferiv(beginInfo.framebuffer, GL_COLOR, 0, clearValue.color.int32);
                                break;
                            case ClearColorFormat::UINT32:
                                glClearNamedFramebufferuiv(beginInfo.framebuffer, GL_COLOR, 0, clearValue.color.uint32);
                                break;
                        }
                    }
                    if ((attachmentDescription.clearBuffer & CLEAR_BUFFER_DEPTH_STENCIL_BIT) != 0) {
                        glClearNamedFramebufferfi(beginInfo.framebuffer, GL_DEPTH_STENCIL, 0, clearValue.depthStencil.depth, clearValue.depthStencil.stencil);
                    }
                }
            }
        }

        void bindGraphicsPipeline(GraphicsPipeline* graphicsPipeline) {
            assert(isInRenderPass_);
            graphicsPipeline_ = graphicsPipeline;
        }

        void bindVertexBuffers(const GLuint firstBinding, const GLsizei bindingCount, const GLuint* pBuffers, const GLintptr* pOffsets) const {
            assert(isInRenderPass_);
            assert(graphicsPipeline_ != nullptr);
            const auto& info = graphicsPipeline_->pipelineInfo();
            const auto& strides = new GLsizei[bindingCount]{};
            for (size_t i = 0; i < bindingCount; ++i) {
                for (size_t j = 0; j < info->bindingDescriptionCount; ++j) {
                    if (const auto& [binding, stride] = info->bindingDescriptions[j]; firstBinding + i == binding) {
                        strides[i] = stride;
                    }
                }
            }
            glVertexArrayVertexBuffers(graphicsPipeline_->vertexArray(), firstBinding, bindingCount, pBuffers, pOffsets, strides);
            delete[] strides;
        }

        void bindIndexBuffer(const GLuint buffer) const {
            assert(isInRenderPass_);
            assert(graphicsPipeline_ != nullptr);
            glVertexArrayElementBuffer(graphicsPipeline_->vertexArray(), buffer);
        }

        void drawIndexed(const GLenum mode, const GLsizei count, const GLenum type) const {
            assert(isInRenderPass_);
            assert(graphicsPipeline_ != nullptr);
            graphicsPipeline_->bind();
            glDrawElements(mode, count, type, nullptr);
        }

        void endRenderPass() {
            assert(isInRenderPass_);
            glUseProgram(0);
            glBindVertexArray(0);
            isInRenderPass_ = false;
        }
    };
}
