#include <thread>
#include <glm/glm.hpp>

#include "sceneobjects.h"
#include "camera.h"
#include "camera_manipulator.h"

#include "device.h"
#include "bitmap_utils.h"

#define MAX_DEPTH 5

std::vector<std::shared_ptr<SceneObject>> sceneObjects;
std::vector<std::shared_ptr<SceneObject>> lightObjects;
std::shared_ptr<Camera> pCamera;
std::shared_ptr<Manipulator> pManipulator;
BufferData* screenBufferData;

float cameraDistance = 8.0f;
bool pause = false;
bool step = true;
int currentSample = 0;
int partitions = std::thread::hardware_concurrency();

glm::vec3 backgroundColor = glm::vec3{ 135.0f / 255.0f, 206.0f / 255.0f, 235.0f / 255.0f };
glm::vec3 backgroundColor2 = glm::vec3{ 135.0f / 255.0f, 206.0f / 255.0f, 235.0f / 255.0f } * .75f;
float bias = 0.001f;

void render_init()
{
    deviceParams.pName = "Whitted Ray Tracer";

    sceneObjects.clear();
    lightObjects.clear();

    // Red ball
    Material mat;
    mat.albedo = glm::vec3(.7f, .1f, .1f);
    mat.specular = glm::vec3(.9f, .5f, .5f);
    mat.refractive_index = .95f;
    mat.opacity = .06f;
    sceneObjects.push_back(std::make_shared<Sphere>(mat, glm::vec3(0.0f, 2.0f, 0.f), 2.0f));

    // Purple ball
    mat.albedo = glm::vec3(0.7f, 0.0f, 0.7f);
    mat.specular = glm::vec3(0.9f, 0.9f, 0.8f);
    mat.refractive_index = .991f;
    mat.opacity = 0.6f;
    sceneObjects.push_back(std::make_shared<Sphere>(mat, glm::vec3(-2.5f, 1.0f, 2.f), 1.0f));
    mat.refractive_index = 1.0f;
    mat.opacity = 1.0f;

    // Blue ball
    mat.albedo = glm::vec3(0.0f, 0.3f, 1.0f);
    mat.specular = glm::vec3(0.0f, 0.2f, 1.0f);
    mat.emissive = glm::vec3(0.0f, 0.0f, 0.0f);
    mat.specular_exponent = 35;
    sceneObjects.push_back(std::make_shared<Sphere>(mat, glm::vec3(-1.0f, 0.7f, 3.f), 0.7f));
   

    // White ball
    mat.albedo = glm::vec3(1.0f, 1.0f, 1.0f);
    mat.specular = glm::vec3(0.0f, 0.0f, 0.0f);
    mat.emissive = glm::vec3(0.3f, 0.5f, 0.5f);
    sceneObjects.push_back(std::make_shared<Sphere>(mat, glm::vec3(2.8f, 0.8f, 2.0f), 0.8f));

    // White light
    mat.albedo = glm::vec3(0.0f, 0.8f, 0.0f);
    mat.specular = glm::vec3(0.0f, 0.0f, 0.0f);
    mat.emissive = glm::vec3(.7f, 0.7f, 0.7f);
    sceneObjects.push_back(std::make_shared<Sphere>(mat, glm::vec3(-0.8f, 10.4f, 8.0f), 1.0f));

    sceneObjects.push_back(std::make_shared<TiledPlane>(glm::vec3(0.0f, 0.0f, 0.0f), normalize(glm::vec3(0.0f, 1.0f, 0.0f))));

    pCamera = std::make_shared<Camera>();
    pCamera->SetPositionAndFocalPoint(glm::vec3(0.0f, 5.0f, cameraDistance), glm::vec3(0.0f, 1.0f, 0.0f));

    pManipulator = std::make_shared<Manipulator>(pCamera);
}

void render_destroy()
{
    device_buffer_destroy(screenBufferData);
    screenBufferData = nullptr;
}

SceneObject* FindNearestObject(glm::vec3 rayorig, glm::vec3 raydir, float& nearestDistance)
{
    SceneObject* nearestObject = nullptr;
    nearestDistance = std::numeric_limits<float>::max();

    // find intersection of this ray with the sphere in the scene
    for (auto pObject : sceneObjects)
    {
        float distance;
        if (pObject->Intersects(rayorig, raydir, distance) &&
            nearestDistance > distance)
        {
            nearestObject = pObject.get();
            nearestDistance = distance;
        }
    }
    return nearestObject;
}

