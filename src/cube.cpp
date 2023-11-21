#include "cube.h"

Cube::Cube(const glm::vec3& minCorner, const glm::vec3& maxCorner, const Material& mat)
  : minCorner(minCorner), maxCorner(maxCorner), Object(mat) {}

Intersect Cube::rayIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) const {
    float tmin = (minCorner.x - rayOrigin.x) / rayDirection.x;
    float tmax = (maxCorner.x - rayOrigin.x) / rayDirection.x;
    
    if (tmin > tmax) {
        std::swap(tmin, tmax);
    }
    
    float tymin = (minCorner.y - rayOrigin.y) / rayDirection.y;
    float tymax = (maxCorner.y - rayOrigin.y) / rayDirection.y;
    
    if (tymin > tymax) {
        std::swap(tymin, tymax);
    }
    
    if ((tmin > tymax) || (tymin > tmax)) {
        return Intersect{false};
    }
    
    if (tymin > tmin) {
        tmin = tymin;
    }
    
    if (tymax < tmax) {
        tmax = tymax;
    }
    
    float tzmin = (minCorner.z - rayOrigin.z) / rayDirection.z;
    float tzmax = (maxCorner.z - rayOrigin.z) / rayDirection.z;
    
    if (tzmin > tzmax) {
        std::swap(tzmin, tzmax);
    }
    
    if ((tmin > tzmax) || (tzmin > tmax)) {
        return Intersect{false};
    }
    
    if (tzmin > tmin) {
        tmin = tzmin;
    }
    
    if (tzmax < tmax) {
        tmax = tzmax;
    }
    
    if (tmin < 0 && tmax < 0) {
        return Intersect{false};
    }
    
    glm::vec3 point = rayOrigin + tmin * rayDirection;
    glm::vec3 normal = glm::normalize(point - minCorner);

    glm::vec3 center = (minCorner + maxCorner) / 2.0f;
    float edgeLength = maxCorner.x - minCorner.x;

    glm::vec3 delta = rayOrigin + tmin * rayDirection - center;
    glm::vec3 absDelta = glm::abs(delta);

    // Determine which face was hit
    if (absDelta.x > absDelta.y && absDelta.x > absDelta.z) {
        normal = glm::vec3(delta.x > 0 ? 1 : -1, 0, 0);
    } else if (absDelta.y > absDelta.z) {
        normal = glm::vec3(0, delta.y > 0 ? 1 : -1, 0);
    } else {
        normal = glm::vec3(0, 0, delta.z > 0 ? 1 : -1);
    }

    // Calculate UV coordinates for the hit face
    float tx, ty;
    if (absDelta.x > absDelta.y && absDelta.x > absDelta.z) {
        normal = glm::vec3(delta.x > 0 ? 1 : -1, 0, 0);
        tx = (delta.y + edgeLength / 2) / edgeLength;
        ty = (delta.z + edgeLength / 2) / edgeLength;
    } else if (absDelta.y > absDelta.z) {
        normal = glm::vec3(0, delta.y > 0 ? 1 : -1, 0);
        tx = (delta.x + edgeLength / 2) / edgeLength;
        ty = (delta.z + edgeLength / 2) / edgeLength;
    } else {
        normal = glm::vec3(0, 0, delta.z > 0 ? 1 : -1);
        tx = (delta.x + edgeLength / 2) / edgeLength;
        ty = (delta.y + edgeLength / 2) / edgeLength;
    }

    tx = glm::clamp(tx, 0.0f, 1.0f);
    ty = glm::clamp(ty, 0.0f, 1.0f);

    return Intersect{true, tmin, point, normal, tx, ty};
}