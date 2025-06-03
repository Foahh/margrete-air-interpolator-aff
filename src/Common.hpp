#pragma once
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

constexpr int MG_RESOLUTION = 1920;
constexpr int MG_BEAT = MG_RESOLUTION / 4;

enum class EasingMode {
    Linear = 0,
    In = 1,
    Out = 2,
};

enum class EasingKind {
    Sine = 0,
    Power = 1,
    Circular = 2,
};

struct ConfigContext {
    EasingKind esType{EasingKind::Sine};
    int esExponent{2};
    int width = 1;
    int til = 0;
    int snap = 5;
    double esEpsilon{0.2};
    double yOffset = 0.0;
    bool clamp = true;
};

class EasingSolver {
public:
    virtual ~EasingSolver() = default;

    double Solve(const double u, const EasingMode e) const {
        ValidateRange(u);
        switch (e) {
            case EasingMode::In:
                return SolveIn(u);
            case EasingMode::Out:
                return SolveOut(u);
            case EasingMode::Linear:
            default:
                return u;
        }
    }

    double InverseSolve(const double v, const EasingMode e) const {
        ValidateRange(v);
        switch (e) {
            case EasingMode::In:
                return InverseIn(v);
            case EasingMode::Out:
                return InverseOut(v);
            case EasingMode::Linear:
            default:
                return v;
        }
    }

protected:
    virtual double SolveIn(double t) const = 0;
    virtual double SolveOut(double t) const = 0;
    virtual double InverseIn(double y) const = 0;
    virtual double InverseOut(double y) const = 0;

    static void ValidateRange(const double x) {
        if (x < 0.0 || x > 1.0) {
            throw std::out_of_range("Value must be in [0, 1]");
        }
    }
};

class EasingSolverForward : public EasingSolver {
protected:
    double SolveOut(const double t) const override {
        return 1.0 - SolveIn(1.0 - t);
    }

    double InverseOut(const double y) const override {
        return 1.0 - InverseIn(1.0 - y);
    }
};

class EasingSolverBackward : public EasingSolver {
protected:
    double SolveIn(const double t) const override {
        return 1.0 - SolveOut(1.0 - t);
    }

    double InverseIn(const double y) const override {
        return 1.0 - InverseOut(1.0 - y);
    }
};

class SineSolver final : public EasingSolverForward {
protected:
    double SolveIn(const double t) const override {
        return std::sin(t * std::numbers::pi_v<double> / 2.0);
    }

    double InverseIn(const double y) const override {
        return 2.0 / std::numbers::pi_v<double> * std::asin(y);
    }
};

class PowerSolver final : public EasingSolverBackward {
public:
    explicit PowerSolver(const int exponent) : m_exponent(exponent) {
        if (exponent == 0) {
            throw std::invalid_argument("Exponent cannot be zero");
        }
    }

protected:
    double SolveOut(const double t) const override {
        return std::pow(t, m_exponent);
    }

    double InverseOut(const double y) const override {
        return std::pow(y, 1.0 / static_cast<double>(m_exponent));
    }

private:
    int m_exponent{0};
};

class CircularSolver final : public EasingSolverBackward {
public:
    explicit CircularSolver(const double linearity = 0.0) : epsilon(linearity) {
        if (linearity < 0.0 || linearity > 1.0) {
            throw std::invalid_argument("Linearity must be in [0, 1]");
        }
    }

protected:
    double SolveOut(const double t) const override {
        return epsilon * t + (1.0 - epsilon) * (1.0 - std::sqrt(1.0 - t * t));
    }

    double InverseOut(const double y) const override {
        if (epsilon == 1.0) {
            return y;
        }

        if (epsilon == 0.0) {
            return std::sqrt(1.0 - (1.0 - y) * (1.0 - y));
        }

        const double bias = epsilon / (1.0 - epsilon);
        const double offset = (1.0 - epsilon - y) / (1.0 - epsilon);
        const double qA = 1.0 + bias * bias;
        const double qB = 2.0 * offset * bias;
        const double qC = offset * offset - 1.0;
        const double discriminant = (std::max) (0.0, qB * qB - 4.0 * qA * qC);

        return std::clamp((-qB + std::sqrt(discriminant)) / (2.0 * qA), 0.0, 1.0);
    }

private:
    double epsilon;
};
