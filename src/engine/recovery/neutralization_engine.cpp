#include "neutralization_engine.hpp"
#include "../../core/logger.hpp"
#include <algorithm>
#include <cmath>

namespace topea::engine::recovery {

NeutralizationEngine::NeutralizationEngine() {
}

NeutralizationMetrics NeutralizationEngine::calculate(
    core::Lot buy_lots,
    core::Lot sell_lots,
    double buy_floating,
    double sell_floating,
    double recovery_strength) {

    NeutralizationMetrics metrics;

    // Determine dominant side
    core::Lot dominant_lots = std::max(buy_lots, sell_lots);
    core::Lot weaker_lots = std::min(buy_lots, sell_lots);
    double delta = dominant_lots - weaker_lots;

    // Floating for dominant side
    double dominant_floating = (buy_lots > sell_lots) ? buy_floating : sell_floating;

    // Calculate required force
    metrics.force = calculateNeutralizationForce(
        dominant_lots, dominant_floating, recovery_strength);

    // Calculate recommended lots
    metrics.recommended_lots =
        static_cast<core::Lot>(delta * (metrics.force / 2.0));
    metrics.recommended_lots =
        std::max(1LL, metrics.recommended_lots);

    // Calculate efficiency
    double target_force = std::abs(dominant_floating) / std::max(1.0, (double)dominant_lots);
    metrics.efficiency = calculateEfficiency(
        metrics.recommended_lots, metrics.force, target_force);

    // Calculate cost (simplified)
    metrics.cost = metrics.force * 0.1;  // 10% of force as cost

    // Estimate time
    metrics.time_estimate = estimateNeutralizationTime(
        metrics.force, 0.5, 0.0);  // Dummy volatility/momentum

    return metrics;
}

double NeutralizationEngine::calculateNeutralizationForce(
    core::Lot dominant_lots,
    double dominant_floating,
    double recovery_strength) {

    if (dominant_lots == 0) return MIN_FORCE;

    // Force based on floating loss and lot size
    double loss_factor = std::abs(dominant_floating) / std::max(1.0, (double)dominant_lots);

    // Apply recovery strength multiplier
    double force = (loss_factor * 0.5 + 1.0) * recovery_strength;

    return std::max(MIN_FORCE, std::min(MAX_FORCE, force));
}

double NeutralizationEngine::calculateEfficiency(
    core::Lot recovery_lots,
    double recovery_force,
    double target_force) {

    if (target_force == 0.0) return 1.0;

    double ratio = std::min(recovery_force / target_force, 2.0);
    return std::min(1.0, ratio);
}

double NeutralizationEngine::estimateNeutralizationTime(
    double recovery_force,
    double volatility,
    double momentum) {

    // Time inversely proportional to force
    double base_time = 60.0 / (recovery_force + 0.1);  // seconds

    // Adjust for volatility
    double volatility_factor = 1.0 + volatility * 0.5;

    // Adjust for momentum
    double momentum_factor = 1.0 - (momentum * 0.3);

    return base_time * volatility_factor * momentum_factor;
}

RecoveryMissionType NeutralizationEngine::recommendMission(
    double imbalance_ratio,
    double pressure) {

    // imbalance_ratio: 0.0 to 1.0 (1.0 = complete imbalance)
    // pressure: 0.0 to 1.0 (1.0 = critical)

    if (pressure > 0.8) {
        return RecoveryMissionType::DEFENSIVE_RECOVERY;
    }

    if (imbalance_ratio > 0.7 && pressure > 0.5) {
        return RecoveryMissionType::AGGRESSIVE_RECOVERY;
    }

    if (imbalance_ratio > 0.5) {
        return RecoveryMissionType::COUNTER_RECOVERY;
    }

    if (imbalance_ratio > 0.2) {
        return RecoveryMissionType::NEUTRALIZATION;
    }

    return RecoveryMissionType::SOFT_RECOVERY;
}

} // namespace topea::engine::recovery
