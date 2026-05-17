#include "recovery_agent.hpp"
#include "../../core/logger.hpp"

namespace topea::engine::recovery {

RecoveryAgent::RecoveryAgent(
    std::uint64_t agent_id,
    const std::string& symbol,
    core::OrderType direction,
    core::Lot lot_size,
    RecoveryMissionType mission)
    : unique_id_(agent_id),
      symbol_(symbol),
      direction_(direction),
      lot_size_(lot_size),
      current_mission_(mission),
      mission_profile_(RecoveryMissionProfile::getProfileFor(mission)),
      creation_time_(std::chrono::high_resolution_clock::now()) {

    state_machine_ = std::make_unique<RecoveryStateMachine>();
    velocity_engine_ = std::make_unique<RecoveryVelocity>(
        mission_profile_.velocity_factor, 0.95);

    aggression_ = mission_profile_.aggression;
    strength_ = mission_profile_.strength;
}

std::uint64_t RecoveryAgent::getUniqueId() const {
    return unique_id_;
}

std::string RecoveryAgent::getSymbol() const {
    return symbol_;
}

core::OrderType RecoveryAgent::getDirection() const {
    return direction_;
}

core::Lot RecoveryAgent::getLotSize() const {
    return lot_size_;
}

RecoveryMissionType RecoveryAgent::getMissionType() const {
    return current_mission_;
}

const RecoveryMissionProfile& RecoveryAgent::getMissionProfile() const {
    return mission_profile_;
}

void RecoveryAgent::setMission(RecoveryMissionType mission) {
    current_mission_ = mission;
    mission_profile_ = RecoveryMissionProfile::getProfileFor(mission);
    aggression_ = mission_profile_.aggression;
    strength_ = mission_profile_.strength;
    TOPEA_DEBUG("RecoveryAgent {}: Mission changed to {}",
                unique_id_,
                recoveryMissionTypeToString(mission));
}

void RecoveryAgent::recycleMission(RecoveryMissionType new_mission) {
    TOPEA_DEBUG("RecoveryAgent {}: Recycled from {} to {}",
                unique_id_,
                recoveryMissionTypeToString(current_mission_),
                recoveryMissionTypeToString(new_mission));
    setMission(new_mission);
}

RecoveryState RecoveryAgent::getState() const {
    return state_machine_->getCurrentState();
}

bool RecoveryAgent::isActive() const {
    return state_machine_->isActive();
}

bool RecoveryAgent::isDefensive() const {
    return state_machine_->isDefensive();
}

bool RecoveryAgent::isAggressive() const {
    return state_machine_->isAggressive();
}

bool RecoveryAgent::isFrozen() const {
    return state_machine_->isFrozen();
}

void RecoveryAgent::freeze() {
    state_machine_->freeze();
    velocity_engine_->freeze();
}

void RecoveryAgent::unfreeze() {
    state_machine_->unfreeze();
    velocity_engine_->unfreeze();
}

core::Price RecoveryAgent::getEntryPrice() const {
    return entry_price_;
}

void RecoveryAgent::setEntryPrice(core::Price price) {
    entry_price_ = price;
}

core::Price RecoveryAgent::getCurrentDistance() const {
    return current_distance_;
}

void RecoveryAgent::updateDistance(core::Price current_price) {
    current_distance_ = std::abs(current_price - entry_price_);
}

core::Price RecoveryAgent::getTargetZone() const {
    return target_zone_;
}

void RecoveryAgent::setTargetZone(core::Price target) {
    target_zone_ = target;
}

core::Price RecoveryAgent::getVelocity() const {
    return velocity_engine_->getVelocity();
}

void RecoveryAgent::accelerate(double factor) {
    velocity_engine_->accelerate(factor);
}

void RecoveryAgent::decelerate(double factor) {
    velocity_engine_->decelerate(factor);
}

void RecoveryAgent::resetVelocity() {
    velocity_engine_->resetToBase();
}

void RecoveryAgent::updateVelocity(
    core::Duration delta_time,
    double momentum,
    double volatility,
    double pressure) {

    velocity_engine_->update(delta_time, momentum, volatility, pressure);
}

double RecoveryAgent::getStrength() const {
    return strength_;
}

void RecoveryAgent::setStrength(double strength) {
    strength_ = std::max(0.1, std::min(5.0, strength));
}

double RecoveryAgent::getAggressionLevel() const {
    return aggression_;
}

void RecoveryAgent::setAggressionLevel(double level) {
    aggression_ = std::max(0.0, std::min(1.0, level));
}

bool RecoveryAgent::transitionState(RecoveryState new_state) {
    return state_machine_->transitionTo(new_state);
}

void RecoveryAgent::update(core::Duration delta_time) {
    if (!isActive()) return;
    // Update velocity based on market conditions
    // This will be called by RecoveryCoordinator
}

void RecoveryAgent::reset() {
    state_machine_->reset();
    velocity_engine_->resetToBase();
    entry_price_ = 0.0;
    current_distance_ = 0.0;
    target_zone_ = 0.0;
}

std::int64_t RecoveryAgent::getTimeActive() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - creation_time_);
    return duration.count();
}

// ============================================================
// FACTORY IMPLEMENTATION
// ============================================================

std::shared_ptr<RecoveryAgent> RecoveryAgentFactory::createAgent(
    std::uint64_t agent_id,
    const std::string& symbol,
    core::OrderType direction,
    core::Lot lot_size,
    RecoveryMissionType mission) {

    return std::make_shared<RecoveryAgent>(
        agent_id, symbol, direction, lot_size, mission);
}

std::shared_ptr<RecoveryAgent> RecoveryAgentFactory::createAggressive(
    std::uint64_t agent_id,
    const std::string& symbol,
    core::OrderType direction,
    core::Lot lot_size) {

    return createAgent(agent_id, symbol, direction, lot_size,
                       RecoveryMissionType::AGGRESSIVE_RECOVERY);
}

std::shared_ptr<RecoveryAgent> RecoveryAgentFactory::createSoft(
    std::uint64_t agent_id,
    const std::string& symbol,
    core::OrderType direction,
    core::Lot lot_size) {

    return createAgent(agent_id, symbol, direction, lot_size,
                       RecoveryMissionType::SOFT_RECOVERY);
}

std::shared_ptr<RecoveryAgent> RecoveryAgentFactory::createDefensive(
    std::uint64_t agent_id,
    const std::string& symbol,
    core::OrderType direction,
    core::Lot lot_size) {

    return createAgent(agent_id, symbol, direction, lot_size,
                       RecoveryMissionType::DEFENSIVE_RECOVERY);
}

} // namespace topea::engine::recovery
