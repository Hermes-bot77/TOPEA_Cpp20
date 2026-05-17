#ifndef TOPEA_ENGINE_RECOVERY_COORDINATOR_HPP
#define TOPEA_ENGINE_RECOVERY_COORDINATOR_HPP

#include "recovery_agent.hpp"
#include "recovery_mission.hpp"
#include "../../core/types.hpp"
#include <memory>
#include <vector>
#include <shared_mutex>

namespace topea::engine::recovery {

// ============================================================
// RECOVERY COORDINATOR STATISTICS
// ============================================================

struct RecoveryStatistics {
    std::int64_t total_agents = 0;
    std::int64_t active_agents = 0;
    std::int64_t frozen_agents = 0;
    std::int64_t total_lots = 0;
    std::int64_t aggressive_count = 0;
    std::int64_t defensive_count = 0;
    std::int64_t neutralizing_count = 0;
};

// ============================================================
// RECOVERY COORDINATOR
// ============================================================

/**
 * Orchestrates multiple recovery agents.
 * Responsibilities:
 * - Agent lifecycle management
 * - Collision prevention
 * - Batch operations
 * - Priority management
 * - Statistics tracking
 */
class RecoveryCoordinator {
public:
    explicit RecoveryCoordinator(
        std::int64_t max_agents = 500,
        double min_distance = 5.0);

    /**
     * Initialize coordinator
     */
    void initialize();

    /**
     * Shutdown coordinator
     */
    void shutdown();

    /**
     * Add recovery agent
     */
    bool addAgent(std::shared_ptr<RecoveryAgent> agent);

    /**
     * Remove recovery agent by ID
     */
    bool removeAgent(std::uint64_t agent_id);

    /**
     * Get agent by ID
     */
    std::shared_ptr<RecoveryAgent> getAgent(std::uint64_t agent_id);

    /**
     * Get all active agents
     */
    std::vector<std::shared_ptr<RecoveryAgent>> getActiveAgents();

    /**
     * Update all agents
     */
    void updateAll(core::Duration delta_time);

    /**
     * Get statistics
     */
    RecoveryStatistics getStatistics() const;

    /**
     * Batch operations
     */
    void freezeByDirection(core::OrderType direction);
    void unfreezeByDirection(core::OrderType direction);
    void accelerateByMission(RecoveryMissionType mission, double factor);
    void recycleByMission(
        RecoveryMissionType from_mission,
        RecoveryMissionType to_mission);

    /**
     * Event responses
     */
    void onBasketNearTP();
    void onDeltaCritical(double dominance_pressure);
    void onRecoveryActivated();
    void onMomentumChange(double momentum);

    /**
     * Logging
     */
    void logStatistics() const;

private:
    std::int64_t max_agents_;
    double min_distance_;
    mutable std::shared_mutex agents_mutex_;
    std::vector<std::shared_ptr<RecoveryAgent>> agents_;
    bool initialized_ = false;

    void checkCollisions();
    std::int64_t getTotalLots() const;
    bool canAddAgent() const;
};

} // namespace topea::engine::recovery

#endif // TOPEA_ENGINE_RECOVERY_COORDINATOR_HPP
