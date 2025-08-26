module;

#include <GLFW/glfw3.h>

export module sandbox.window;

namespace sandbox {
    export class Mouse;
    export class IMouseAcquirable {
    public:
        virtual ~IMouseAcquirable() = default;
        virtual Mouse* mouse() = 0;
    };

    export using cursor_callback_t = void (*)(const Mouse*);

    export class Mouse {
        double x_ = 0;
        double y_ = 0;
        double dx_ = 0;
        double dy_ = 0;
        GLFWwindow* window_;
        cursor_callback_t cursorCallback_ = nullptr;
    public:
        explicit Mouse(GLFWwindow* window) : window_(window) {
            glfwGetCursorPos(window, &x_, &y_);
            glfwSetCursorPosCallback(window, [](GLFWwindow* window1, const double x, const double y) noexcept {
                const auto& userPtr = static_cast<IMouseAcquirable*>(glfwGetWindowUserPointer(window1));
                const auto& self = userPtr->mouse();
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

        [[nodiscard]] int getButton(const int button) const {
            return glfwGetMouseButton(window_, button);
        }

        [[nodiscard]] double x() const { return x_; }
        [[nodiscard]] double y() const { return y_; }
        [[nodiscard]] double deltaX() const { return dx_; }
        [[nodiscard]] double deltaY() const { return dy_; }
        [[nodiscard]] GLFWwindow* window() const { return window_; }
    };
}
