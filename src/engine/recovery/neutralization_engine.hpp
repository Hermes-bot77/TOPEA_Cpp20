#ifndef TOPEA_ENGINE_RECOVERY_NEUTRALIZATION_ENGINE_HPP
#define TOPEA_ENGINE_RECOVERY_NEUTRALIZATION_ENGINE_HPP

#include "../../core/types.hpp"
#include "recovery_mission.hpp"
#include <cstdint>
#include <memory>

namespace topea::engine::recovery {

// ============================================================
// NEUTRALIZATION METRICS
// ============================================================

struct NeutralizationMetrics {
    double force = 0.0;                  // Recovery force needed
    double efficiency = 0.0;              // Expected efficiency (0.0-1.0)
    double cost = 0.0;                    // Estimated cost
    std::int64_t recommended_lots = 0;   // Recommended lot size
    double time_estimate = 0.0;          // Estimated time (seconds)
};

// ============================================================
// NEUTRALIZATION ENGINE
// ============================================================

/**
 * Calculates neutralization parameters.
 * Handles:
 * - Recovery force calculation
 * - Mathematical balance equations
 * - Efficiency optimization
 * - Cost estimation
 */
class NeutralizationEngine {
public:
    explicit NeutralizationEngine();

    /**
     * Calculate neutralization metrics
     */
    NeutralizationMetrics calculate(
        core::Lot buy_lots,
        core::Lot sell_lots,
        double buy_floating,
        double sell_floating,
        double recovery_strength);

    /**
     * Calculate force needed for neutralization
     */
    double calculateNeutralizationForce(
        core::Lot dominant_lots,
        double dominant_floating,
        double recovery_strength);

    /**
     * Calculate efficiency of recovery
     */
    double calculateEfficiency(
        core::Lot recovery_lots,
        double recovery_force,
        double target_force);

    /**
     * Calculate estimated time to neutralize
     */
    double estimateNeutralizationTime(
        double recovery_force,
        double volatility,
        double momentum);

    /**
     * Get recommended recovery mission based on conditions
     */
    RecoveryMissionType recommendMission(
        double imbalance_ratio,
        double pressure);

private:
    // Configuration
    static constexpr double MIN_FORCE = 0.1;
    static constexpr double MAX_FORCE = 10.0;
};

} // namespace topea::engine::recovery

#endif // TOPEA_ENGINE_RECOVERY_NEUTRALIZATION_ENGINE_HPP
