module;

#include <glad/glad.h>

export module sandbox.gl.buffer;

namespace sandbox::gl {
    export class Buffer {
        GLuint handle_ = 0;
    public:
        Buffer() {
            glCreateBuffers(1, &handle_);
        }

        ~Buffer() {
            glDeleteBuffers(1, &handle_);
        }

        Buffer(const Buffer& other) = delete;
        Buffer& operator=(const Buffer& other) = delete;
        Buffer(Buffer&& other) = delete;
        Buffer& operator=(Buffer&& other) = delete;

        void storage(const GLsizeiptr size, const void* data, const GLbitfield flags) const {
            glNamedBufferStorage(handle_, size, data, flags);
        }

        [[nodiscard]] void* map(const GLenum access) const {
            return glMapNamedBuffer(handle_, access);
        }

        void unmap() const {
            glUnmapNamedBuffer(handle_);
        }

        [[nodiscard]] GLuint handle() const { return handle_; }
    };
}
