#include "recovery_coordinator.hpp"
#include "../../core/logger.hpp"
#include <algorithm>

namespace topea::engine::recovery {

RecoveryCoordinator::RecoveryCoordinator(
    std::int64_t max_agents,
    double min_distance)
    : max_agents_(max_agents),
      min_distance_(min_distance) {
}

void RecoveryCoordinator::initialize() {
    initialized_ = true;
    TOPEA_DEBUG("RecoveryCoordinator initialized");
}

void RecoveryCoordinator::shutdown() {
    {
        std::unique_lock<std::shared_mutex> lock(agents_mutex_);
        agents_.clear();
    }
    initialized_ = false;
    TOPEA_DEBUG("RecoveryCoordinator shutdown");
}

bool RecoveryCoordinator::addAgent(std::shared_ptr<RecoveryAgent> agent) {
    if (!agent || !initialized_) return false;

    std::unique_lock<std::shared_mutex> lock(agents_mutex_);

    if (agents_.size() >= static_cast<std::size_t>(max_agents_)) {
        TOPEA_WARNING("RecoveryCoordinator: Max agents reached");
        return false;
    }

    agents_.push_back(agent);
    TOPEA_DEBUG("RecoveryAgent added: ID={}", agent->getUniqueId());
    return true;
}

bool RecoveryCoordinator::removeAgent(std::uint64_t agent_id) {
    std::unique_lock<std::shared_mutex> lock(agents_mutex_);

    auto it = std::find_if(
        agents_.begin(), agents_.end(),
        [agent_id](const auto& a) { return a->getUniqueId() == agent_id; });

    if (it != agents_.end()) {
        agents_.erase(it);
        TOPEA_DEBUG("RecoveryAgent removed: ID={}", agent_id);
        return true;
    }

    return false;
}

std::shared_ptr<RecoveryAgent> RecoveryCoordinator::getAgent(
    std::uint64_t agent_id) {

    std::shared_lock<std::shared_mutex> lock(agents_mutex_);

    auto it = std::find_if(
        agents_.begin(), agents_.end(),
        [agent_id](const auto& a) { return a->getUniqueId() == agent_id; });

    if (it != agents_.end()) {
        return *it;
    }

    return nullptr;
}

std::vector<std::shared_ptr<RecoveryAgent>>
RecoveryCoordinator::getActiveAgents() {
    std::shared_lock<std::shared_mutex> lock(agents_mutex_);

    std::vector<std::shared_ptr<RecoveryAgent>> active;
    for (const auto& agent : agents_) {
        if (agent && agent->isActive()) {
            active.push_back(agent);
        }
    }

    return active;
}

void RecoveryCoordinator::updateAll(core::Duration delta_time) {
    std::shared_lock<std::shared_mutex> lock(agents_mutex_);

    for (auto& agent : agents_) {
        if (agent) {
            agent->update(delta_time);
        }
    }

    // Check for collisions periodically
    static int update_count = 0;
    if (++update_count % 100 == 0) {
        checkCollisions();
    }
}

RecoveryStatistics RecoveryCoordinator::getStatistics() const {
    RecoveryStatistics stats;

    {
        std::shared_lock<std::shared_mutex> lock(agents_mutex_);

        stats.total_agents = agents_.size();
        stats.total_lots = getTotalLots();

        for (const auto& agent : agents_) {
            if (!agent) continue;

            if (agent->isActive()) {
                stats.active_agents++;
            }
            if (agent->isFrozen()) {
                stats.frozen_agents++;
            }
            if (agent->isAggressive()) {
                stats.aggressive_count++;
            }
            if (agent->isDefensive()) {
                stats.defensive_count++;
            }
        }
    }

    return stats;
}

void RecoveryCoordinator::freezeByDirection(core::OrderType direction) {
    std::shared_lock<std::shared_mutex> lock(agents_mutex_);

    for (auto& agent : agents_) {
        if (agent && agent->getDirection() == direction) {
            agent->freeze();
        }
    }

    TOPEA_DEBUG("RecoveryCoordinator: Froze agents in direction");
}

void RecoveryCoordinator::unfreezeByDirection(core::OrderType direction) {
    std::shared_lock<std::shared_mutex> lock(agents_mutex_);

    for (auto& agent : agents_) {
        if (agent && agent->getDirection() == direction) {
            agent->unfreeze();
        }
    }

    TOPEA_DEBUG("RecoveryCoordinator: Unfroze agents in direction");
}

void RecoveryCoordinator::accelerateByMission(
    RecoveryMissionType mission,
    double factor) {

    std::shared_lock<std::shared_mutex> lock(agents_mutex_);

    for (auto& agent : agents_) {
        if (agent && agent->getMissionType() == mission) {
            agent->accelerate(factor);
        }
    }

    TOPEA_DEBUG("RecoveryCoordinator: Accelerated {} agents",
                recoveryMissionTypeToString(mission));
}

void RecoveryCoordinator::recycleByMission(
    RecoveryMissionType from_mission,
    RecoveryMissionType to_mission) {

    std::shared_lock<std::shared_mutex> lock(agents_mutex_);

    int recycled = 0;
    for (auto& agent : agents_) {
        if (agent && agent->getMissionType() == from_mission) {
            agent->recycleMission(to_mission);
            recycled++;
        }
    }

    TOPEA_DEBUG("RecoveryCoordinator: Recycled {} agents from {} to {}",
                recycled,
                recoveryMissionTypeToString(from_mission),
                recoveryMissionTypeToString(to_mission));
}

void RecoveryCoordinator::onBasketNearTP() {
    // Slow down recovery agents approaching TP
    std::shared_lock<std::shared_mutex> lock(agents_mutex_);

    for (auto& agent : agents_) {
        if (agent && agent->isActive()) {
            agent->decelerate(0.7);
        }
    }
}

void RecoveryCoordinator::onDeltaCritical(double dominance_pressure) {
    // Accelerate recovery against dominant direction
    std::shared_lock<std::shared_mutex> lock(agents_mutex_);

    for (auto& agent : agents_) {
        if (agent && agent->isActive()) {
            double acceleration = 1.0 + (dominance_pressure * 2.0);
            agent->accelerate(acceleration);
        }
    }
}

void RecoveryCoordinator::onRecoveryActivated() {
    // Transition waiting agents to active state
    std::shared_lock<std::shared_mutex> lock(agents_mutex_);

    for (auto& agent : agents_) {
        if (agent) {
            auto state = agent->getState();
            if (state == RecoveryState::IDLE) {
                agent->transitionState(RecoveryState::SCOUTING);
            }
        }
    }
}

void RecoveryCoordinator::onMomentumChange(double momentum) {
    // Adjust velocity based on momentum
    std::shared_lock<std::shared_mutex> lock(agents_mutex_);

    for (auto& agent : agents_) {
        if (agent && agent->isActive()) {
            // Momentum will be factored in next velocity update
        }
    }
}

void RecoveryCoordinator::logStatistics() const {
    auto stats = getStatistics();

    TOPEA_INFO("\n=== RecoveryCoordinator Statistics ===");
    TOPEA_INFO("  Total Agents: {}", stats.total_agents);
    TOPEA_INFO("  Active Agents: {}", stats.active_agents);
    TOPEA_INFO("  Frozen Agents: {}", stats.frozen_agents);
    TOPEA_INFO("  Total Lots: {}", stats.total_lots);
    TOPEA_INFO("  Aggressive: {}, Defensive: {}, Neutralizing: {}",
               stats.aggressive_count, stats.defensive_count,
               stats.neutralizing_count);
}

void RecoveryCoordinator::checkCollisions() {
    // Prevent agents from stacking too close
    // Implementation details would check distances
}

std::int64_t RecoveryCoordinator::getTotalLots() const {
    std::int64_t total = 0;
    for (const auto& agent : agents_) {
        if (agent) {
            total += agent->getLotSize();
        }
    }
    return total;
}

bool RecoveryCoordinator::canAddAgent() const {
    return agents_.size() < static_cast<std::size_t>(max_agents_);
}

} // namespace topea::engine::recovery
