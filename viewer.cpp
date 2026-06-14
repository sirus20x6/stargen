/**
 * @file viewer.cpp
 * @brief Real-time stellar system viewer using raylib
 *
 * Interactive visualization of generated stellar systems with orbital motion.
 * Features:
 * - Real-time orbital simulation
 * - Follow planet mode with smooth transitions
 * - Interactive camera controls
 * - Planet selection and tracking
 */

#include <iostream>
#include <cmath>
#include <cstdlib>      // for EXIT_FAILURE
#include <exception>    // for std::exception
#include <vector>
#include "raylib.h"
#include "stargen.h"
#include "StarGenerator.h"
#include "OrbitalSimulator.h"
#include "OrbitalStateManager.h"
#include "accrete.h"
#include "elements.h"
#include "planets.h"
#include "radius_tables.h"
#include "dole.h"
#include "solstation.h"
#include "jimb.h"
#include "omega_galaxy.h"
#include "ring_universe.h"
#include "ic3094.h"
#include "andromeda.h"
#include "star_trek.h"
#include "Planetary_Habitability_Laboratory.h"

// Screen dimensions
constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;

// Camera smoothing parameters
constexpr float CAMERA_LERP_SPEED = 5.0f;  // Units per second
constexpr float ZOOM_LERP_SPEED = 3.0f;    // Zoom rate

// Camera follow modes
enum class FollowMode {
    FREE,        // Free camera control
    FOLLOW_SUN,  // Center on sun
    FOLLOW_PLANET // Follow selected planet
};

// Enhanced camera/view settings
struct ViewState {
    Vector2 offset;          // Current camera offset (pan)
    Vector2 targetOffset;    // Target offset for smooth transition
    float zoom;              // Current zoom level (pixels per AU)
    float targetZoom;        // Target zoom for smooth transition
    float timeScale;         // Simulation speed multiplier
    bool paused;             // Simulation paused
    FollowMode followMode;   // Camera follow mode
    planet* followTarget;    // Planet to follow (nullptr = none)
    int followIndex;         // Index of followed planet
};

/**
 * @brief Linear interpolation
 */
float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

/**
 * @brief Vector2 linear interpolation
 */
Vector2 lerpVec2(Vector2 a, Vector2 b, float t) {
    return {lerp(a.x, b.x, t), lerp(a.y, b.y, t)};
}

/**
 * @brief Initialize all global data tables required for generation
 */
void initializeGenerationData() {
    initRadii();
    initGases();
    initPlanets();
    initDole();
    initSolStation();
    initJimb();
    initOmegaGalaxy();
    initRingUniverse();
    initIC3094();
    initAndromeda();
    initStarTrek();
    initPlanetaryHabitabilityLaboratory();
}

/**
 * @brief Generate a stellar system
 */
planet* generateSystem(StarGenerator& gen, accrete& acc, sun& the_sun, long seed) {
    gen.random_context.setSeed(seed);
    gen.config.flags = 0;
    gen.config.verbose_level = 0;

    the_sun.setMass(1.0);  // Solar mass
    the_sun.setLuminosity(1.0);

    acc.setRandomContext(&gen.random_context);

    generate_stellar_system(&gen, the_sun, false, nullptr, "S",
                           0, "Viewer System", 0.0, 0.0,
                           0.077, 1.0, true, true, acc);

    return gen.sim_context.innermost_planet;
}

/**
 * @brief Build planet list for indexed access
 */
std::vector<planet*> buildPlanetList(planet* system) {
    std::vector<planet*> planets;
    for (planet* p = system; p != nullptr; p = p->next_planet) {
        planets.push_back(p);
    }
    return planets;
}

/**
 * @brief Convert world position (AU) to screen position (pixels)
 */
Vector2 worldToScreen(const Vector3& worldPos, const ViewState& view) {
    float screenX = SCREEN_WIDTH / 2.0f + (worldPos.x * view.zoom + view.offset.x);
    float screenY = SCREEN_HEIGHT / 2.0f + (worldPos.y * view.zoom + view.offset.y);
    return {screenX, screenY};
}

/**
 * @brief Convert screen position to world position
 */
