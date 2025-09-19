module;

#include <cassert>
#include <glad/glad.h>

export module sandbox.gl;

import std;
import sandbox.gl.buffer;
import sandbox.gl.descriptor;
import sandbox.gl.pipeline;
import sandbox.io;
import sandbox.log;

namespace sandbox::gl {
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

    export enum class Format {
        B8G8R8A8_UNORM = 44,
        D16_UNORM = 124,
        D32_SFLOAT = 126,
        S8_UINT = 127,
        D16_UNORM_S8_UINT = 128,
        D24_UNORM_S8_UINT = 129,
        D32_SFLOAT_S8_UINT = 130,
    };

    export enum class AttachmentLoadOp {
        CLEAR = 1,
        DONT_CARE = 2,
    };

    export struct RenderingAttachmentInfo {
        Format format;
        AttachmentLoadOp loadOp;
        ClearValue clearValue;
    };

    export struct RenderingInfo {
        GLuint colorAttachmentCount;
        const RenderingAttachmentInfo* colorAttachments;
        const RenderingAttachmentInfo* depthAttachment;
        const RenderingAttachmentInfo* stencilAttachment;
    };

    export class CommandBuffer {
        GraphicsPipeline* graphicsPipeline_ = nullptr;
        bool isInRenderPass_ = false;
    public:
        void beginRenderPass(const RenderingInfo& renderingInfo) {
            assert(!isInRenderPass_);
            isInRenderPass_ = true;
            for (GLuint i = 0; i < renderingInfo.colorAttachmentCount; ++i) {
                const auto& colorAttachment = renderingInfo.colorAttachments[i];
                if (colorAttachment.loadOp == AttachmentLoadOp::CLEAR) {
                    if (colorAttachment.format == Format::B8G8R8A8_UNORM) {
                        glClearNamedFramebufferfv(0, GL_COLOR, static_cast<GLint>(i), colorAttachment.clearValue.color.float32);
                    } else {
                        assert(false);
                    }
                }
            }
            if (renderingInfo.depthAttachment != nullptr && renderingInfo.depthAttachment->loadOp == AttachmentLoadOp::CLEAR) {
                switch (renderingInfo.depthAttachment->format) {
                    case Format::D16_UNORM:
                    case Format::D16_UNORM_S8_UINT:
                    case Format::D24_UNORM_S8_UINT:
                    case Format::D32_SFLOAT:
                    case Format::D32_SFLOAT_S8_UINT:
                        glClearNamedFramebufferfv(0, GL_DEPTH, 0, &renderingInfo.depthAttachment->clearValue.depthStencil.depth);
                        break;
                    case Format::B8G8R8A8_UNORM:
                    case Format::S8_UINT:
                    default:
                        assert(false);
                }
            }
            if (renderingInfo.stencilAttachment != nullptr && renderingInfo.stencilAttachment->loadOp == AttachmentLoadOp::CLEAR) {
                switch (renderingInfo.stencilAttachment->format) {
                    case Format::S8_UINT:
                    case Format::D16_UNORM_S8_UINT:
                    case Format::D24_UNORM_S8_UINT:
                    case Format::D32_SFLOAT_S8_UINT:
                        glClearNamedFramebufferuiv(0, GL_STENCIL, 0, &renderingInfo.stencilAttachment->clearValue.depthStencil.stencil);
                        break;
                    case Format::B8G8R8A8_UNORM:
                    case Format::D16_UNORM:
                    case Format::D32_SFLOAT:
                    default:
                        assert(false);
                }
            }
        }

        void bindGraphicsPipeline(GraphicsPipeline* graphicsPipeline) {
            assert(isInRenderPass_);
            graphicsPipeline_ = graphicsPipeline;
        }

        void bindDescriptorSet(const DescriptorSet& set) const {
            assert(isInRenderPass_);
            for (const auto& [binding, descriptor] : set.descriptors()) {
                switch (descriptor->type()) {
                    case DescriptorType::UNIFORM_BUFFER:
                        const auto& info = dynamic_cast<UniformBufferDescriptor*>(descriptor)->info();
                        assert(info.buffer != nullptr);
                        glBindBufferRange(GL_UNIFORM_BUFFER, binding, info.buffer->handle(), info.offset, info.range);
                        break;
                }
            }
        }

        void bindVertexBuffers(const GLuint firstBinding, const GLsizei bindingCount, const GLuint* pBuffers, const GLintptr* pOffsets) const {
            assert(isInRenderPass_);
            assert(graphicsPipeline_ != nullptr);
            const auto& info = graphicsPipeline_->pipelineInfo();
            const auto& strides = new GLsizei[static_cast<size_t>(bindingCount)]{};
            for (GLsizei i = 0; i < bindingCount; ++i) {
                for (GLuint j = 0; j < info->bindingDescriptionCount; ++j) {
                    if (const auto& [binding, stride] = info->bindingDescriptions[j]; firstBinding + i == binding) {
                        strides[i] = stride;
                    }
                }
            }
            glVertexArrayVertexBuffers(graphicsPipeline_->vertexArray(), firstBinding, bindingCount, pBuffers, pOffsets, strides);
            delete[] strides;
        }

        void bindVertexBuffer(const GLuint binding, const Buffer& buffer, const GLintptr offset) const {
            assert(isInRenderPass_);
            assert(graphicsPipeline_ != nullptr);
            const auto& info = graphicsPipeline_->pipelineInfo();
            GLsizei stride = 0;
            for (GLuint i = 0; i < info->bindingDescriptionCount; ++i) {
                if (const auto& bindingDescription = info->bindingDescriptions[i]; binding == bindingDescription.binding) {
                    stride = bindingDescription.stride;
                }
            }
            glVertexArrayVertexBuffer(graphicsPipeline_->vertexArray(), binding, buffer.handle(), offset, stride);
        }

        void bindIndexBuffer(const Buffer& buffer) const {
            assert(isInRenderPass_);
            assert(graphicsPipeline_ != nullptr);
            glVertexArrayElementBuffer(graphicsPipeline_->vertexArray(), buffer.handle());
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
            graphicsPipeline_ = nullptr;
        }
    };
}
