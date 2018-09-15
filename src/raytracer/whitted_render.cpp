#include <thread>
#include <glm/glm.hpp>

#include "sceneobjects.h"
#include "camera.h"
#include "camera_manipulator.h"

#include "device.h"
#include "bitmap_utils.h"

#define MAX_DEPTH 3

std::vector<std::shared_ptr<SceneObject>> sceneObjects;
std::shared_ptr<Camera> pCamera;
std::shared_ptr<Manipulator> pManipulator;
BufferData* screenBufferData;

float cameraDistance = 8.0f;
bool pause = false;
bool step = true;
int currentSample = 0;
int partitions = std::thread::hardware_concurrency();

void render_init()
{
    deviceParams.pName = "Whitted Ray Tracer";

    sceneObjects.clear();

    // Red ball
    Material mat;
    mat.albedo = glm::vec3(.7f, .1f, .1f);
    mat.specular = glm::vec3(.9f, .1f, .1f);
    mat.reflectance = 0.5f;
    sceneObjects.push_back(std::make_shared<Sphere>(mat, glm::vec3(0.0f, 2.0f, 0.f), 2.0f));

    // Purple ball
    mat.albedo = glm::vec3(0.7f, 0.0f, 0.7f);
    mat.specular = glm::vec3(0.9f, 0.9f, 0.8f);
    mat.reflectance = 0.5f;
    sceneObjects.push_back(std::make_shared<Sphere>(mat, glm::vec3(-2.5f, 1.0f, 2.f), 1.0f));

    // Blue ball
    mat.albedo = glm::vec3(0.0f, 0.3f, 1.0f);
    mat.specular = glm::vec3(0.0f, 0.0f, 1.0f);
    mat.reflectance = 0.0f;
    mat.emissive = glm::vec3(0.0f, 0.0f, 0.0f);
    sceneObjects.push_back(std::make_shared<Sphere>(mat, glm::vec3(0.0f, 0.5f, 3.f), 0.5f));

    // White ball
    mat.albedo = glm::vec3(1.0f, 1.0f, 1.0f);
    mat.specular = glm::vec3(0.0f, 0.0f, 0.0f);
    mat.reflectance = .0f;
    mat.emissive = glm::vec3(0.0f, 0.8f, 0.8f);
    sceneObjects.push_back(std::make_shared<Sphere>(mat, glm::vec3(2.8f, 0.8f, 2.0f), 0.8f));

    // White light
    mat.albedo = glm::vec3(0.0f, 0.8f, 0.0f);
    mat.specular = glm::vec3(0.0f, 0.0f, 0.0f);
    mat.reflectance = 0.0f;
    mat.emissive = glm::vec3(1.0f, 1.0f, 1.0f);
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

glm::vec3 TraceRay(const glm::vec3& rayorig, const glm::vec3 &raydir, const int depth)
{
    const SceneObject* nearestObject = nullptr;
    float distance;
    nearestObject = FindNearestObject(rayorig, raydir, distance);

    if (!nearestObject)
    {
        return glm::vec3{ 0.2f, 0.2f, 0.2f };
    }
    glm::vec3 pos = rayorig + (raydir * distance);
    glm::vec3 normal = nearestObject->GetSurfaceNormal(pos);
    glm::vec3 outputColor{ 0.0f, 0.0f, 0.0f };

    const Material& material = nearestObject->GetMaterial(pos);

    glm::vec3 reflect = glm::normalize(glm::reflect(raydir, normal));

    // If the object is transparent, get the reflection color
    if (depth < MAX_DEPTH && (material.reflectance > 0.0f))
    {
        glm::vec3 reflectColor(0.0f, 0.0f, 0.0f);
        glm::vec3 refractColor(0.0f, 0.0f, 0.0f);

        reflectColor = TraceRay(pos + (reflect * 0.001f), reflect, depth + 1);
        outputColor = (reflectColor * material.reflectance);
    }
    // For every emitter, gather the light
    for (auto& emitterObj : sceneObjects)
    {
        glm::vec3 emitterDir = emitterObj->GetRayFrom(pos);

        float bestDistance = std::numeric_limits<float>::max();
        SceneObject* pOccluder = nullptr;
        const Material* pEmissiveMat = nullptr;
        for (auto& occluder : sceneObjects)
        {
            if (occluder->Intersects(pos + (emitterDir * 0.001f), emitterDir, distance))
            {
                if (occluder == emitterObj)
                {
                    if (bestDistance > distance)
                    {
                        bestDistance = distance;

                        // If we found our emitter, and the point we hit is not emissive, then ignore
                        pEmissiveMat = &occluder->GetMaterial(pos + (emitterDir * distance));
                        if (pEmissiveMat->emissive == glm::vec3(0.0f, 0.0f, 0.0f))
                        {
                            pEmissiveMat = nullptr;
                        }
                        else
                        {
                            pOccluder = nullptr;
                        }
                    }
                }
                else
                {
                    if (bestDistance > distance)
                    {
                        pOccluder = occluder.get();
                        pEmissiveMat = nullptr;
                        bestDistance = distance;
                    }
                }
            }
        }

        // No emissive material, or occluder
        if (!pEmissiveMat || pOccluder)
        {
            continue;
        }

        float diffuseI = 0.0f;
        float specI = 0.0f;

        diffuseI = dot(normal, emitterDir);// / (bestDistance * .1f);

        if (diffuseI > 0.0f)
        {
            specI = dot(reflect, emitterDir);
            if (specI > 0.0f)
            {
                specI = pow(specI, 10);
                specI = std::max(0.0f, specI);

            }
            else
            {
                specI = 0.0f;
            }
        }
        else
        {
            diffuseI = 0.0f;
        }
        outputColor += (pEmissiveMat->emissive * material.albedo * diffuseI) + (material.specular * specI);
    }
    outputColor *= 1.f - material.reflectance;
    outputColor += material.emissive;
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
        device_buffer_ensure_screen_size(screenBufferData);
    }

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