glm::vec3 refract(const glm::vec3& I, const glm::vec3& N, const float &ior)
{
    float cosi = glm::clamp(glm::dot(I, N), -1.0f, 1.0f);
    float etai = 1, etat = ior;
    glm::vec3 n = N;
    if (cosi < 0)
    {
        cosi = -cosi;
    }
    else
    {
        std::swap(etai, etat);
        n = -N;
    }
    float eta = etai / etat;
    float k = 1 - eta * eta * (1 - cosi * cosi);
    return k < 0 ? glm::vec3(0.0f) : eta * I + (eta * cosi - sqrtf(k)) * n;
}

void fresnel(const glm::vec3& I, const glm::vec3& N, const float &ior, float &kr)
{
    float cosi = glm::clamp(glm::dot(I, N), -1.0f, 1.0f);
    float etai = 1;
    float etat = ior;
    if (cosi > 0)
    {
        std::swap(etai, etat);
    }

    // Compute sini using Snell's law
    float sint = etai / etat * sqrtf(std::max(0.f, 1 - cosi * cosi));

    // Total internal reflection
    if (sint >= 1)
    {
        kr = 1;
    }
    else
    {
        float cost = sqrtf(std::max(0.f, 1 - sint * sint));
        cosi = fabsf(cosi);
        float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
        float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
        kr = (Rs * Rs + Rp * Rp) / 2;
    }
    // As a consequence of the conservation of energy, transmittance is given by:
    // kt = 1 - kr;
}

glm::vec3 TraceRay(const glm::vec3& ray_origin, const glm::vec3 &ray_dir, const int depth)
{
    const SceneObject* nearestObject = nullptr;
    float distance;

    glm::vec3 back = glm::mix(backgroundColor, backgroundColor2, 1.0f - glm::clamp(glm::dot(ray_dir, glm::vec3(0.0f, 1.0f, 0.0f)), 0.0f, 1.0f));

    // Too deep
    if (depth > MAX_DEPTH)
    {
        return back;
    }

    nearestObject = FindNearestObject(ray_origin, ray_dir, distance);
    if (!nearestObject)
    {
        // Didn't hit an object, so return background color
        return back;
    }

    glm::vec3 outputColor = glm::vec3(0.0f, 0.0f, 0.0f);

    // Where we hit on the surface
    glm::vec3 hit_point = ray_origin + (ray_dir * distance);

    glm::vec3 normal = nearestObject->GetSurfaceNormal(hit_point);
    const Material& material = nearestObject->GetMaterial(hit_point);

    if (material.refractive_index != 1.0f && material.opacity < 1.0f)
    {
        float kr;
        fresnel(ray_dir, normal, material.refractive_index, kr);

        glm::vec3 reflect_direction = normalize(glm::reflect(ray_dir, normal));
        auto reflect_start = (glm::dot(reflect_direction, normal) > 0) ? (hit_point + normal * bias) : (hit_point - normal * bias);

        glm::vec3 refract_direction = normalize(refract(ray_dir, normal, material.refractive_index));
        auto refract_start = (glm::dot(refract_direction, normal) > 0) ? (hit_point + normal * bias) : (hit_point - normal * bias);

        auto reflection_color = TraceRay(reflect_start, reflect_direction, depth + 1);
        auto refraction_color = TraceRay(refract_start, refract_direction, depth + 1);

        outputColor = (reflection_color * kr + refraction_color * (1.0f - kr)) * (1.0f - material.opacity);
    }

    if (material.opacity > 0.0f)
    {
        glm::vec3 light_accumulation = glm::vec3(0.0f);
        glm::vec3 specular_accumulation = glm::vec3(0.0f);

        // For every emitter, gather the light
        for (auto& emitterObj : sceneObjects)
        {
            // Find the part of the object we hit
            glm::vec3 light_dir = emitterObj->GetRayFrom(hit_point);
            float light_distance;

            // Move hit point out slightly
            auto light_origin = (glm::dot(ray_dir, normal) < 0) ? (hit_point + normal * bias) : (hit_point - normal * bias);

            emitterObj->Intersects(light_origin + (light_dir * bias), light_dir, light_distance);
            auto lightMaterial = emitterObj->GetMaterial(hit_point + light_dir * light_distance);

            // It's not a light!
            if (lightMaterial.emissive == glm::vec3(0.0f))
                continue;

            float shadowFactor = 1.0f;
            float nearestDistance;
            auto nearestOccluder = FindNearestObject(light_origin, light_dir, nearestDistance);
            if (nearestOccluder != nullptr)
            {
                if (nearestDistance < (light_distance))// - bias))
                {
                    // Quick fix for shadows through non-opaque objects!
                    auto occluderMat = nearestOccluder->GetMaterial(hit_point + light_dir * nearestDistance);
                    shadowFactor -= occluderMat.opacity;
                    shadowFactor = std::max(0.0f, shadowFactor);
                }
            }

            float l_dot_n = std::max(0.0f, dot(normal, light_dir));
            light_accumulation += (shadowFactor) * lightMaterial.emissive * l_dot_n;

            auto light_reflect_dir = glm::reflect(-light_dir, normal);
            specular_accumulation += powf(std::max(0.0f, -glm::dot(light_reflect_dir, ray_dir)), material.specular_exponent) * lightMaterial.emissive;
        }
        outputColor += (light_accumulation * material.albedo * material.opacity) + material.emissive + (material.specular * specular_accumulation * material.opacity);
    }
    return outputColor;
}

