#pragma once
#include "raylib.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>

// ============================================================================
// PHANTOM FORCES STYLE 3D DAMPED HARMONIC OSCILLATOR (SPRING SOLVER)
// Used for procedural recoil, walk bob, mouse sway, ADS, and jump inertia
// ============================================================================
struct Spring3D {
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    Vector3 velocity = { 0.0f, 0.0f, 0.0f };
    Vector3 target   = { 0.0f, 0.0f, 0.0f };
    float stiffness  = 180.0f; // k
    float damping    = 18.0f;  // c

    Spring3D() = default;
    Spring3D(float k, float c) : stiffness(k), damping(c) {}

    void Shove(Vector3 impulse) {
        velocity = Vector3Add(velocity, impulse);
    }

    void Update(float dt) {
        // Sub-step for unconditional stability even on low FPS spikes
        const int steps = 4;
        float subDt = std::min(dt, 0.05f) / (float)steps;
        for (int i = 0; i < steps; ++i) {
            Vector3 displacement = Vector3Subtract(position, target);
            Vector3 springForce = Vector3Scale(displacement, -stiffness);
            Vector3 dampForce = Vector3Scale(velocity, -damping);
            Vector3 accel = Vector3Add(springForce, dampForce);
            velocity = Vector3Add(velocity, Vector3Scale(accel, subDt));
            position = Vector3Add(position, Vector3Scale(velocity, subDt));
        }
    }
};

inline float LerpFloat(float a, float b, float t) {
    return a + (b - a) * std::clamp(t, 0.0f, 1.0f);
}

inline float SmoothDampFloat(float a, float b, float speed, float dt) {
    return LerpFloat(a, b, 1.0f - std::exp(-speed * dt));
}

inline Vector3 SmoothDampVec3(Vector3 a, Vector3 b, float speed, float dt) {
    float f = 1.0f - std::exp(-speed * dt);
    return Vector3Lerp(a, b, std::clamp(f, 0.0f, 1.0f));
}

// Compute Ray vs AABB intersection with surface normal and entry/exit distances
struct RayBoxHit {
    bool hit = false;
    float tNear = 0.0f;
    float tFar = 0.0f;
    Vector3 point = { 0, 0, 0 };
    Vector3 normal = { 0, 1, 0 };
};

inline RayBoxHit IntersectRayAABB(Vector3 orig, Vector3 dir, BoundingBox box) {
    RayBoxHit res;
    float tmin = -1e30f;
    float tmax =  1e30f;
    Vector3 hitNormal = { 0, 0, 0 };

    const float eps = 1e-6f;
    float origArr[3] = { orig.x, orig.y, orig.z };
    float dirArr[3]  = { dir.x,  dir.y,  dir.z };
    float minArr[3]  = { box.min.x, box.min.y, box.min.z };
    float maxArr[3]  = { box.max.x, box.max.y, box.max.z };

    for (int a = 0; a < 3; ++a) {
        if (std::fabs(dirArr[a]) < eps) {
            if (origArr[a] < minArr[a] || origArr[a] > maxArr[a]) {
                return res;
            }
        } else {
            float invD = 1.0f / dirArr[a];
            float t1 = (minArr[a] - origArr[a]) * invD;
            float t2 = (maxArr[a] - origArr[a]) * invD;
            float sign = -1.0f;
            if (t1 > t2) {
                std::swap(t1, t2);
                sign = 1.0f;
            }
            if (t1 > tmin) {
                tmin = t1;
                hitNormal = { 0, 0, 0 };
                if (a == 0) hitNormal.x = sign;
                if (a == 1) hitNormal.y = sign;
                if (a == 2) hitNormal.z = sign;
            }
            if (t2 < tmax) {
                tmax = t2;
            }
            if (tmin > tmax) return res;
        }
    }

    if (tmax < 0.0f) return res;

    res.hit = true;
    res.tNear = tmin;
    res.tFar = tmax;
    float impactT = (tmin >= 0.0f) ? tmin : 0.0f;
    res.point = Vector3Add(orig, Vector3Scale(dir, impactT));
    res.normal = hitNormal;
    return res;
}

