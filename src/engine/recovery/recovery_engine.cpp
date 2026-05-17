#include "recovery_engine.hpp"
#include "../../core/logger.hpp"

namespace topea::engine::recovery {

RecoveryEngine::RecoveryEngine(
    std::shared_ptr<core::EventBus> event_bus,
    std::shared_ptr<shared::StateManager> state_manager)
    : event_bus_(event_bus),
      state_manager_(state_manager) {

    coordinator_ = std::make_unique<RecoveryCoordinator>(500, 5.0);
    neutralizer_ = std::make_unique<NeutralizationEngine>();
}

std::string RecoveryEngine::getName() const {
    return "RecoveryEngine";
}

std::string RecoveryEngine::getDescription() const {
    return "Recovery Engine - Defensive neutralization and survival";
}

bool RecoveryEngine::initialize() {
    if (initialized_) return true;

    if (!event_bus_ || !state_manager_) {
        TOPEA_ERROR("RecoveryEngine: Missing dependencies");
        return false;
    }

    if (coordinator_) {
        coordinator_->initialize();
    }

    subscribeToEvents();
    initialized_ = true;

    TOPEA_INFO("RecoveryEngine initialized successfully");

    return true;
}

bool RecoveryEngine::shutdown() {
    if (!initialized_) return true;

    if (coordinator_) {
        coordinator_->shutdown();
    }

    initialized_ = false;

    TOPEA_INFO("RecoveryEngine shutdown successfully");

    return true;
}

void RecoveryEngine::update(core::Duration delta_time) {
    if (!initialized_) return;

    updateRecoveries();
}

bool RecoveryEngine::isInitialized() const {
    return initialized_;
}

void RecoveryEngine::reset() {
    if (coordinator_) {
        coordinator_->shutdown();
        coordinator_->initialize();
    }

    next_agent_id_ = 2000;

    TOPEA_INFO("RecoveryEngine reset");
}

std::shared_ptr<RecoveryAgent> RecoveryEngine::createRecoveryAgent(
    const std::string& symbol,
    core::OrderType direction,
    core::Lot lot_size,
    RecoveryMissionType mission) {

    if (!initialized_) {
        TOPEA_ERROR("RecoveryEngine not initialized");
        return nullptr;
    }

    auto agent = RecoveryAgentFactory::createAgent(
        next_agent_id_++, symbol, direction, lot_size, mission);

    if (coordinator_->addAgent(agent)) {
        TOPEA_DEBUG("Created recovery agent: ID={}, Mission={}",
                    agent->getUniqueId(),
                    recoveryMissionTypeToString(mission));
        return agent;
    }

    return nullptr;
}

std::shared_ptr<RecoveryAgent> RecoveryEngine::createAggressiveRecovery(
    const std::string& symbol,
    core::OrderType direction,
    core::Lot lot_size) {

    return createRecoveryAgent(
        symbol, direction, lot_size, RecoveryMissionType::AGGRESSIVE_RECOVERY);
}

std::shared_ptr<RecoveryAgent> RecoveryEngine::createSoftRecovery(
    const std::string& symbol,
    core::OrderType direction,
    core::Lot lot_size) {

    return createRecoveryAgent(
        symbol, direction, lot_size, RecoveryMissionType::SOFT_RECOVERY);
}

bool RecoveryEngine::removeRecoveryAgent(std::uint64_t agent_id) {
    return coordinator_->removeAgent(agent_id);
}

std::shared_ptr<RecoveryAgent> RecoveryEngine::getRecoveryAgent(
    std::uint64_t agent_id) {

    return coordinator_->getAgent(agent_id);
}

std::vector<std::shared_ptr<RecoveryAgent>> RecoveryEngine::getActiveRecoveries() {
    return coordinator_->getActiveAgents();
}

NeutralizationMetrics RecoveryEngine::calculateNeutralization(
    core::Lot buy_lots,
    core::Lot sell_lots,
    double buy_floating,
    double sell_floating) {

    if (!neutralizer_) {
        return NeutralizationMetrics();
    }

    return neutralizer_->calculate(
        buy_lots, sell_lots, buy_floating, sell_floating, 1.0);
}

void RecoveryEngine::freezeRecoveriesByDirection(core::OrderType direction) {
    if (coordinator_) {
        coordinator_->freezeByDirection(direction);
    }
}

void RecoveryEngine::accelerateRecoveriesByMission(
    RecoveryMissionType mission,
    double factor) {

    if (coordinator_) {
        coordinator_->accelerateByMission(mission, factor);
    }
}

void RecoveryEngine::recycleRecoveries(
    RecoveryMissionType from_mission,
    RecoveryMissionType to_mission) {

    if (coordinator_) {
        coordinator_->recycleByMission(from_mission, to_mission);
    }
}

RecoveryStatistics RecoveryEngine::getStatistics() const {
    if (coordinator_) {
        return coordinator_->getStatistics();
    }
    return RecoveryStatistics();
}

void RecoveryEngine::logStatistics() const {
    if (coordinator_) {
        coordinator_->logStatistics();
    }
}

core::EventBus* RecoveryEngine::getEventBus() const {
    return event_bus_.get();
}

void RecoveryEngine::subscribeToEvents() {
    if (!event_bus_) return;

    event_bus_->subscribe(
        core::EventType::BASKET_NEAR_TP,
        [this](const core::Event& e) { onBasketNearTP(e); });

    event_bus_->subscribe(
        core::EventType::DELTA_CRITICAL,
        [this](const core::Event& e) { onDeltaCritical(e); });

    event_bus_->subscribe(
        core::EventType::RECOVERY_ACTIVATED,
        [this](const core::Event& e) { onRecoveryActivated(e); });

    TOPEA_DEBUG("RecoveryEngine: subscribed to events");
}

void RecoveryEngine::updateRecoveries() {
    if (coordinator_) {
        coordinator_->updateAll(10);  // 10ms default
    }
}

void RecoveryEngine::onBasketNearTP(const core::Event& event) {
    if (coordinator_) {
        coordinator_->onBasketNearTP();
    }
}

void RecoveryEngine::onDeltaCritical(const core::Event& event) {
    if (coordinator_) {
        coordinator_->onDeltaCritical(0.5);  // Dummy pressure
    }
}

void RecoveryEngine::onRecoveryActivated(const core::Event& event) {
    if (coordinator_) {
        coordinator_->onRecoveryActivated();
    }
}

} // namespace topea::engine::recovery
