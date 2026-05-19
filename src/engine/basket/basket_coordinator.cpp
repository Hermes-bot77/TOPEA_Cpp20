#include "basket_coordinator.hpp"
#include <algorithm>

namespace topea::engine::basket {

BasketCoordinator::BasketCoordinator(
    const TPCorridorConfig& tp_config,
    double initial_dd,
    double critical_dd)
    : entry_price_(0.0),
      highest_price_(0.0),
      lowest_price_(0.0) {
    
    tp_manager_ = std::make_unique<GlobalTPManager>(tp_config);
    drawdown_manager_ = std::make_unique<DrawdownManager>(initial_dd, critical_dd);
    pressure_analyzer_ = std::make_unique<BasketPressureAnalyzer>();
    state_machine_ = std::make_unique<BasketStateMachine>();
}

bool BasketCoordinator::initialize(double entry_price, double current_price) {
    if (entry_price <= 0.0 || current_price <= 0.0) {
        return false;
    }

    entry_price_ = entry_price;
    highest_price_ = current_price;
    lowest_price_ = current_price;

    if (!tp_manager_->initialize(entry_price, current_price)) {
        return false;
    }

    if (!drawdown_manager_->initialize(entry_price, highest_price_)) {
        return false;
    }

    if (!pressure_analyzer_->initialize(entry_price, highest_price_)) {
        return false;
    }

    if (!state_machine_->initialize()) {
        return false;
    }

    return true;
}

void BasketCoordinator::update(double current_price, double highest_price,
                              double lowest_price, double volatility,
                              double delta_time) {
    std::unique_lock<std::shared_mutex> lock(state_mutex_);

    // Update tracking
    highest_price_ = std::max(highest_price_, highest_price);
    lowest_price_ = std::min(lowest_price_, lowest_price);

    // Update drawdown
    drawdown_manager_->update(current_price, delta_time);

    // Update pressure analyzer
    double current_dd = drawdown_manager_->getCurrentDrawdown();
    pressure_analyzer_->update(current_price, highest_price_, lowest_price_,
                              current_dd, volatility, delta_time);

    // Update TP manager
    double pressure_level = pressure_analyzer_->getPressureLevel();
    BasketState inferred_state = pressure_analyzer_->getInferredState();
    tp_manager_->update(current_price, pressure_level, inferred_state, delta_time);

    // Update state machine
    double distance_to_tp = tp_manager_->getDistanceToTP(current_price);
    bool is_accelerating = drawdown_manager_->isAccelerating();
    state_machine_->update(pressure_level, current_dd, is_accelerating,
                          distance_to_tp, delta_time);

    // Emit events on state change
    if (state_machine_->hasStateChanged()) {
        BasketState old_state = state_machine_->getPreviousState();
        BasketState new_state = state_machine_->getCurrentState();
        emitStateChangeEvent(old_state, new_state);
    }

    // Check for special conditions
    if (isNearTP(current_price)) {
        emitNearTPEvent();
    }

    if (state_machine_->getCurrentState() == BasketState::CRITICAL_DD) {
        emitCriticalEvent(BasketState::CRITICAL_DD);
    }
}

BasketMetrics BasketCoordinator::getMetrics(double current_price) const {
    std::shared_lock<std::shared_mutex> lock(state_mutex_);

    BasketMetrics metrics;
    
    metrics.entry_price = entry_price_;
    metrics.current_price = current_price;
    metrics.current_tp = tp_manager_->getCurrentTP();
    metrics.highest_price = highest_price_;
    metrics.lowest_price = lowest_price_;
    
    metrics.current_drawdown = drawdown_manager_->getCurrentDrawdown();
    metrics.max_drawdown = drawdown_manager_->getMaxDrawdown();
    metrics.max_drawdown_pct = metrics.max_drawdown;
    
    metrics.volatility = pressure_analyzer_->getVolatilityAdjustedPressure();
    metrics.gravity = pressure_analyzer_->getGravity();
    metrics.pressure_level = pressure_analyzer_->getPressureLevel();
    metrics.acceleration = drawdown_manager_->getAccelerationRate();
    
    metrics.state = state_machine_->getCurrentState();
    metrics.cycles_in_state = state_machine_->getCyclesInState();
    metrics.time_in_state = state_machine_->getTimeInState();
    
    metrics.distance_to_tp = tp_manager_->getDistanceToTP(current_price);
    metrics.price_range = highest_price_ - lowest_price_;
    
    return metrics;
}

bool BasketCoordinator::shouldActivateRecovery() const {
    BasketState state = state_machine_->getCurrentState();
    return state == BasketState::PRESSURED ||
           state == BasketState::CRITICAL_DD ||
           state == BasketState::RECOVERY_HEAVY;
}

bool BasketCoordinator::shouldFreezeRecovery() const {
    BasketState state = state_machine_->getCurrentState();
    double pressure = pressure_analyzer_->getPressureLevel();
    
    return (state == BasketState::NEAR_TP && pressure < 0.3) ||
           (state == BasketState::BALANCED && pressure < 0.2);
}

bool BasketCoordinator::isNearTP(double current_price) const {
    double distance_to_tp = tp_manager_->getDistanceToTP(current_price);
    double tp_range = entry_price_ * (NEAR_TP_DISTANCE_RATIO);
    
    return distance_to_tp < tp_range;
}

double BasketCoordinator::getRecoveryAccelerationFactor() const {
    BasketState state = state_machine_->getCurrentState();
    double pressure = pressure_analyzer_->getPressureLevel();
    
    switch (state) {
        case BasketState::CRITICAL_DD:
            return 2.0 + (pressure * 1.0);  // Up to 3x
        case BasketState::RECOVERY_HEAVY:
            return 1.5 + (pressure * 0.5);  // Up to 2x
        case BasketState::PRESSURED:
            return 1.2 + (pressure * 0.3);  // Up to 1.5x
        case BasketState::NEAR_TP:
            return 0.7;  // Slow down
        case BasketState::BALANCED:
            return 1.0;  // Normal
        default:
            return 1.0;
    }
}

void BasketCoordinator::reset() {
    std::unique_lock<std::shared_mutex> lock(state_mutex_);
    
    entry_price_ = 0.0;
    highest_price_ = 0.0;
    lowest_price_ = 0.0;
    
    tp_manager_->reset();
    drawdown_manager_->reset();
    pressure_analyzer_->reset();
    state_machine_->reset();
}

void BasketCoordinator::logStatistics(double current_price) const {
    std::shared_lock<std::shared_mutex> lock(state_mutex_);
    
    auto metrics = getMetrics(current_price);
    
    // Logging would be done through event system or logger
    // This is a placeholder for actual logging implementation
}

void BasketCoordinator::emitStateChangeEvent(BasketState old_state,
                                            BasketState new_state) const {
    // Would emit through event bus
    // Placeholder for event emission
}

void BasketCoordinator::emitNearTPEvent() const {
    // Would emit through event bus
    // Placeholder for event emission
}

void BasketCoordinator::emitCriticalEvent(BasketState state) const {
    // Would emit through event bus
    // Placeholder for event emission
}

} // namespace topea::engine::basket
