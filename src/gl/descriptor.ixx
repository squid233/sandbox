module;

#include <cassert>
#include <glad/glad.h>

export module sandbox.gl.descriptor;

import std;
import sandbox.gl.buffer;

namespace sandbox::gl {
    export enum class DescriptorType {
        UNIFORM_BUFFER = 6,
    };

    export struct DescriptorSetLayoutBinding {
        GLuint binding;
        DescriptorType type;
    };

    export struct DescriptorSetLayoutInfo {
        GLuint bindingCount;
        const DescriptorSetLayoutBinding* bindings;
    };

    export struct DescriptorBufferInfo {
        Buffer* buffer;
        GLintptr offset;
        GLsizeiptr range;
    };

    export struct WriteDescriptorSet {
        GLuint dstBinding;
        DescriptorType descriptorType;

        const DescriptorBufferInfo* bufferInfo;
    };

    export class Descriptor {
        DescriptorType descriptorType_;
    public:
        explicit Descriptor(const DescriptorType descriptorType) : descriptorType_(descriptorType) {}
        virtual ~Descriptor() = default;

        Descriptor(const Descriptor& other) = delete;
        Descriptor& operator=(const Descriptor& other) = delete;
        Descriptor(Descriptor&& other) = delete;
        Descriptor& operator=(Descriptor&& other) = delete;

        [[nodiscard]] DescriptorType type() const { return descriptorType_; }
    };

    export class UniformBufferDescriptor final : public Descriptor {
        DescriptorBufferInfo info_{};
    public:
        explicit UniformBufferDescriptor() : Descriptor(DescriptorType::UNIFORM_BUFFER) {}
        ~UniformBufferDescriptor() override = default;

        UniformBufferDescriptor(const UniformBufferDescriptor& other) = delete;
        UniformBufferDescriptor& operator=(const UniformBufferDescriptor& other) = delete;
        UniformBufferDescriptor(UniformBufferDescriptor&& other) = delete;
        UniformBufferDescriptor& operator=(UniformBufferDescriptor&& other) = delete;

        [[nodiscard]] const DescriptorBufferInfo& info() const { return info_; }
        void update(const DescriptorBufferInfo& info) { info_ = info; }
    };

    export class DescriptorSet {
        std::map<GLuint, Descriptor*> map_{};
    public:
        explicit DescriptorSet(const DescriptorSetLayoutInfo& info) {
            for (GLuint i = 0; i < info.bindingCount; ++i) {
                const auto& [binding, type] = info.bindings[i];
                auto& descriptor = map_[binding];
                switch (type) {
                    case DescriptorType::UNIFORM_BUFFER:
                        descriptor = new UniformBufferDescriptor();
                        break;
                }
            }
        }

        ~DescriptorSet() {
            for (const auto& descriptor : map_ | std::views::values) {
                delete descriptor;
            }
            map_.clear();
        }

        DescriptorSet(const DescriptorSet& other) = delete;
        DescriptorSet& operator=(const DescriptorSet& other) = delete;
        DescriptorSet(DescriptorSet&& other) = delete;
        DescriptorSet& operator=(DescriptorSet&& other) = delete;

        void update(const GLuint descriptorWriteCount, const WriteDescriptorSet* descriptorWrites) {
            for (GLuint i = 0; i < descriptorWriteCount; ++i) {
                const auto& descriptorWrite = descriptorWrites[i];
                const auto& descriptor = map_[descriptorWrite.dstBinding];
                assert(descriptor != nullptr);
                assert(descriptor->type() == descriptorWrite.descriptorType);
                switch (descriptorWrite.descriptorType) {
                    case DescriptorType::UNIFORM_BUFFER:
                        dynamic_cast<UniformBufferDescriptor*>(descriptor)->update(*descriptorWrite.bufferInfo);
                        break;
                }
            }
        }

        [[nodiscard]] const std::map<GLuint, Descriptor*>& descriptors() const { return map_; }
    };
}
