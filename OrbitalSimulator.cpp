#include "OrbitalSimulator.h"
#include <cmath>

// Mathematical constants
static const double PI = 3.14159265358979323846;
static const double TWO_PI = 2.0 * PI;

void OrbitalSimulator::initializeSystem(planet* system_root) {
    states_.clear();

    // Walk the planet linked list
    for (planet* p = system_root; p != nullptr; p = p->next_planet) {
        // Create state for this planet (reads orbital parameters, doesn't modify them)
        OrbitalState state;
        state.last_update_time = current_time_;
        state.mean_anomaly = 0.0;  // Start at perihelion for simplicity

        // Store initial state
        states_[p] = state;

        // Recursively initialize moons
        if (p->first_moon != nullptr) {
            initializeSystem(p->first_moon);
        }
    }
}

void OrbitalSimulator::advance(double delta_time) {
    current_time_ += delta_time * time_scale_;
    updatePositions();
}

void OrbitalSimulator::setTime(double time) {
    current_time_ = time;
    updatePositions();
}

void OrbitalSimulator::updatePositions() {
    for (auto& [planet_ptr, state] : states_) {
        // Read orbital parameters from planet (const access - no modification)
        double a = planet_ptr->getA();           // Semi-major axis (AU)
        double e = planet_ptr->getE();           // Eccentricity
        double i = planet_ptr->getInclination(); // Inclination (degrees)
        double omega = planet_ptr->getAscendingNode();  // Long. of ascending node (degrees)
        double w = planet_ptr->getLongitudeOfPericenter();  // Longitude of pericenter (degrees)

        // Convert angles to radians
        double i_rad = i * PI / 180.0;
        double omega_rad = omega * PI / 180.0;
        double w_rad = w * PI / 180.0;

        // Calculate mean anomaly at current time
        state.mean_anomaly = calculateMeanAnomaly(a, current_time_, 0.0);

        // Solve Kepler's equation for eccentric anomaly
        double E = solveKeplersEquation(state.mean_anomaly, e);

        // Calculate position in orbital plane
        double cos_E = std::cos(E);
        double sin_E = std::sin(E);

        double x_orb = a * (cos_E - e);
        double y_orb = a * std::sqrt(1.0 - e * e) * sin_E;

        // Transform to 3D space using rotation matrices
        double cos_omega = std::cos(omega_rad);
        double sin_omega = std::sin(omega_rad);
        double cos_w = std::cos(w_rad);
        double sin_w = std::sin(w_rad);
        double cos_i = std::cos(i_rad);
        double sin_i = std::sin(i_rad);

        // Rotation matrix: R = R_z(Ω) * R_x(i) * R_z(ω)
        double x = (cos_omega * cos_w - sin_omega * sin_w * cos_i) * x_orb +
                   (-cos_omega * sin_w - sin_omega * cos_w * cos_i) * y_orb;

        double y = (sin_omega * cos_w + cos_omega * sin_w * cos_i) * x_orb +
                   (-sin_omega * sin_w + cos_omega * cos_w * cos_i) * y_orb;

        double z = (sin_w * sin_i) * x_orb + (cos_w * sin_i) * y_orb;

        state.position = Vector3(x, y, z);

        // Calculate velocity (optional, for future use)
        // v = (μ/h) * [-sin(ν), e + cos(ν), 0] in orbital plane
        // Then rotate to 3D space
        // For now, we'll leave velocity as zero - can be added later if needed
        state.velocity = Vector3(0, 0, 0);

        state.last_update_time = current_time_;
    }
}

Vector3 OrbitalSimulator::getPosition(planet* p) const {
    auto it = states_.find(p);
    if (it != states_.end()) {
        return it->second.position;
    }
    return Vector3(0, 0, 0);
}

Vector3 OrbitalSimulator::getVelocity(planet* p) const {
    auto it = states_.find(p);
    if (it != states_.end()) {
        return it->second.velocity;
    }
    return Vector3(0, 0, 0);
}

bool OrbitalSimulator::hasState(planet* p) const {
    return states_.find(p) != states_.end();
}

double OrbitalSimulator::solveKeplersEquation(double M, double e,
                                               double tolerance,
                                               int max_iterations) const {
    // Normalize M to [0, 2π)
    M = std::fmod(M, TWO_PI);
    if (M < 0) M += TWO_PI;

    // Initial guess for eccentric anomaly
    double E = M + e * std::sin(M);

    // Newton-Raphson iteration
    for (int i = 0; i < max_iterations; ++i) {
        double f = E - e * std::sin(E) - M;
        double f_prime = 1.0 - e * std::cos(E);

        double delta = f / f_prime;
        E -= delta;

        // Check convergence
        if (std::abs(delta) < tolerance) {
            break;
        }
    }

    return E;
}

double OrbitalSimulator::calculateMeanAnomaly(double a, double t, double M0) const {
    if (a <= 0) return 0.0;

    // Mean motion: n = 2π / T = 2π / (a^(3/2))
    // (Using G*M_sun = 1 in units where 1 AU³/year² = G*M_sun)
    double n = TWO_PI / (std::pow(a, 1.5));

    // Mean anomaly: M = M0 + n*t
    double M = M0 + n * t;

    return M;
}

Vector3 OrbitalSimulator::orbitalToCartesian(double a, double e, double i,
                                              double omega, double w, double M) const {
    // This is an alternative implementation (not currently used)
    // Kept for reference

    // Solve Kepler's equation
    double E = solveKeplersEquation(M, e);

    // Position in orbital plane
    double x_orb = a * (std::cos(E) - e);
    double y_orb = a * std::sqrt(1.0 - e * e) * std::sin(E);

    // Rotation matrices
    double cos_omega = std::cos(omega);
    double sin_omega = std::sin(omega);
    double cos_w = std::cos(w);
    double sin_w = std::sin(w);
    double cos_i = std::cos(i);
    double sin_i = std::sin(i);

    // Transform to 3D
    double x = (cos_omega * cos_w - sin_omega * sin_w * cos_i) * x_orb +
               (-cos_omega * sin_w - sin_omega * cos_w * cos_i) * y_orb;

    double y = (sin_omega * cos_w + cos_omega * sin_w * cos_i) * x_orb +
               (-sin_omega * sin_w + cos_omega * cos_w * cos_i) * y_orb;

    double z = (sin_w * sin_i) * x_orb + (cos_w * sin_i) * y_orb;

    return Vector3(x, y, z);
}
