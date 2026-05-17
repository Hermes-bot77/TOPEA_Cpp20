#ifndef TOPEA_ENGINE_RECOVERY_VELOCITY_HPP
#define TOPEA_ENGINE_RECOVERY_VELOCITY_HPP

#include "../../core/types.hpp"
#include <cmath>

namespace topea::engine::recovery {

// ============================================================
// RECOVERY VELOCITY ENGINE
// ============================================================

/**
 * Manages velocity and acceleration of recovery orders.
 * Supports:
 * - Dynamic acceleration/deceleration
 * - Momentum-based velocity
 * - Elastic movement with damping
 * - Distance-based velocity adaptation
 */
class RecoveryVelocity {
public:
    explicit RecoveryVelocity(
        core::Price initial_velocity = 1.0,
        double damping_factor = 0.95);

    /**
     * Update velocity based on market conditions
     */
    void update(
        core::Duration delta_time,
        double momentum,
        double volatility,
        double pressure);

    /**
     * Get current velocity
     */
    core::Price getVelocity() const;

    /**
     * Accelerate recovery
     */
    void accelerate(double factor = 1.5);

    /**
     * Decelerate recovery
     */
    void decelerate(double factor = 0.7);

    /**
     * Set velocity to base
     */
    void resetToBase();

    /**
     * Freeze velocity (stop movement)
     */
    void freeze();

    /**
     * Unfreeze velocity
     */
    void unfreeze();

    /**
     * Check if frozen
     */
    bool isFrozen() const;

    /**
     * Get velocity direction
     */
    double getDirection() const;

private:
    core::Price base_velocity_;
    core::Price current_velocity_;
    double damping_factor_;
    bool frozen_ = false;

    core::Price calculateMomentumVelocity(double momentum, double volatility) const;
    core::Price calculatePressureVelocity(double pressure) const;
};

} // namespace topea::engine::recovery

#endif // TOPEA_ENGINE_RECOVERY_VELOCITY_HPP
