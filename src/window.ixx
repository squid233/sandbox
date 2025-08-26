module;

#include <GLFW/glfw3.h>

export module sandbox.window;

namespace sandbox {
    export class Mouse;
    export using cursor_callback_t = void (*)(const Mouse*);

    export class Mouse {
        double x_ = 0;
        double y_ = 0;
        double dx_ = 0;
        double dy_ = 0;
        cursor_callback_t cursorCallback_ = nullptr;
    public:
        explicit Mouse(GLFWwindow* window) {
            glfwSetWindowUserPointer(window, this);
            glfwGetCursorPos(window, &x_, &y_);
            glfwSetCursorPosCallback(window, [](GLFWwindow* window1, const double x, const double y) noexcept {
                // TODO
                const auto& self = static_cast<Mouse*>(glfwGetWindowUserPointer(window1));
                self->dx_ = x - self->x_;
                self->dy_ = y - self->y_;
                if (self->cursorCallback_ != nullptr) {
                    self->cursorCallback_(self);
                }
                self->x_ = x;
                self->y_ = y;
            });
        }

        void setCursorCallback(const cursor_callback_t callback) {
            cursorCallback_ = callback;
        }

        [[nodiscard]] double x() const { return x_; }
        [[nodiscard]] double y() const { return y_; }
        [[nodiscard]] double deltaX() const { return dx_; }
        [[nodiscard]] double deltaY() const { return dy_; }
    };
}
