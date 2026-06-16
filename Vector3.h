#ifndef VECTOR3_H
#define VECTOR3_H

#include <cmath>

/**
 * @brief Simple 3D vector for orbital calculations
 *
 * Used by OrbitalSimulator for position/velocity calculations.
 * Does NOT affect planet generation - purely for runtime simulation.
 */
struct Vec3 {
    double x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    // Vector operations
    Vec3 operator+(const Vec3& v) const {
        return Vec3(x + v.x, y + v.y, z + v.z);
    }

    Vec3 operator-(const Vec3& v) const {
        return Vec3(x - v.x, y - v.y, z - v.z);
    }

    Vec3 operator*(double s) const {
        return Vec3(x * s, y * s, z * s);
    }

    Vec3 operator/(double s) const {
        return Vec3(x / s, y / s, z / s);
    }

    // Dot product
    double dot(const Vec3& v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    // Cross product
    Vec3 cross(const Vec3& v) const {
        return Vec3(
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x
        );
    }

    // Magnitude
    double length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    // Normalize
    Vec3 normalize() const {
        double len = length();
        if (len > 0.0) {
            return *this / len;
        }
        return Vec3(0, 0, 0);
    }
};

#endif // VECTOR3_H
