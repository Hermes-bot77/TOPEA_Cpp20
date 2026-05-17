#include "pending_engine.hpp"
#include "../../core/logger.hpp"

namespace topea::engine::pending {

// ============================================================
// CONSTRUCTOR
// ============================================================

PendingEngine::PendingEngine(
    std::shared_ptr<core::EventBus> event_bus,
    std::shared_ptr<shared::StateManager> state_mgr)
    : event_bus_(event_bus),
      state_manager_(state_mgr),
      coordinator_(std::make_shared<PendingCoordinator>(100, 5.0)),
      next_agent_id_(1000),
      initialized_(false),
      current_bid_(0.0),
      current_ask_(0.0),
      current_volatility_(0.0),
      current_momentum_(0.0) {
}

// ============================================================
// ENGINE MODULE INTERFACE
// ============================================================

std::string PendingEngine::getName() const {
    return "PendingEngine";
}

std::string PendingEngine::getDescription() const {
    return "Pending Elastic Swarm Engine - Manages living autonomous pending orders";
}

bool PendingEngine::initialize() {
    if (initialized_) return true;

    if (!coordinator_) {
        TOPEA_ERROR("PendingEngine: Coordinator not initialized");
        return false;
    }

    coordinator_->initialize();
    subscribeToEvents();

    initialized_ = true;

    TOPEA_INFO("PendingEngine initialized successfully");

    return true;
}

bool PendingEngine::shutdown() {
    if (!initialized_) return true;

    if (coordinator_) {
        coordinator_->shutdown();
    }

    initialized_ = false;

    TOPEA_INFO("PendingEngine shutdown successfully");

    return true;
}

void PendingEngine::update(core::Duration delta_time) {
    if (!initialized_) return;

    // Update from market state
    updateFromMarketState();

    // Update all agents
    if (coordinator_) {
        coordinator_->updateAll(delta_time);
    }

    // Update behaviors based on market conditions
    updateAgentBehaviors();

    // Price pursuit
    updatePursuit();
}

bool PendingEngine::isInitialized() const {
    return initialized_;
}

void PendingEngine::reset() {
    if (coordinator_) {
        coordinator_->shutdown();
        coordinator_->initialize();
    }

    next_agent_id_ = 1000;

    TOPEA_INFO("PendingEngine reset");
}

// ============================================================
// PENDING ENGINE INTERFACE
// ============================================================

std::shared_ptr<PendingAgent> PendingEngine::createPendingAgent(
    const std::string& symbol,
    core::OrderType direction,
    core::Lot lot_size,
    MissionType mission) {

    if (!initialized_) {
        TOPEA_ERROR("PendingEngine not initialized");
        return nullptr;
    }

    auto agent = PendingAgentFactory::createAgent(
        next_agent_id_++, symbol, direction, lot_size, mission);

    if (coordinator_->addAgent(agent)) {
        TOPEA_DEBUG("Created pending agent: ID={}, Mission={}",
                    agent->getUniqueId(),
                    PendingMission::missionTypeToString(mission));
        return agent;
    }

    TOPEA_WARNING("Failed to add agent to coordinator");
    return nullptr;
}

std::shared_ptr<PendingAgent> PendingEngine::createRecoveryAgent(
    const std::string& symbol,
    core::OrderType recovery_direction,
    core::Lot recovery_lot) {

    if (!initialized_) {
        TOPEA_ERROR("PendingEngine not initialized");
        return nullptr;
    }

    auto agent = PendingAgentFactory::createRecoveryAgent(
        next_agent_id_++, symbol, recovery_direction, recovery_lot);

    if (coordinator_->addAgent(agent)) {
        TOPEA_DEBUG("Created recovery agent: ID={}, Direction={}",
                    agent->getUniqueId(),
                    recovery_direction == core::OrderType::BUY ? "BUY"
                                                               : "SELL");
        return agent;
    }

    return nullptr;
}

std::shared_ptr<PendingAgent> PendingEngine::createDefenseAgent(
    const std::string& symbol,
    core::OrderType direction,
    core::Lot lot_size) {

    if (!initialized_) {
        TOPEA_ERROR("PendingEngine not initialized");
        return nullptr;
    }

    auto agent = PendingAgentFactory::createDefenseAgent(
        next_agent_id_++, symbol, direction, lot_size);

    if (coordinator_->addAgent(agent)) {
        TOPEA_DEBUG("Created defense agent: ID={}", agent->getUniqueId());
        return agent;
    }

    return nullptr;
}

bool PendingEngine::removePendingAgent(std::uint64_t agent_id) {
    return coordinator_->removeAgent(agent_id);
}

std::shared_ptr<PendingAgent> PendingEngine::getPendingAgent(
    std::uint64_t agent_id) {
    return coordinator_->getAgent(agent_id);
}

std::vector<std::shared_ptr<PendingAgent>> PendingEngine::getActiveAgents() {
    return coordinator_->getActiveAgents();
}

// ============================================================
// STATISTICS
// ============================================================

void PendingEngine::logStatistics() const {
    if (coordinator_) {
        coordinator_->logStatistics();
    }
}

// ============================================================
// INTERNAL METHODS
// ============================================================

core::EventBus* PendingEngine::getEventBus() const {
    return event_bus_.get();
}

void PendingEngine::subscribeToEvents() {
    if (!event_bus_) return;

    // Subscribe to events
    event_bus_->subscribe(
        core::EventType::BASKET_NEAR_TP,
        [this](const core::Event& e) { onBasketNearTP(e); });

    event_bus_->subscribe(
        core::EventType::BASKET_FAR_TP,
        [this](const core::Event& e) { onBasketFarTP(e); });

    event_bus_->subscribe(
        core::EventType::DELTA_CRITICAL,
        [this](const core::Event& e) { onDeltaCritical(e); });

    event_bus_->subscribe(
        core::EventType::RECOVERY_ACTIVATED,
        [this](const core::Event& e) { onRecoveryActivated(e); });

    event_bus_->subscribe(
        core::EventType::RECOVERY_STOPPED,
        [this](const core::Event& e) { onRecoveryStopped(e); });

    event_bus_->subscribe(
        core::EventType::VOLATILITY_HIGH,
        [this](const core::Event& e) { onVolatilityChange(e); });

    event_bus_->subscribe(
        core::EventType::MOMENTUM_BULLISH,
        [this](const core::Event& e) { onMomentumChange(e); });

    event_bus_->subscribe(
        core::EventType::MOMENTUM_BEARISH,
        [this](const core::Event& e) { onMomentumChange(e); });

    TOPEA_DEBUG("PendingEngine: subscribed to events");
}

void PendingEngine::updateFromMarketState() {
    if (!state_manager_) return;

    auto market_state = state_manager_->getMarketState();
    current_bid_ = market_state.bid;
    current_ask_ = market_state.ask;
    current_volatility_ = market_state.volatility;
    current_momentum_ = market_state.momentum;
}

void PendingEngine::updateAgentBehaviors() {
    if (!coordinator_) return;

    auto basket_state = state_manager_->getBasketState();
    auto recovery_state = state_manager_->getRecoveryState();

    // Adapt distance based on market conditions
    coordinator_->onVolatilityChange(current_volatility_);
    coordinator_->onMomentumChange(current_momentum_);

    // Check delta criticality
    double delta_pressure =
        (double)basket_state.delta_lots / std::max(1.0, (double)(
            basket_state.total_buy_lots + basket_state.total_sell_lots));

    if (std::abs(delta_pressure) > 0.8) {
        coordinator_->onDeltaCritical(basket_state.delta_lots, delta_pressure);
    }
}

void PendingEngine::updatePursuit() {
    auto agents = coordinator_->getActiveAgents();

    for (auto& agent : agents) {
        if (agent && agent->isPursuitMode()) {
            agent->followPrice(current_bid_, current_momentum_);
        }
    }
}

// ============================================================
// EVENT HANDLERS
// ============================================================

void PendingEngine::onBasketNearTP(const core::Event& event) {
    if (coordinator_) {
        coordinator_->onBasketNearTP();
    }
}

void PendingEngine::onBasketFarTP(const core::Event& event) {
    if (coordinator_) {
        coordinator_->onBasketFarTP();
    }
}

void PendingEngine::onDeltaCritical(const core::Event& event) {
    // Delta criticality is handled in updateAgentBehaviors
}

void PendingEngine::onRecoveryActivated(const core::Event& event) {
    TOPEA_DEBUG("PendingEngine: Recovery activated event received");
    // Handled dynamically in update
}

void PendingEngine::onRecoveryStopped(const core::Event& event) {
    if (coordinator_) {
        coordinator_->deactivateRecovery();
    }
}

void PendingEngine::onVolatilityChange(const core::Event& event) {
    // Handled in updateAgentBehaviors
}

void PendingEngine::onMomentumChange(const core::Event& event) {
    // Handled in updateAgentBehaviors
}

} // namespace topea::engine::pending
