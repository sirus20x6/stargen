#ifndef VECTOR3_H
#define VECTOR3_H

#include <cmath>

/**
 * @brief Simple 3D vector for orbital calculations
 *
 * Used by OrbitalSimulator for position/velocity calculations.
 * Does NOT affect planet generation - purely for runtime simulation.
 */
struct Vector3 {
    double x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    // Vector operations
    Vector3 operator+(const Vector3& v) const {
        return Vector3(x + v.x, y + v.y, z + v.z);
    }

    Vector3 operator-(const Vector3& v) const {
        return Vector3(x - v.x, y - v.y, z - v.z);
    }

    Vector3 operator*(double s) const {
        return Vector3(x * s, y * s, z * s);
    }

    Vector3 operator/(double s) const {
        return Vector3(x / s, y / s, z / s);
    }

    // Dot product
    double dot(const Vector3& v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    // Cross product
    Vector3 cross(const Vector3& v) const {
        return Vector3(
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
    Vector3 normalize() const {
        double len = length();
        if (len > 0.0) {
            return *this / len;
        }
        return Vector3(0, 0, 0);
    }
};

#endif // VECTOR3_H
