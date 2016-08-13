#ifndef CODERSSTRIKEBACK_VECTOR_H
#define CODERSSTRIKEBACK_VECTOR_H

#include <cmath>
#include <iostream>

class Vector {
public:
    static const int UN_SET = -1;
    mutable double length = UN_SET;
    mutable double lengthSq = UN_SET;
    double x;
    double y;

    Vector() {}

    Vector(double x, double y) : x(x), y(y) {}

    Vector(double x, double y, double length, double lengthSq) : x(x), y(y), length(length), lengthSq(lengthSq) {}

    // Use default.
    //Vector(const Vector& vector);

    ~Vector() {};

    static Vector fromMagAngle(double magnitude, double angle) {
        return Vector(magnitude * std::cos(angle), magnitude * std::sin(angle));
    }

    double getX() const {
        return x;
    }

    double getY() const {
        return y;
    }

    double getLength() const {
        if(length == UN_SET) {
            length = std::sqrt(getLengthSq());
        }
        return length;
    }

    double getLengthSq() const {
        if(lengthSq == UN_SET) {
            lengthSq = x*x + y*y;
        }
        return lengthSq;
    }

    // Could replace with overloading Vector * Vector.
    double dotProduct(const Vector& other) const {
        return x * other.x + y * other.y;
    };

    // Could replace with overloading Vector ^ Vector.
    double crossProduct(const Vector& other) const {
        return x * other.y - y * other.x;
    }

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

    Vector operator*=(double scalar) {
        x *= scalar;
        y *= scalar;
        length = UN_SET;
        lengthSq = UN_SET;
        return *this;
    }

    Vector operator*(double scalar) const {
        return Vector(*this) *= scalar;
    }

    Vector &operator/=(double scalar) {
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
