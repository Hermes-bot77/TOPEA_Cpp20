#include "recovery_velocity.hpp"
#include "../../core/logger.hpp"

namespace topea::engine::recovery {

RecoveryVelocity::RecoveryVelocity(
    core::Price initial_velocity,
    double damping_factor)
    : base_velocity_(initial_velocity),
      current_velocity_(initial_velocity),
      damping_factor_(damping_factor) {
}

void RecoveryVelocity::update(
    core::Duration delta_time,
    double momentum,
    double volatility,
    double pressure) {

    if (frozen_) return;

    core::Price momentum_velocity = calculateMomentumVelocity(momentum, volatility);
    core::Price pressure_velocity = calculatePressureVelocity(pressure);

    // Combine velocities
    current_velocity_ = (momentum_velocity * 0.4) +
                        (pressure_velocity * 0.4) +
                        (base_velocity_ * 0.2);

    // Apply damping
    current_velocity_ *= damping_factor_;

    // Clamp to reasonable range
    current_velocity_ = std::max(0.1, std::min(5.0, current_velocity_));
}

core::Price RecoveryVelocity::getVelocity() const {
    return current_velocity_;
}

void RecoveryVelocity::accelerate(double factor) {
    if (!frozen_) {
        current_velocity_ *= factor;
        current_velocity_ = std::min(5.0, current_velocity_);
        TOPEA_DEBUG("RecoveryVelocity: Accelerated to {:.2f}", current_velocity_);
    }
}

void RecoveryVelocity::decelerate(double factor) {
    if (!frozen_) {
        current_velocity_ *= factor;
        current_velocity_ = std::max(0.1, current_velocity_);
        TOPEA_DEBUG("RecoveryVelocity: Decelerated to {:.2f}", current_velocity_);
    }
}

void RecoveryVelocity::resetToBase() {
    current_velocity_ = base_velocity_;
    TOPEA_DEBUG("RecoveryVelocity: Reset to base {:.2f}", base_velocity_);
}

void RecoveryVelocity::freeze() {
    frozen_ = true;
    TOPEA_DEBUG("RecoveryVelocity: Frozen");
}

void RecoveryVelocity::unfreeze() {
    frozen_ = false;
    TOPEA_DEBUG("RecoveryVelocity: Unfrozen");
}

bool RecoveryVelocity::isFrozen() const {
    return frozen_;
}

core::Price RecoveryVelocity::getDirection() const {
    return current_velocity_ > 0.0 ? 1.0 : -1.0;
}

core::Price RecoveryVelocity::calculateMomentumVelocity(
    double momentum,
    double volatility) const {

    // High positive momentum = low velocity (trailing)
    // High negative momentum = high velocity (catching up)
    double momentum_factor = 1.0 - (momentum * 0.5);  // -50% to +50%

    // Volatility affects velocity
    double volatility_factor = 1.0 / (1.0 + volatility * 0.3);

    return base_velocity_ * momentum_factor * volatility_factor;
}

core::Price RecoveryVelocity::calculatePressureVelocity(double pressure) const {
    // High pressure = higher velocity
    return base_velocity_ * (1.0 + pressure * 2.0);
}

} // namespace topea::engine::recovery