Vector2 screenToWorld(Vector2 screenPos, const ViewState& view) {
    float worldX = (screenPos.x - SCREEN_WIDTH / 2.0f - view.offset.x) / view.zoom;
    float worldY = (screenPos.y - SCREEN_HEIGHT / 2.0f - view.offset.y) / view.zoom;
    return {worldX, worldY};
}

/**
 * @brief Update camera to follow target
 */
void updateFollowCamera(ViewState& view, const OrbitalStateManager& manager) {
    if (view.followMode == FollowMode::FOLLOW_SUN) {
        // Center on sun at origin
        view.targetOffset = {0, 0};
    } else if (view.followMode == FollowMode::FOLLOW_PLANET && view.followTarget != nullptr) {
        // Follow selected planet
        Vector3 planetPos = manager.getPosition("main", view.followTarget);

        // Calculate offset to center planet
        view.targetOffset.x = -planetPos.x * view.zoom;
        view.targetOffset.y = -planetPos.y * view.zoom;

        // Auto-adjust zoom based on planet distance from sun
        float distance = planetPos.length();
        if (distance > 0.1f) {
            // Target zoom that keeps planet reasonably sized on screen
            float idealZoom = std::min(300.0f, std::max(20.0f, 200.0f / distance));
            view.targetZoom = idealZoom;
        }
    }
}

/**
 * @brief Find planet at screen position (for mouse picking)
 */
planet* findPlanetAtPosition(Vector2 screenPos, const std::vector<planet*>& planets,
                              const OrbitalStateManager& manager, const ViewState& view) {
    constexpr float CLICK_RADIUS = 10.0f;  // Pixel tolerance

    for (planet* p : planets) {
        Vector3 planetPos = manager.getPosition("main", p);
        Vector2 screenPlanetPos = worldToScreen(planetPos, view);

        float dx = screenPos.x - screenPlanetPos.x;
        float dy = screenPos.y - screenPlanetPos.y;
        float distSq = dx * dx + dy * dy;

        if (distSq <= CLICK_RADIUS * CLICK_RADIUS) {
            return p;
        }
    }

    return nullptr;
}

/**
 * @brief Draw the sun at the center
 */
void drawSun(const ViewState& view, bool isFollowTarget) {
    Vector2 sunPos = worldToScreen(Vector3(0, 0, 0), view);
    float sunRadius = std::max(5.0f, 10.0f * view.zoom / 50.0f);

    DrawCircleV(sunPos, sunRadius, YELLOW);
    DrawCircleV(sunPos, sunRadius * 1.2f, Fade(ORANGE, 0.3f));

    // Highlight if following
    if (isFollowTarget) {
        DrawCircleLines(sunPos.x, sunPos.y, sunRadius + 5, GREEN);
    }
}

/**
 * @brief Draw a planet
 */
void drawPlanet(planet* p, const Vector3& pos, const ViewState& view, bool isFollowTarget) {
    Vector2 screenPos = worldToScreen(pos, view);

    // Calculate planet visual radius (not to scale, for visibility)
    float visualRadius = 3.0f;
    if (p->getType() == tGasGiant) {
        visualRadius = 6.0f;
    } else if (p->getType() == tSubGasGiant) {
        visualRadius = 5.0f;
    }

    // Color based on planet type
    Color planetColor = GRAY;
    if (p->getType() == tGasGiant) {
        planetColor = ORANGE;
    } else if (p->getType() == tSubGasGiant) {
        planetColor = SKYBLUE;
    } else if (p->getSurfTemp() > FREEZING_POINT_OF_WATER) {
        planetColor = BLUE;  // Potentially habitable
    } else {
        planetColor = LIGHTGRAY;  // Rocky/icy
    }

    DrawCircleV(screenPos, visualRadius, planetColor);

    // Highlight if following
    if (isFollowTarget) {
        DrawCircleLines(screenPos.x, screenPos.y, visualRadius + 3, GREEN);
        DrawCircleLines(screenPos.x, screenPos.y, visualRadius + 5, GREEN);
    }
}

/**
 * @brief Draw orbit path
 */
void drawOrbit(planet* p, const ViewState& view) {
    float a = p->getA();  // Semi-major axis
    float e = p->getE();  // Eccentricity

    Vector2 center = worldToScreen(Vector3(0, 0, 0), view);
    float radiusX = a * view.zoom;
    float radiusY = a * (1.0f - e * e) * view.zoom;  // Semi-minor axis approximation

    // Draw ellipse (approximation with circle for now)
    DrawCircleLines(center.x, center.y, radiusX, Fade(DARKGRAY, 0.3f));
}

