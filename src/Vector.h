#ifndef CODERSSTRIKEBACK_VECTOR_H
#define CODERSSTRIKEBACK_VECTOR_H

#include <cmath>
#include <iostream>

class Vector {
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

    static Vector fromMagAngle(float magnitude, float angle) {
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
        return (x == other.x && y == other.y);
    }

    bool operator !=(const Vector& other) const {
        return !(*this == other);
    }

    friend std::ostream &operator<<(std::ostream &os, const Vector &vector) {
        os << "(" << vector.x << "," << vector.y << ")";
        return
                os;
    }
};

#endif //CODERSSTRIKEBACK_VECTOR_H
