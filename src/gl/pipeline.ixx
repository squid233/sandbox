module;

#include <glad/glad.h>

export module sandbox.gl.pipeline;

import std;
import sandbox.gl.descriptor;
import sandbox.io;
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

        [[nodiscard]] bool compile(const std::string& filename) const {
            const auto& optional = io::readFile(filename);
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
                const auto infoLog = new char[static_cast<std::size_t>(infoLogLength) + 1];
                glGetShaderInfoLog(handle_, infoLogLength, &infoLogLength, infoLog);
                log::error(std::format("Failed to compile {} shader {}: {}", type_, handle_, infoLog));
                delete[] infoLog;
                return false;
            }

            return true;
        }

        [[nodiscard]] GLuint handle() const { return handle_; }
        [[nodiscard]] GLenum type() const { return type_; }
    };

    export struct PipelineInfo {
        const char* vertexShaderFilename;
        const char* fragmentShaderFilename;
        GLuint bindingDescriptionCount;
        const BindingDescription* bindingDescriptions;
        GLuint attributeDescriptionCount;
        const AttributeDescription* attributeDescriptions;
        GLuint setLayoutCount;
        const DescriptorSetLayoutInfo* setLayouts;
    };

    export class GraphicsPipeline {
        PipelineInfo* pipelineInfo_;
        GLuint handle_;
        GLuint vertexArray_ = 0;
    public:
        explicit GraphicsPipeline(const PipelineInfo& pipelineInfo) :
            handle_(glCreateProgram()) {
            glCreateVertexArrays(1, &vertexArray_);

            pipelineInfo_ = new PipelineInfo();
            const auto& vertexShaderFilenameSize = std::strlen(pipelineInfo.vertexShaderFilename) + 1;
            pipelineInfo_->vertexShaderFilename = new char[vertexShaderFilenameSize];
            std::memcpy(const_cast<char*>(pipelineInfo_->vertexShaderFilename),
                pipelineInfo.vertexShaderFilename,
                vertexShaderFilenameSize);

            const auto& fragmentShaderFilenameSize = std::strlen(pipelineInfo.fragmentShaderFilename) + 1;
            pipelineInfo_->fragmentShaderFilename = new char[fragmentShaderFilenameSize];
            std::memcpy(const_cast<char*>(pipelineInfo_->fragmentShaderFilename),
                pipelineInfo.fragmentShaderFilename,
                fragmentShaderFilenameSize);

            pipelineInfo_->bindingDescriptionCount = pipelineInfo.bindingDescriptionCount;
            if (pipelineInfo.bindingDescriptionCount > 0) {
                pipelineInfo_->bindingDescriptions = new BindingDescription[pipelineInfo.bindingDescriptionCount];
                std::memcpy(const_cast<BindingDescription*>(pipelineInfo_->bindingDescriptions),
                    pipelineInfo.bindingDescriptions,
                    pipelineInfo.bindingDescriptionCount * sizeof(BindingDescription));
            } else {
                pipelineInfo_->bindingDescriptions = nullptr;
            }

            pipelineInfo_->attributeDescriptionCount = pipelineInfo.attributeDescriptionCount;
            if (pipelineInfo.attributeDescriptionCount > 0) {
                pipelineInfo_->attributeDescriptions = new AttributeDescription[pipelineInfo.attributeDescriptionCount];
                std::memcpy(const_cast<AttributeDescription*>(pipelineInfo_->attributeDescriptions),
                    pipelineInfo.attributeDescriptions,
                    pipelineInfo.attributeDescriptionCount * sizeof(AttributeDescription));
            } else {
                pipelineInfo_->attributeDescriptions = nullptr;
            }

            pipelineInfo_->setLayoutCount = pipelineInfo.setLayoutCount;
            if (pipelineInfo.setLayoutCount > 0) {
                pipelineInfo_->setLayouts = new DescriptorSetLayoutInfo[pipelineInfo.setLayoutCount];
                std::memcpy(const_cast<DescriptorSetLayoutInfo*>(pipelineInfo_->setLayouts),
                    pipelineInfo.setLayouts,
                    pipelineInfo.setLayoutCount * sizeof(DescriptorSetLayoutInfo));
            } else {
                pipelineInfo_->setLayouts = nullptr;
            }
        }

        ~GraphicsPipeline() {
            glDeleteProgram(handle_);
            glDeleteVertexArrays(1, &vertexArray_);
            delete[] pipelineInfo_->vertexShaderFilename;
            delete[] pipelineInfo_->fragmentShaderFilename;
            delete[] pipelineInfo_->bindingDescriptions;
            delete[] pipelineInfo_->attributeDescriptions;
            delete[] pipelineInfo_->setLayouts;
            delete pipelineInfo_;
        }

        GraphicsPipeline(const GraphicsPipeline& other) = delete;
        GraphicsPipeline& operator=(const GraphicsPipeline& other) = delete;
        GraphicsPipeline(GraphicsPipeline&& other) = delete;
        GraphicsPipeline& operator=(GraphicsPipeline&& other) = delete;

        [[nodiscard]] bool create() {
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
                const auto infoLog = new char[static_cast<std::size_t>(infoLogLength) + 1];
                glGetProgramInfoLog(handle_, infoLogLength, &infoLogLength, infoLog);
                log::error(std::format("Failed to link program {}: {}", handle_, infoLog));
                delete[] infoLog;
            }

            glDetachShader(handle_, vertexShader.handle());
            glDetachShader(handle_, fragmentShader.handle());

            for (GLuint i = 0; i < pipelineInfo_->attributeDescriptionCount; ++i) {
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

        [[nodiscard]] const PipelineInfo* pipelineInfo() const { return pipelineInfo_; }
        [[nodiscard]] GLuint handle() const { return handle_; }
        [[nodiscard]] GLuint vertexArray() const { return vertexArray_; }
    };
}