/**
 * @brief Draw UI overlay with system info
 */
void drawUI(const OrbitalStateManager& manager, const ViewState& view,
            int planetCount, float fps) {
    const int padding = 10;
    const int lineHeight = 20;
    int y = padding;

    DrawText(TextFormat("FPS: %.0f", fps), padding, y, 20, GREEN);
    y += lineHeight;

    DrawText(TextFormat("Time: %.2f years", manager.getCurrentTime()), padding, y, 20, WHITE);
    y += lineHeight;

    DrawText(TextFormat("Speed: %.1fx", view.timeScale), padding, y, 20, WHITE);
    y += lineHeight;

    if (view.paused) {
        DrawText("PAUSED", padding, y, 20, RED);
        y += lineHeight;
    }

    DrawText(TextFormat("Zoom: %.1f", view.zoom), padding, y, 20, WHITE);
    y += lineHeight;

    // Camera mode
    const char* modeText = "FREE";
    if (view.followMode == FollowMode::FOLLOW_SUN) {
        modeText = "FOLLOW: Sun";
    } else if (view.followMode == FollowMode::FOLLOW_PLANET) {
        modeText = TextFormat("FOLLOW: Planet %d", view.followIndex + 1);
    }
    DrawText(TextFormat("Mode: %s", modeText), padding, y, 20, YELLOW);
    y += lineHeight;

    // Controls
    y = SCREEN_HEIGHT - 160;
    DrawText("Controls:", padding, y, 20, YELLOW);
    y += lineHeight;
    DrawText("SPACE: Pause/Resume", padding, y, 16, WHITE);
    y += lineHeight;
    DrawText("+/-: Adjust speed", padding, y, 16, WHITE);
    y += lineHeight;
    DrawText("Mouse Wheel: Zoom", padding, y, 16, WHITE);
    y += lineHeight;
    DrawText("Mouse Drag: Pan (free mode)", padding, y, 16, WHITE);
    y += lineHeight;
    DrawText("Left Click: Select planet", padding, y, 16, WHITE);
    y += lineHeight;
    DrawText("F: Follow selected / cycle mode", padding, y, 16, WHITE);
    y += lineHeight;
    DrawText("1-9: Follow planet by number", padding, y, 16, WHITE);
    y += lineHeight;
    DrawText("ESC: Exit", padding, y, 16, WHITE);
}

