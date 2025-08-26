module;

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

export module sandbox.camera;

namespace sandbox {
    constexpr glm::dvec3 worldUp{0.0f, 1.0f, 0.0f};

    export class Camera {
    public:
        glm::dvec3 position{};
        /// The rotation is stored in (pitch, yaw).
        glm::dvec2 rotation{};

        void moveToEntity(const glm::dvec3& position, const glm::dvec2& rotation) {
            // add eye height
            this->position = glm::dvec3(position.x, position.y + 1.5, position.z);
            this->rotation = glm::dvec2(-rotation.x, rotation.y);
        }

        [[nodiscard]] glm::dvec3 forwardVec() const {
            const double pitch = glm::radians(glm::clamp(rotation.x, -89.9, 89.9));
            const double yaw = glm::radians(rotation.y);
            return {
                glm::cos(yaw) * glm::cos(pitch),
                glm::sin(pitch),
                glm::sin(yaw) * glm::cos(pitch)
            };
        }

        [[nodiscard]] glm::mat4 viewMat() const {
            return glm::lookAt(
                position,
                position - forwardVec(),
                worldUp
            );
        }
    };
}
