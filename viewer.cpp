/**
 * @file viewer.cpp
 * @brief Real-time stellar system viewer using raylib
 *
 * Interactive visualization of generated stellar systems with orbital motion.
 * Demonstrates integration of:
 * - System generation (StarGenerator)
 * - Orbital simulation (OrbitalStateManager)
 * - Real-time rendering (raylib)
 */

#include <iostream>
#include <cmath>
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

// Camera/view settings
struct ViewState {
    Vector2 offset;      // Camera offset (pan)
    float zoom;          // Zoom level (pixels per AU)
    float timeScale;     // Simulation speed multiplier
    bool paused;         // Simulation paused
};

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
 * @brief Convert world position (AU) to screen position (pixels)
 */
Vector2 worldToScreen(const Vector3& worldPos, const ViewState& view) {
    float screenX = SCREEN_WIDTH / 2.0f + (worldPos.x * view.zoom + view.offset.x);
    float screenY = SCREEN_HEIGHT / 2.0f + (worldPos.y * view.zoom + view.offset.y);
    return {screenX, screenY};
}

/**
 * @brief Draw the sun at the center
 */
void drawSun(const ViewState& view) {
    Vector2 sunPos = worldToScreen(Vector3(0, 0, 0), view);
    float sunRadius = std::max(5.0f, 10.0f * view.zoom / 50.0f);
    DrawCircleV(sunPos, sunRadius, YELLOW);
    DrawCircleV(sunPos, sunRadius * 1.2f, Fade(ORANGE, 0.3f));
}

/**
 * @brief Draw a planet
 */
void drawPlanet(planet* p, const Vector3& pos, const ViewState& view) {
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
void drawUI(const OrbitalStateManager& manager, const ViewState& view, float fps) {
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

    // Controls
    y = SCREEN_HEIGHT - 120;
    DrawText("Controls:", padding, y, 20, YELLOW);
    y += lineHeight;
    DrawText("SPACE: Pause/Resume", padding, y, 16, WHITE);
    y += lineHeight;
    DrawText("+/-: Adjust speed", padding, y, 16, WHITE);
    y += lineHeight;
    DrawText("Mouse Wheel: Zoom", padding, y, 16, WHITE);
    y += lineHeight;
    DrawText("Mouse Drag: Pan", padding, y, 16, WHITE);
    y += lineHeight;
    DrawText("ESC: Exit", padding, y, 16, WHITE);
}

int main() {
    // Initialize generation system
    std::cout << "Initializing stellar system generator...\n";
    initializeGenerationData();

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

    // Count planets
    int planetCount = 0;
    for (planet* p = system; p != nullptr; p = p->next_planet) {
        planetCount++;
    }
    std::cout << "Generated " << planetCount << " planets\n";

    // Initialize orbital simulation
    std::cout << "Initializing orbital simulator...\n";
    OrbitalStateManager manager(0.0);
    manager.addSystem("main", system);

    // Initialize view state
    ViewState view = {
        .offset = {0, 0},
        .zoom = 50.0f,        // 50 pixels per AU
        .timeScale = 1.0f,    // 1x speed
        .paused = false
    };

    // Initialize window
    std::cout << "Starting viewer...\n";
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Stellar System Viewer");
    SetTargetFPS(60);

    // Main loop
    while (!WindowShouldClose()) {
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

        // Zoom
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            view.zoom *= (1.0f + wheel * 0.1f);
            if (view.zoom < 1.0f) view.zoom = 1.0f;
            if (view.zoom > 500.0f) view.zoom = 500.0f;
        }

        // Pan with mouse drag
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 delta = GetMouseDelta();
            view.offset.x += delta.x;
            view.offset.y += delta.y;
        }

        // =====================================================================
        // UPDATE
        // =====================================================================

        float deltaTime = GetFrameTime();

        // Convert frame time to years (assuming 1 second = 1 day by default)
        // This gives us smooth animation
        float simDeltaTime = deltaTime * (1.0f / 365.25f);

        manager.update(simDeltaTime);

        // =====================================================================
        // RENDER
        // =====================================================================

        BeginDrawing();
        ClearBackground(BLACK);

        // Draw sun
        drawSun(view);

        // Draw planets and orbits
        for (planet* p = system; p != nullptr; p = p->next_planet) {
            // Draw orbit path
            drawOrbit(p, view);

            // Get current position
            Vector3 pos = manager.getPosition("main", p);

            // Draw planet
            drawPlanet(p, pos, view);
        }

        // Draw UI
        drawUI(manager, view, GetFPS());

        EndDrawing();
    }

    // Cleanup
    CloseWindow();
    acc.free_generations();

    std::cout << "Viewer closed.\n";
    return 0;
}
