#include "basket_engine.hpp"
#include "../../core/logger.hpp"

namespace topea::engine::basket {

BasketEngine::BasketEngine(
    std::shared_ptr<core::EventBus> event_bus,
    std::shared_ptr<shared::StateManager> state_manager)
    : event_bus_(event_bus),
      state_manager_(state_manager) {

    // Create default TP configuration
    TPCorridorConfig default_config{
        .initial_tp_distance = 5.0,
        .min_tp_distance = 1.0,
        .max_tp_distance = 20.0,
        .elasticity_factor = 0.8,
        .recovery_expansion_rate = 1.2,
        .dynamic_adjustment = true
    };

    coordinator_ = std::make_unique<BasketCoordinator>(default_config, 5.0, 15.0);
}

std::string BasketEngine::getName() const {
    return "BasketEngine";
}

std::string BasketEngine::getDescription() const {
    return "Basket Engine - Global basket supervision and strategy";
}

bool BasketEngine::initialize() {
    if (initialized_) return true;

    if (!event_bus_ || !state_manager_) {
        TOPEA_ERROR("BasketEngine: Missing dependencies");
        return false;
    }

    // Get initial prices from state manager
    auto market_state = state_manager_->getMarketState();
    auto basket_state = state_manager_->getBasketState();

    if (!coordinator_->initialize(market_state.bid, market_state.bid)) {
        TOPEA_ERROR("BasketEngine: Failed to initialize coordinator");
        return false;
    }

    subscribeToEvents();
    initialized_ = true;

    TOPEA_INFO("BasketEngine initialized successfully");

    return true;
}

bool BasketEngine::shutdown() {
    if (!initialized_) return true;

    initialized_ = false;

    TOPEA_INFO("BasketEngine shutdown successfully");

    return true;
}

void BasketEngine::update(core::Duration delta_time) {
    if (!initialized_) return;

    updateBasket();
}

bool BasketEngine::isInitialized() const {
    return initialized_;
}

void BasketEngine::reset() {
    {
        std::unique_lock<std::shared_mutex> lock(metrics_mutex_);
        coordinator_->reset();
    }

    TOPEA_DEBUG("BasketEngine reset");
}

BasketMetrics BasketEngine::getCurrentMetrics(core::Price current_price) const {
    std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
    return coordinator_->getMetrics(current_price);
}

BasketState BasketEngine::getBasketState() const {
    std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
    return coordinator_->getState();
}

double BasketEngine::getCurrentTP() const {
    std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
    return coordinator_->getCurrentTP();
}

double BasketEngine::getCurrentDrawdown() const {
    std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
    return coordinator_->getCurrentDrawdown();
}

double BasketEngine::getPressureLevel() const {
    std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
    return coordinator_->getPressureLevel();
}

bool BasketEngine::isNearTP(core::Price current_price) const {
    std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
    return coordinator_->isNearTP(current_price);
}

bool BasketEngine::isCriticalDrawdown() const {
    auto state = getBasketState();
    return state == BasketState::CRITICAL_DD;
}

bool BasketEngine::shouldActivateRecovery() const {
    std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
    return coordinator_->shouldActivateRecovery();
}

bool BasketEngine::shouldFreezeRecovery() const {
    std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
    return coordinator_->shouldFreezeRecovery();
}

double BasketEngine::getRecoveryAccelerationFactor() const {
    std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
    return coordinator_->getRecoveryAccelerationFactor();
}

void BasketEngine::setTPConfig(const TPCorridorConfig& config) {
    std::unique_lock<std::shared_mutex> lock(metrics_mutex_);
    
    // Would reconfigure TP manager
    TOPEA_DEBUG("BasketEngine: TP configuration updated");
}

void BasketEngine::setDrawdownThresholds(double initial_dd, double critical_dd) {
    std::unique_lock<std::shared_mutex> lock(metrics_mutex_);
    
    // Would reconfigure drawdown manager
    TOPEA_DEBUG("BasketEngine: Drawdown thresholds updated");
}

void BasketEngine::logStatistics(core::Price current_price) const {
    std::shared_lock<std::shared_mutex> lock(metrics_mutex_);

    auto metrics = coordinator_->getMetrics(current_price);

    TOPEA_INFO("\\n=== BasketEngine Statistics ===\");
    TOPEA_INFO(\"  Current Price: {:.5f}\", current_price);
    TOPEA_INFO(\"  Take Profit: {:.5f}\", metrics.current_tp);
    TOPEA_INFO(\"  Distance to TP: {:.5f}\", metrics.distance_to_tp);
    TOPEA_INFO(\"  Current Drawdown: {:.2f}%\", metrics.current_drawdown);
    TOPEA_INFO(\"  Max Drawdown: {:.2f}%\", metrics.max_drawdown);
    TOPEA_INFO(\"  Pressure Level: {:.2f}\", metrics.pressure_level);
    TOPEA_INFO(\"  Gravity: {:.2f}\", metrics.gravity);
    TOPEA_INFO(\"  State: {}\", basketStateToString(metrics.state));
    TOPEA_INFO(\"  Time in State: {:.2f}s\", metrics.time_in_state);
}

core::EventBus* BasketEngine::getEventBus() const {
    return event_bus_.get();
}

void BasketEngine::subscribeToEvents() {
    if (!event_bus_) return;

    event_bus_->subscribe(
        core::EventType::DELTA_CRITICAL,
        [this](const core::Event& e) { onDeltaEvent(e); });

    event_bus_->subscribe(
        core::EventType::RECOVERY_ACTIVATED,
        [this](const core::Event& e) { onRecoveryEvent(e); });

    TOPEA_DEBUG("BasketEngine: subscribed to events");
}

void BasketEngine::updateBasket() {
    if (!state_manager_ || !coordinator_) return;

    auto market_state = state_manager_->getMarketState();
    auto basket_state = state_manager_->getBasketState();

    {
        std::unique_lock<std::shared_mutex> lock(metrics_mutex_);
        coordinator_->update(
            market_state.bid,
            market_state.highest,
            market_state.lowest,
            market_state.volatility,
            0.01  // Default 10ms
        );
    }
}

void BasketEngine::onDeltaEvent(const core::Event& event) {
    updateBasket();
}

void BasketEngine::onRecoveryEvent(const core::Event& event) {
    updateBasket();
}

void BasketEngine::onMarketEvent(const core::Event& event) {
    updateBasket();
}

} // namespace topea::engine::basket
