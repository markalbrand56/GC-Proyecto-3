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
    return Intersect{true, tmin, point, normal};
    }