#pragma once

#include <algorithm>
#include "glm/glm/gtx/rotate_vector.hpp"
#include "glm/glm/gtc/quaternion.hpp"
#include "glm/glm/gtc/random.hpp"
#include <string>

/** Build a unit quaternion representing the rotation
 * from u to v. The input vectors need not be normalised. */
glm::quat QuatFromVectors(glm::vec3 u, glm::vec3 v)
{
    float norm_u_norm_v = sqrt(dot(u, u) * dot(v, v));
    float real_part = norm_u_norm_v + dot(u, v);
    glm::vec3 w;

    if (real_part < 1.e-6f * norm_u_norm_v)
    {
        /* If u and v are exactly opposite, rotate 180 degrees
         * around an arbitrary orthogonal axis. Axis normalisation
         * can happen later, when we normalise the quaternion. */
        real_part = 0.0f;
        w = abs(u.x) > abs(u.z) ? glm::vec3(-u.y, u.x, 0.f)
            : glm::vec3(0.f, -u.z, u.y);
    }
    else
    {
        /* Otherwise, build quaternion the standard way. */
        w = cross(u, v);
    }

    return glm::normalize(glm::quat(real_part, w.x, w.y, w.z));
}

struct Ray
{
    glm::vec3 position;
    glm::vec3 direction;
};

// A simple camera
class Camera
{
private:
    glm::vec3 position = glm::vec3(0.0f);                          // Position of the camera in world space
    glm::vec3 focalPoint = glm::vec3(0.0f);                        // Look at point

    float filmWidth = 1.0f;                                         // Width/height of the film
    float filmHeight = 1.0f;

    glm::vec3 viewDirection = glm::vec3(0.0f, 0.0f, 1.0f);         // The direction the camera is looking in
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);                 // The vector to the right
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);                    // The vector up

    float fieldOfView = 60.0f;                                      // Field of view
    float halfAngle = 30.0f;                                        // Half angle of the view frustum
    float aspectRatio = 1.0f;                                       // Ratio of x to y of the viewport

    glm::quat orientation;                                          // A quaternion representing the camera rotation

    glm::vec2 orbitDelta = glm::vec2(0.0f);
    glm::vec3 positionDelta = glm::vec3(0.0f);

    int64_t lastTime = 0;
public:
    Camera()
    {
    }

    virtual ~Camera()
    {

    }

    const glm::vec3& GetPosition() const
    {
        return position;
    }

    void SetPositionAndFocalPoint(const glm::vec3& pos, const glm::vec3& point)
    {
        // From
        position = pos;

        // Focal
        focalPoint = point;

        // Work out direction
        viewDirection = focalPoint - position;
        viewDirection = glm::normalize(viewDirection);

        // Get camera orientation relative to -z
        orientation = QuatFromVectors(viewDirection, glm::vec3(0.0f, 0.0f, -1.0f));
        orientation = glm::normalize(orientation);

        UpdateRightUp();
    }

    void SetFilmSize(float width, float height)
    {
        filmWidth = width;
        filmHeight = height;
        aspectRatio = width / height;

    }

    bool PreRender()
    {
        // The half-width of the viewport, in world space
        halfAngle = float(tan(glm::radians(fieldOfView) / 2.0));

        auto time = GetTime();

        int64_t delta;
        if (lastTime == 0)
        {
            lastTime = time;
            delta = 0;
        }
        else
        {
            delta = time - lastTime;
            lastTime = time;
        }

        bool changed = false;
        if (orbitDelta != glm::vec2(0.0f))
        {
            UpdateOrbit(delta);
            changed = true;
        }

        if (positionDelta != glm::vec3(0.0f))
        {
            UpdatePosition(delta);
            changed = true;
        }
        UpdateRightUp();
        return changed;
    }

    int64_t GetTime()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }

    // Given a screen coordinate, return a ray leaving the camera and entering the world at that 'pixel'
    Ray GetWorldRay(const glm::vec2& imageSample)
    {
        // Could move some of this maths out of here for speed, but this isn't time critical
        //glm::vec3 dir(viewDirection);
       

        auto lensRand = glm::circularRand(0.14f);

        auto dir = viewDirection;
        float x = ((imageSample.x * 2.0f) / filmWidth) - 1.0f;
        float y = ((imageSample.y * 2.0f) / filmHeight) - 1.0f;

        // Take the view direction and adjust it to point at the given sample, based on the 
        // the frustum 
        dir += (right * (halfAngle * aspectRatio * x));
        dir -= (up * (halfAngle * y));
        //dir = normalize(dir);
        float ft = (glm::length(focalPoint - position) - 1.0f) / glm::length(dir);
        glm::vec3 focasPoint = position + dir * ft;

        glm::vec3 lensPoint = position;
        lensPoint += (right * lensRand.x);
        lensPoint += (up * lensRand.y);
        dir = glm::normalize(focasPoint - lensPoint);

        return Ray{ lensPoint, dir };
    }

    void Dolly(float distance)
    {
        positionDelta += viewDirection * distance;
    }

    // Orbit around the focal point, keeping y 'Up'
    void Orbit(const glm::vec2& angle)
    {
        orbitDelta += angle;
    }

    float SmoothStep(float val)
    {
        return val * val * (3.0f - 2.0f * val);
    }

    void UpdatePosition(int64_t timeDelta)
    {
        const float settlingTimeMs = 50;
        float frac = std::min(timeDelta / settlingTimeMs, 1.0f);
        frac = SmoothStep(frac);
        glm::vec3 distance = frac * positionDelta;
        positionDelta *= (1.0f - frac);

        position += distance;
    }

    void UpdateOrbit(int64_t timeDelta)
    {
        const float settlingTimeMs = 50;
        float frac = std::min(timeDelta / settlingTimeMs, 1.0f);
        frac = SmoothStep(frac);

        // Get a proportion of the remaining turn angle, based on the time delta
        glm::vec2 angle = frac * orbitDelta;

        // Reduce the orbit delta remaining for next time
        orbitDelta *= (1.0f - frac);
        if (glm::all(glm::lessThan(glm::abs(orbitDelta), glm::vec2(.1f))))
        {
            orbitDelta = glm::vec2(0.0f);
        }

        // 2 rotations, about right and world up, for the camera
        glm::quat rotY = glm::angleAxis(glm::radians(angle.y), glm::vec3(right));
        glm::quat rotX = glm::angleAxis(glm::radians(angle.x), glm::vec3(0.0f, 1.0f, 0.0f));

        // Concatentation of the current rotations with the new one
        orientation = orientation * rotY * rotX;
        orientation = glm::normalize(orientation);

        // Recalculate position from the new view direction, relative to the focal point
        float distance = glm::length(focalPoint - position);
        viewDirection = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f) * orientation);
        position = focalPoint - (viewDirection * distance);

        UpdateRightUp();

    }

private:
    void UpdateRightUp()
    {
        // Right and up vectors updated based on the quaternion orientation
        right = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f) * orientation);
        up = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f) * orientation);
    }
};
