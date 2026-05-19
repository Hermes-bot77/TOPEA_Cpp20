#ifndef TOPEA_ENGINE_BASKET_COORDINATOR_HPP
#define TOPEA_ENGINE_BASKET_COORDINATOR_HPP

#include "basket_types.hpp"
#include "global_tp_manager.hpp"
#include "drawdown_manager.hpp"
#include "basket_pressure_analyzer.hpp"
#include "basket_state_machine.hpp"
#include <memory>
#include <shared_mutex>

namespace topea::engine::basket {

// ============================================================
// BASKET COORDINATOR
// ============================================================

/**
 * Orchestrates all basket components
 * Synchronizes TP management, drawdown protection, and pressure analysis
 * Maintains global basket health and stability
 */
class BasketCoordinator {
public:
    /**
     * Constructor
     * @param tp_config TP corridor configuration
     * @param initial_dd Initial drawdown threshold
     * @param critical_dd Critical drawdown threshold
     */
    explicit BasketCoordinator(
        const TPCorridorConfig& tp_config,
        double initial_dd = 5.0,
        double critical_dd = 15.0);

    /**
     * Initialize coordinator
     * @param entry_price Entry price
     * @param current_price Current price
     * @return true if successful
     */
    bool initialize(double entry_price, double current_price);

    /**
     * Update all components
     * @param current_price Current market price
     * @param highest_price Highest price so far
     * @param lowest_price Lowest price so far
     * @param volatility Current volatility
     * @param delta_time Time since last update
     */
    void update(double current_price, double highest_price, double lowest_price,
               double volatility, double delta_time);

    /**
     * Get current basket metrics
     * @param current_price Current price
     * @return Basket metrics
     */
    BasketMetrics getMetrics(double current_price) const;

    /**
     * Get current basket state
     * @return Basket state
     */
    BasketState getState() const {
        return state_machine_->getCurrentState();
    }

    /**
     * Check if state changed in last update
     * @return true if changed
     */
    bool hasStateChanged() const {
        return state_machine_->hasStateChanged();
    }

    /**
     * Get current take profit
     * @return TP price
     */
    double getCurrentTP() const {
        return tp_manager_->getCurrentTP();
    }

    /**
     * Get current drawdown
     * @return Drawdown percentage
     */
    double getCurrentDrawdown() const {
        return drawdown_manager_->getCurrentDrawdown();
    }

    /**
     * Get current pressure level
     * @return Pressure level (0.0-1.0)
     */
    double getPressureLevel() const {
        return pressure_analyzer_->getPressureLevel();
    }

    /**
     * Check if recovery should be activated
     * @return true if recovery needed
     */
    bool shouldActivateRecovery() const;

    /**
     * Check if recovery should be frozen
     * @return true if recovery should freeze
     */
    bool shouldFreezeRecovery() const;

    /**
     * Check if approaching TP
     * @param current_price Current price
     * @return true if near TP
     */
    bool isNearTP(double current_price) const;

    /**
     * Get recommended recovery acceleration
     * @return Acceleration factor (1.0 = normal)
     */
    double getRecoveryAccelerationFactor() const;

    /**
     * Reset coordinator to initial state
     */
    void reset();

    /**
     * Log basket statistics
     */
    void logStatistics(double current_price) const;

private:
    // Components
    std::unique_ptr<GlobalTPManager> tp_manager_;
    std::unique_ptr<DrawdownManager> drawdown_manager_;
    std::unique_ptr<BasketPressureAnalyzer> pressure_analyzer_;
    std::unique_ptr<BasketStateMachine> state_machine_;

    // Tracking
    double entry_price_;
    double highest_price_;
    double lowest_price_;
    mutable std::shared_mutex state_mutex_;

    // Configuration
    static constexpr double NEAR_TP_DISTANCE_RATIO = 0.05;
    static constexpr double CRITICAL_PRESSURE_THRESHOLD = 0.8;

    /**
     * Emit state change event
     */
    void emitStateChangeEvent(BasketState old_state, BasketState new_state) const;

    /**
     * Emit near TP event
     */
    void emitNearTPEvent() const;

    /**
     * Emit critical event
     */
    void emitCriticalEvent(BasketState state) const;
};

} // namespace topea::engine::basket

#endif // TOPEA_ENGINE_BASKET_COORDINATOR_HPP
