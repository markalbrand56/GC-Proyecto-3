#include <SDL2/SDL.h>
#include <SDL_events.h>
#include <SDL_render.h>
#include <SDL_image.h>
#include <cstdlib>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/geometric.hpp>
#include <string>
#include <glm/glm.hpp>
#include <vector>
#include <print.h>

#include "color.h"
#include "object.h"
#include "sphere.h"
#include "cube.h"
#include "light.h"
#include "camera.h"
#include "skybox.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const float ASPECT_RATIO = static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT);
const int MAX_RECURSION_DEPTH = 3;
const float SHADOW_BIAS = 0.0001f;

SDL_Renderer* renderer;
std::vector<Object*> objects;
Light light(glm::vec3(-1.0, 0, 10), 1.5f, Color(255, 255, 255));
Camera camera(glm::vec3(0.0, 0.0, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 10.0f);
Skybox skybox("assets/sky.jpg");

void point(glm::vec2 position, Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawPoint(renderer, position.x, position.y);
}


float castShadow(const glm::vec3& shadowOrig, const glm::vec3& lightDir, const std::vector<Object*>& objects, Object* hitObject) {
    for (auto& obj : objects) {
        if (obj != hitObject) {
            Intersect shadowIntersect = obj->rayIntersect(shadowOrig, lightDir);
            if (shadowIntersect.isIntersecting && shadowIntersect.dist > 0) {  // zbuffer?
                const float shadowIntensity =  (1.0f - glm::min(1.0f, shadowIntersect.dist / glm::length(light.position - shadowOrig)));
                return shadowIntensity;
            }
        }
    }

    return 1.0f;
}

Color castRay(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const short recursion = 0) {
    float zBuffer = 99999;
    Object* hitObject = nullptr;
    Intersect intersect;

    for (const auto& object : objects) {
        Intersect i = object->rayIntersect(rayOrigin, rayDirection);
        if (i.isIntersecting && i.dist < zBuffer) {
            zBuffer = i.dist;
            hitObject = object;
            intersect = i;
        }
    }

    if (!intersect.isIntersecting || recursion >= MAX_RECURSION_DEPTH) {
        return skybox.getColor(rayDirection);  // Sky color
    }

    glm::vec3 lightDir = glm::normalize(light.position - intersect.point);
    glm::vec3 viewDir = glm::normalize(rayOrigin - intersect.point);
    glm::vec3 reflectDir = glm::reflect(-lightDir, intersect.normal);

    float shadowIntensity = castShadow(
        intersect.point + intersect.normal,
        lightDir, objects, hitObject);

    float diffuseLightIntensity = std::max(0.0f, glm::dot(intersect.normal, lightDir));
    float specReflection = glm::dot(viewDir, reflectDir);

    Material mat = hitObject->material;

    float specLightIntensity = std::pow(std::max(0.0f, glm::dot(viewDir, reflectDir)), mat.specularCoefficient);

    Color diffuseLight = mat.diffuse * light.intensity * diffuseLightIntensity * mat.albedo * shadowIntensity;

    Color specularLight = light.color * light.intensity * specLightIntensity * mat.specularAlbedo * shadowIntensity;

    // If the material has a texture, apply texture mapping
    if (mat.texture != nullptr) {
        int textureWidth = mat.texture->w;
        int textureHeight = mat.texture->h;

        float u = glm::clamp(glm::dot(intersect.normal, glm::vec3(1.0, 0.0, 0.0)), 0.0f, 1.0f);
        float v = glm::clamp(glm::dot(intersect.normal, glm::vec3(0.0, 1.0, 0.0)), 0.0f, 1.0f);

        int texX = static_cast<int>(u * textureWidth) % textureWidth;
        int texY = static_cast<int>(v * textureHeight) % textureHeight;

        Uint8* pixels = static_cast<Uint8*>(mat.texture->pixels);
        Uint8 r = pixels[texY * mat.texture->pitch + texX * mat.texture->format->BytesPerPixel];
        Uint8 g = pixels[texY * mat.texture->pitch + texX * mat.texture->format->BytesPerPixel + 1];
        Uint8 b = pixels[texY * mat.texture->pitch + texX * mat.texture->format->BytesPerPixel + 2];

        diffuseLight = Color(r / 255.0f, g / 255.0f, b / 255.0f);
    }

    // If the material is reflective, cast a reflected ray
    Color reflectedColor(0.0f, 0.0f, 0.0f);
    if (mat.reflectivity > 0) {
        glm::vec3 offsetOrigin = intersect.point + intersect.normal * SHADOW_BIAS;
        reflectedColor = castRay(offsetOrigin, reflectDir, recursion + 1);
    }

    // If the material is refractive, cast a refracted ray
    Color refractedColor(0.0f, 0.0f, 0.0f);
    if (mat.transparency > 0) {
        glm::vec3 refractDir = glm::refract(rayDirection, intersect.normal, mat.refractionIndex);
        glm::vec3 offsetOrigin = intersect.point - intersect.normal * SHADOW_BIAS;
        refractedColor = castRay(offsetOrigin, refractDir, recursion + 1);
    }

    Color color = (diffuseLight + specularLight) * (1 - mat.reflectivity - mat.transparency) + reflectedColor * mat.reflectivity + refractedColor * mat.transparency;

    return color;
}


void setUp() {
    Material rubber = {
        Color(80, 0, 0),   // diffuse
        0.9,
        0.1,
        10.0f,
        0.0f,
        0.0f
    };

    Material ivory = {
        Color(100, 100, 80),
        0.5,
        0.5,
        50.0f,
        0.2f,
        0.0f
    };

    Material mirror(
        Color(255, 255, 255),
        0.0f,
        10.0f,
        1425.0f,
        0.9f,
        0.0f
    );

    Material glass(
        Color(255, 255, 255),
        0.1f,
        1.0f,
        125.0f,
        0.0f,
        0.9f,
        0.1f
    );

    Material obsidiana(
        Color(255, 255, 255),
        0.1f,
        1.0f,
        125.0f,
        0.0f,
        0.0f,
        0.0f,
        IMG_Load("assets/textures/obsidian.png")
    );

    Material oro(
        Color(255, 255, 255),
        0.1f,
        1.0f,
        125.0f,
        0.0f,
        0.0f,
        0.0f,
        IMG_Load("assets/textures/gold.png")
    );

    // obsidiana
    objects.push_back(new Cube(glm::vec3(-1.0f, -3.0f, 0.0f), glm::vec3(0.0f, -2.0f, 1.0f), obsidiana));
    objects.push_back(new Cube(glm::vec3(0.0f, -3.0f, 0.0f), glm::vec3(1.0f, -2.0f, 1.0f), obsidiana));
    objects.push_back(new Cube(glm::vec3(-2.0f, -2.0f, 0.0f), glm::vec3(-1.0f, -1.0f, 1.0f), obsidiana));
    objects.push_back(new Cube(glm::vec3(-2.0f, -1.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 1.0f), obsidiana));
    objects.push_back(new Cube(glm::vec3(-2.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 1.0f, 1.0f), obsidiana));
    objects.push_back(new Cube(glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(0.0f, 2.0f, 1.0f), obsidiana));

    // bloque de oro
    objects.push_back(new Cube(glm::vec3(-1.0f, 2.0f, 0.0f), glm::vec3(0.0f, 3.0f, 1.0f), oro));

    // lava
    objects.push_back(new Cube(glm::vec3(-1.0f, -3.0f, 1.0f), glm::vec3(0.0f, -2.0f, 2.0f), glass));
    objects.push_back(new Cube(glm::vec3(0.0f, -3.0f, 1.0f), glm::vec3(1.0f, -2.0f, 2.0f), glass));

    // nether brick
    objects.push_back(new Cube(glm::vec3(-2.0f, -3.0f, 1.0f), glm::vec3(-1.0f, -2.0f, 2.0f), rubber));
    objects.push_back(new Cube(glm::vec3(-2.0f, -3.0f, 2.0f), glm::vec3(-1.0f, -2.0f, 3.0f), rubber));
    objects.push_back(new Cube(glm::vec3(-1.0f, -3.0f, 2.0f), glm::vec3(0.0f, -2.0f, 3.0f), rubber));
    objects.push_back(new Cube(glm::vec3(0.0f, -3.0f, 2.0f), glm::vec3(1.0f, -2.0f, 3.0f), rubber));

    // netherrack
    objects.push_back(new Cube(glm::vec3(1.0f, -3.0f, 1.0f), glm::vec3(2.0f, -2.0f, 2.0f), ivory));
    objects.push_back(new Cube(glm::vec3(1.0f, -3.0f, 0.0f), glm::vec3(2.0f, -2.0f, 1.0f), ivory));

}

void render() {
    float fov = 3.1415/3;
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            /*
            float random_value = static_cast<float>(std::rand())/static_cast<float>(RAND_MAX);
            if (random_value < 0.0) {
                continue;
            }
            */


            float screenX = (2.0f * (x + 0.5f)) / SCREEN_WIDTH - 1.0f;
            float screenY = -(2.0f * (y + 0.5f)) / SCREEN_HEIGHT + 1.0f;
            screenX *= ASPECT_RATIO;
            screenX *= tan(fov/2.0f);
            screenY *= tan(fov/2.0f);


            glm::vec3 cameraDir = glm::normalize(camera.target - camera.position);

            glm::vec3 cameraX = glm::normalize(glm::cross(cameraDir, camera.up));
            glm::vec3 cameraY = glm::normalize(glm::cross(cameraX, cameraDir));
            glm::vec3 rayDirection = glm::normalize(
                cameraDir + cameraX * screenX + cameraY * screenY
            );
           
            Color pixelColor = castRay(camera.position, rayDirection);
            /* Color pixelColor = castRay(glm::vec3(0,0,20), glm::normalize(glm::vec3(screenX, screenY, -1.0f))); */

            point(glm::vec2(x, y), pixelColor);
        }
    }
}

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    // Create a window
    SDL_Window* window = SDL_CreateWindow("Hello World - FPS: 0", 
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                          SCREEN_WIDTH, SCREEN_HEIGHT, 
                                          SDL_WINDOW_SHOWN);

    if (!window) {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create a renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer) {
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;

    int frameCount = 0;
    Uint32 startTime = SDL_GetTicks();
    Uint32 currentTime = startTime;
    
    setUp();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            if (event.type == SDL_KEYDOWN) {
                switch(event.key.keysym.sym) {
                    case SDLK_UP:
                        camera.move(-1.0f);
                        break;
                    case SDLK_DOWN:
                        camera.move(1.0f);
                        break;
                    case SDLK_LEFT:
                        camera.rotate(-1.0f, 0.0f);
                        break;
                    case SDLK_RIGHT:
                        camera.rotate(1.0f, 0.0f);
                        break;
                 }
            }


        }

        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        render();

        // Present the renderer
        SDL_RenderPresent(renderer);

        frameCount++;

        // Calculate and display FPS
        if (SDL_GetTicks() - currentTime >= 1000) {
            currentTime = SDL_GetTicks();
            std::string title = "Hello World - FPS: " + std::to_string(frameCount);
            SDL_SetWindowTitle(window, title.c_str());
            frameCount = 0;
        }
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

