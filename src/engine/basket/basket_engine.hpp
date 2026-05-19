#ifndef TOPEA_ENGINE_BASKET_ENGINE_HPP
#define TOPEA_ENGINE_BASKET_ENGINE_HPP

#include "basket_coordinator.hpp"
#include "../../core/engine_module.hpp"
#include "../../shared/state_manager.hpp"
#include <memory>
#include <shared_mutex>

namespace topea::engine::basket {

// ============================================================
// BASKET ENGINE
// ============================================================

/**
 * Main Basket Engine module
 * Supervises the entire basket health, profit, and survival
 */
class BasketEngine : public core::IBasketEngine {
public:
    explicit BasketEngine(
        std::shared_ptr<core::EventBus> event_bus,
        std::shared_ptr<shared::StateManager> state_manager);\

    ~BasketEngine() override = default;

    // ========================================================
    // ENGINE MODULE INTERFACE
    // ========================================================

    std::string getName() const override;
    std::string getDescription() const override;
    bool initialize() override;
    bool shutdown() override;
    void update(core::Duration delta_time) override;
    bool isInitialized() const override;
    void reset() override;

    // ========================================================
    // BASKET ENGINE INTERFACE
    // ========================================================

    /**
     * Get current basket metrics
     * @param current_price Current market price
     * @return Basket metrics
     */
    BasketMetrics getCurrentMetrics(core::Price current_price) const;

    /**
     * Get basket state
     * @return Current state
     */
    BasketState getBasketState() const;

    /**
     * Get current take profit
     * @return TP price
     */
    double getCurrentTP() const;

    /**
     * Get current drawdown
     * @return Drawdown percentage
     */
    double getCurrentDrawdown() const;

    /**
     * Get pressure level
     * @return Pressure (0.0-1.0)
     */
    double getPressureLevel() const;

    /**
     * Check if near take profit
     * @param current_price Current price
     * @return true if near TP
     */
    bool isNearTP(core::Price current_price) const;

    /**
     * Check if critical drawdown
     * @return true if critical
     */
    bool isCriticalDrawdown() const;

    /**
     * Should recovery be activated
     * @return true if recovery needed
     */
    bool shouldActivateRecovery() const;

    /**
     * Should recovery be frozen
     * @return true if recovery should freeze
     */
    bool shouldFreezeRecovery() const;

    /**
     * Get recovery acceleration factor
     * @return Acceleration factor (1.0 = normal)
     */
    double getRecoveryAccelerationFactor() const;

    /**
     * Configure TP corridor
     * @param config TP configuration
     */
    void setTPConfig(const TPCorridorConfig& config);

    /**
     * Configure drawdown thresholds
     * @param initial_dd Initial drawdown
     * @param critical_dd Critical drawdown
     */
    void setDrawdownThresholds(double initial_dd, double critical_dd);

    /**
     * Log statistics
     */
    void logStatistics(core::Price current_price) const;

private:
    // Dependencies
    std::shared_ptr<core::EventBus> event_bus_;
    std::shared_ptr<shared::StateManager> state_manager_;

    // Components
    std::unique_ptr<BasketCoordinator> coordinator_;

    // State
    mutable std::shared_mutex metrics_mutex_;
    bool initialized_ = false;

    // Internal methods
    core::EventBus* getEventBus() const override;
    void subscribeToEvents();
    void updateBasket();

    // Event handlers
    void onDeltaEvent(const core::Event& event);
    void onRecoveryEvent(const core::Event& event);
    void onMarketEvent(const core::Event& event);
};

} // namespace topea::engine::basket

#endif // TOPEA_ENGINE_BASKET_ENGINE_HPP
