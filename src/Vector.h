#ifndef CODERSSTRIKEBACK_VECTOR_H
#define CODERSSTRIKEBACK_VECTOR_H

#include <cmath>
#include <iostream>
#include <limits>

class Vector {
    static float cosFast(float x) noexcept {
        constexpr float tp = 1.0/(2.0*M_PI);
        x *= tp;
        x -= 0.25 + std::floor(x + 0.25);
        x *= 16.0 * (std::abs(x) - 0.5);
        x += 0.225 * x * (std::abs(x) - 1.0);
        return x;
    }

    static float sinFast(float x) noexcept {
        const float B = 4.0/M_PI;
        const float C = -4.0/(M_PI*M_PI);
        const float Q = 0.775;
        const float P = 0.225;

        float y = B * x + C * x * std::abs(x);
        y = P * (y * std::abs(y) - y) + y +  Q * y + P * y * std::abs(y);
    }

public:
    static const int UN_SET = -1;
    mutable float length = UN_SET;
    mutable float lengthSq = UN_SET;
    float x;
    float y;

    Vector() {}

    Vector(float x, float y) : x(x), y(y) {}

    Vector(float x, float y, float length, float lengthSq) : x(x), y(y), length(length), lengthSq(lengthSq) {}

    // Use default.
    //Vector(const Vector& vector);

    ~Vector() {};

    static Vector fromMagAngle(const float magnitude, const float angle) {
        return Vector(magnitude * std::cos(angle), magnitude * std::sin(angle));
    }

    Vector rotated(float angle) {
        return Vector(x*std::cos(angle) - y*std::sin(angle), x*std::sin(angle) + y*std::cos(angle));
    }

    float getX() const {
        return x;
    }

    float getY() const {
        return y;
    }

    float getLength() const {
        if(length == UN_SET) {
            length = std::sqrt(getLengthSq());
        }
        return length;
    }

    float getLengthSq() const {
        if(lengthSq == UN_SET) {
            lengthSq = x*x + y*y;
        }
        return lengthSq;
    }

    void resetLengths() {
        lengthSq = UN_SET;
        length = UN_SET;
    }

    // Useful when there is no need to create another Vector instance and call getLengthSq().
    static float distSq(const Vector& a, const Vector& b) {
        return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
    }

    static float dist(const Vector& a, const Vector& b) {
        return std::sqrt(distSq(a, b));
    }

    // Could replace with overloading Vector * Vector.
    float dotProduct(const Vector& other) const {
        return x * other.x + y * other.y;
    };

    // Could replace with overloading Vector ^ Vector.
    float crossProduct(const Vector& other) const {
        return x * other.y - y * other.x;
    }

    /**
     * Project vector p onto this vector.
     */
    Vector project(const Vector& p) const {
        return (*this * (dotProduct(p) / getLengthSq()));
    }

    Vector normalize() const {
        return Vector(x / getLength(), y / getLength());
    }

    Vector tanget() const {
        return Vector(-y, x);
    }

    Vector &operator+=(const Vector &other) {
        x += other.x;
        y += other.y;
        length = UN_SET;
        lengthSq = UN_SET;
        return *this;
    }

    Vector operator+(const Vector &other) const {
        return Vector(*this) += other;
    }

    Vector &operator-=(const Vector &other) {
        x -= other.x;
        y -= other.y;
        length = UN_SET;
        lengthSq = UN_SET;
        return *this;
    }

    Vector operator-(const Vector &other) const {
        return Vector(*this) -= other;
    }

    Vector operator-() const {
        return Vector(-x, -y);
    }

    Vector operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        length = UN_SET;
        lengthSq = UN_SET;
        return *this;
    }

    Vector operator*(float scalar) const {
        return Vector(*this) *= scalar;
    }

    Vector &operator/=(float scalar) {
        x /= scalar;
        y /= scalar;
        length = UN_SET;
        lengthSq = UN_SET;
        return *this;
    }

    bool operator==(const Vector& other) const {
        if(x==other.x && y==other.y) return true;
        // Essentially equal.
        bool xeq = std::fabs(x - other.x) <= ( (std::fabs(x) > std::fabs(other.x) ? std::fabs(other.x) :
                                                std::fabs(x)) * std::numeric_limits<float>::epsilon());
        bool yeq = std::fabs(y - other.y) <= ( (std::fabs(y) > std::fabs(other.y) ? std::fabs(other.y) :
                                                std::fabs(y)) * std::numeric_limits<float>::epsilon());
        return xeq && yeq;
    }

    bool operator !=(const Vector& other) const {
        return !(*this == other);
    }

    friend std::ostream &operator<<(std::ostream &os, const Vector &vector) {
        os << "(" << vector.x << "," << vector.y << ")";
        return os;
    }
};

#endif //CODERSSTRIKEBACK_VECTOR_H
