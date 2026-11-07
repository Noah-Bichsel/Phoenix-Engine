#include <glm/glm.hpp>
#include <core/math/Vector.h>

namespace Phoenix
{
    inline static glm::vec3 ToGlm(const Vector3& v) { return glm::vec3(v.x, v.y, v.z); }
    inline static Vector3 FromGlm(const glm::vec3& v) { return Vector3(v.x, v.y, v.z); }
}