int main() {
    // Initialize generation system. The data loaders throw std::runtime_error on
    // a missing/malformed file; fail gracefully instead of std::terminate().
    std::cout << "Initializing stellar system generator...\n";
    try {
        initializeGenerationData();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    // Generate a test system
    std::cout << "Generating stellar system (seed 42)...\n";
    StarGenerator gen;
    accrete acc;
    sun the_sun;
    planet* system = generateSystem(gen, acc, the_sun, 42);

    if (!system) {
        std::cerr << "ERROR: Failed to generate system\n";
        return 1;
    }

    // Build planet list for indexed access
    std::vector<planet*> planets = buildPlanetList(system);
    int planetCount = planets.size();
    std::cout << "Generated " << planetCount << " planets\n";

    // Initialize orbital simulation
    std::cout << "Initializing orbital simulator...\n";
    OrbitalStateManager manager(0.0);
    manager.addSystem("main", system);

    // Initialize view state with smooth camera
    ViewState view = {
        .offset = {0, 0},
        .targetOffset = {0, 0},
        .zoom = 50.0f,
        .targetZoom = 50.0f,
        .timeScale = 1.0f,
        .paused = false,
        .followMode = FollowMode::FREE,
        .followTarget = nullptr,
        .followIndex = -1
    };

    // Initialize window
    std::cout << "Starting viewer...\n";
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Stellar System Viewer - Advanced Camera");
    SetTargetFPS(60);

    // Main loop
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        // =====================================================================
        // INPUT HANDLING
        // =====================================================================

        // Pause/Resume
        if (IsKeyPressed(KEY_SPACE)) {
            view.paused = !view.paused;
            manager.setPaused(view.paused);
        }

        // Speed control
        if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
            view.timeScale *= 2.0f;
            manager.setTimeScale(view.timeScale);
        }
        if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
            view.timeScale *= 0.5f;
            if (view.timeScale < 0.1f) view.timeScale = 0.1f;
            manager.setTimeScale(view.timeScale);
        }

        // Follow mode toggle with F key
        if (IsKeyPressed(KEY_F)) {
            if (view.followMode == FollowMode::FREE) {
                view.followMode = FollowMode::FOLLOW_SUN;
                view.followTarget = nullptr;
                view.followIndex = -1;
            } else if (view.followMode == FollowMode::FOLLOW_SUN) {
                if (planetCount > 0) {
                    view.followMode = FollowMode::FOLLOW_PLANET;
                    view.followTarget = planets[0];
                    view.followIndex = 0;
                } else {
                    view.followMode = FollowMode::FREE;
                }
            } else if (view.followMode == FollowMode::FOLLOW_PLANET) {
                // Cycle to next planet
                view.followIndex = (view.followIndex + 1) % planetCount;
                if (view.followIndex == 0) {
                    // Cycled through all planets, go back to free
                    view.followMode = FollowMode::FREE;
                    view.followTarget = nullptr;
                    view.followIndex = -1;
                } else {
                    view.followTarget = planets[view.followIndex];
                }
            }
        }

        // Number keys for direct planet selection
        for (int i = 0; i < std::min(9, planetCount); i++) {
            if (IsKeyPressed(KEY_ONE + i)) {
                view.followMode = FollowMode::FOLLOW_PLANET;
                view.followTarget = planets[i];
                view.followIndex = i;
            }
        }

        // Mouse click to select planet
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mousePos = GetMousePosition();
            planet* clicked = findPlanetAtPosition(mousePos, planets, manager, view);

            if (clicked != nullptr) {
                view.followMode = FollowMode::FOLLOW_PLANET;
                view.followTarget = clicked;
                // Find index
                for (int i = 0; i < planetCount; i++) {
                    if (planets[i] == clicked) {
                        view.followIndex = i;
                        break;
                    }
                }
            }
        }

        // Zoom (works in all modes)
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            view.targetZoom *= (1.0f + wheel * 0.1f);
            if (view.targetZoom < 1.0f) view.targetZoom = 1.0f;
            if (view.targetZoom > 500.0f) view.targetZoom = 500.0f;
        }

        // Pan with mouse drag (only in free mode)
        if (view.followMode == FollowMode::FREE && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 delta = GetMouseDelta();
            view.targetOffset.x += delta.x;
            view.targetOffset.y += delta.y;
            view.offset = view.targetOffset;  // Instant for manual control
        }

        // =====================================================================
        // UPDATE
        // =====================================================================

        // Update simulation
        float simDeltaTime = deltaTime * (1.0f / 365.25f);
        manager.update(simDeltaTime);

        // Update follow camera target positions
        updateFollowCamera(view, manager);

        // Smooth camera transitions
        float lerpFactor = std::min(1.0f, CAMERA_LERP_SPEED * deltaTime);
        view.offset = lerpVec2(view.offset, view.targetOffset, lerpFactor);

        float zoomLerpFactor = std::min(1.0f, ZOOM_LERP_SPEED * deltaTime);
        view.zoom = lerp(view.zoom, view.targetZoom, zoomLerpFactor);

        // =====================================================================
        // RENDER
        // =====================================================================

        BeginDrawing();
        ClearBackground(BLACK);

        // Draw sun
        bool sunIsFollowTarget = (view.followMode == FollowMode::FOLLOW_SUN);
        drawSun(view, sunIsFollowTarget);

        // Draw planets and orbits
        for (int i = 0; i < planetCount; i++) {
            planet* p = planets[i];

            // Draw orbit path
            drawOrbit(p, view);

            // Get current position
            Vector3 pos = manager.getPosition("main", p);

            // Check if this is the follow target
            bool isFollowTarget = (view.followMode == FollowMode::FOLLOW_PLANET &&
                                   view.followTarget == p);

            // Draw planet
            drawPlanet(p, pos, view, isFollowTarget);
        }

        // Draw UI
        drawUI(manager, view, planetCount, GetFPS());

        EndDrawing();
    }

    // Cleanup
    CloseWindow();
    acc.free_generations();

    std::cout << "Viewer closed.\n";
    return 0;
}
