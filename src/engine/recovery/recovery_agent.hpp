#ifndef TOPEA_ENGINE_RECOVERY_AGENT_HPP
#define TOPEA_ENGINE_RECOVERY_AGENT_HPP

#include "recovery_mission.hpp"
#include "recovery_state_machine.hpp"
#include "recovery_velocity.hpp"
#include "../../core/types.hpp"
#include <memory>
#include <string>

namespace topea::engine::recovery {

// ============================================================
// RECOVERY AGENT
// ============================================================

/**
 * Autonomous recovery agent.
 * Represents a single recovery order/operation.
 * Can be:
 * - Aggressive recovery (strong, fast)
 * - Soft recovery (patient, gentle)
 * - Counter recovery (opposite direction)
 * - Defensive recovery (last line)
 * - Neutralization (mathematical balance)
 */
class RecoveryAgent {
public:
    explicit RecoveryAgent(
        std::uint64_t agent_id,
        const std::string& symbol,
        core::OrderType direction,
        core::Lot lot_size,
        RecoveryMissionType mission);

    // ========================================================
    // IDENTIFICATION
    // ========================================================

    std::uint64_t getUniqueId() const;
    std::string getSymbol() const;
    core::OrderType getDirection() const;
    core::Lot getLotSize() const;

    // ========================================================
    // MISSION
    // ========================================================

    RecoveryMissionType getMissionType() const;
    const RecoveryMissionProfile& getMissionProfile() const;
    void setMission(RecoveryMissionType mission);
    void recycleMission(RecoveryMissionType new_mission);

    // ========================================================
    // STATE
    // ========================================================

    RecoveryState getState() const;
    bool isActive() const;
    bool isDefensive() const;
    bool isAggressive() const;
    bool isFrozen() const;

    void freeze();
    void unfreeze();

    // ========================================================
    // POSITION
    // ========================================================

    core::Price getEntryPrice() const;
    void setEntryPrice(core::Price price);

    core::Price getCurrentDistance() const;
    void updateDistance(core::Price current_price);

    core::Price getTargetZone() const;
    void setTargetZone(core::Price target);

    // ========================================================
    // VELOCITY & MOVEMENT
    // ========================================================

    core::Price getVelocity() const;
    void accelerate(double factor = 1.5);
    void decelerate(double factor = 0.7);
    void resetVelocity();

    void updateVelocity(
        core::Duration delta_time,
        double momentum,
        double volatility,
        double pressure);

    // ========================================================
    // TRADING PARAMETERS
    // ========================================================

    double getStrength() const;
    void setStrength(double strength);

    double getAggressionLevel() const;
    void setAggressionLevel(double level);

    // ========================================================
    // STATE TRANSITIONS
    // ========================================================

    bool transitionState(RecoveryState new_state);

    // ========================================================
    // LIFECYCLE
    // ========================================================

    void update(core::Duration delta_time);
    void reset();

    // ========================================================
    // STATISTICS
    // ========================================================

    std::int64_t getTimeActive() const;

private:
    // Identification
    std::uint64_t unique_id_;
    std::string symbol_;
    core::OrderType direction_;
    core::Lot lot_size_;

    // Mission
    RecoveryMissionType current_mission_;
    RecoveryMissionProfile mission_profile_;

    // State
    std::unique_ptr<RecoveryStateMachine> state_machine_;
    std::unique_ptr<RecoveryVelocity> velocity_engine_;

    // Position
    core::Price entry_price_ = 0.0;
    core::Price current_distance_ = 0.0;
    core::Price target_zone_ = 0.0;

    // Strength
    double strength_ = 1.0;
    double aggression_ = 0.5;

    // Timestamps
    std::chrono::high_resolution_clock::time_point creation_time_;
};

// ============================================================
// RECOVERY AGENT FACTORY
// ============================================================

class RecoveryAgentFactory {
public:
    static std::shared_ptr<RecoveryAgent> createAgent(
        std::uint64_t agent_id,
        const std::string& symbol,
        core::OrderType direction,
        core::Lot lot_size,
        RecoveryMissionType mission);

    static std::shared_ptr<RecoveryAgent> createAggressive(
        std::uint64_t agent_id,
        const std::string& symbol,
        core::OrderType direction,
        core::Lot lot_size);

    static std::shared_ptr<RecoveryAgent> createSoft(
        std::uint64_t agent_id,
        const std::string& symbol,
        core::OrderType direction,
        core::Lot lot_size);

    static std::shared_ptr<RecoveryAgent> createDefensive(
        std::uint64_t agent_id,
        const std::string& symbol,
        core::OrderType direction,
        core::Lot lot_size);
};

} // namespace topea::engine::recovery

#endif // TOPEA_ENGINE_RECOVERY_AGENT_HPP