void render_update()
{
    bool changed = pCamera->PreRender();
    if (changed)
    {
        currentSample = 0;
    }
}

void render_redraw()
{
    if (!screenBufferData)
    {
        screenBufferData = device_buffer_create();
    }
    device_buffer_ensure_screen_size(screenBufferData);

    auto t = time(NULL);
    std::srand(currentSample == 0 ? 0 : (unsigned int)t);

    std::vector<std::shared_ptr<std::thread>> threads;

    const float k1 = float(currentSample);
    const float k2 = 1.f / (k1 + 1.f);
    glm::vec2 sample = glm::linearRand(glm::vec2(0.0f), glm::vec2(1.0f));
    for (int i = 0; i < partitions; i++)
    {
        auto pT = std::make_shared<std::thread>([&](int offset)
        {
            for (int y = offset; y < screenBufferData->BufferHeight; y += partitions)
            {
                for (int x = 0; x < screenBufferData->BufferWidth; x += 1)
                {
                    glm::vec3 color{ 0.0f, 0.0f, 0.0f };
                    auto offset = sample + glm::vec2(x, y);

                    auto ray = pCamera->GetWorldRay(offset);
                    color += TraceRay(ray.position, ray.direction, 0);

                    auto index = (y * screenBufferData->BufferWidth) + x;
                    auto& bufferVal = screenBufferData->buffer[index];

                    bufferVal = ((bufferVal * k1) + glm::vec4(color, 1.0f)) * k2;
                }
            }
        }, i);
        threads.push_back(pT);
    }

    for (auto& t : threads)
    {
        t->join();
    }
    currentSample++;

    device_buffer_set_to_display(screenBufferData);
}

void render_resized(int x, int y)
{
    pCamera->SetFilmSize(float(x), float(y));
    currentSample = 0;
}

void render_key_pressed(char key)
{
    if (key == 'o')
    {
        currentSample = 0;
    }
    else if (key == 'p')
    {
        pause = !pause;
    }
    else if (key == ' ')
    {
        step = true;
    }
    else if (key == 'b')
    {
        auto pBitmap = bitmap_create_from_buffer(screenBufferData->buffer, screenBufferData->BufferWidth, screenBufferData->BufferHeight);
        bitmap_write(pBitmap, "rayout.bmp");
    }
    else if (key == '+')
    {
        deviceParams.zoomFactor += .1f;
    }
    else if (key == '-')
    {
        deviceParams.zoomFactor -= .1f;
    }
    else if (key == 'd')
    {
        deviceParams.offset.x += .1f;
    }
    else if (key == 'a')
    {
        deviceParams.offset.x -= .1f;
    }
}

void render_key_down(char key)
{
    if (key == 'W')
    {
        pCamera->Dolly(.5f);
        currentSample = 0;
        step = true;
    }
    else if (key == 'S')
    {
        pCamera->Dolly(-.5f);
        currentSample = 0;
        step = true;
    }
}

void render_mouse_move(const glm::vec2& pos)
{
    if (pManipulator->MouseMove(pos))
    {
        currentSample = 0;
        step = true;
    }
}

void render_mouse_down(const glm::vec2& pos)
{
    pManipulator->MouseDown(pos);
    currentSample = 0;
    step = true;
}

void render_mouse_up(const glm::vec2& pos)
{
    pManipulator->MouseUp(pos);
    currentSample = 0;
    step = true;
}

