#ifndef TOPEA_ENGINE_RECOVERY_ENGINE_HPP
#define TOPEA_ENGINE_RECOVERY_ENGINE_HPP

#include "recovery_agent.hpp"
#include "recovery_coordinator.hpp"
#include "recovery_velocity.hpp"
#include "neutralization_engine.hpp"
#include "../../core/engine_module.hpp"
#include "../../shared/state_manager.hpp"
#include <memory>
#include <shared_mutex>

namespace topea::engine::recovery {

// ============================================================
// RECOVERY ENGINE
// ============================================================

/**
 * Main Recovery Engine module.
 * Orchestrates defensive strategies and neutralization.
 * Responsibilities:
 * - Create and manage recovery agents
 * - Calculate neutralization strategies
 * - Coordinate recovery operations
 * - Interact with Delta and Pending engines
 */
class RecoveryEngine : public core::IRecoveryEngine {
public:
    explicit RecoveryEngine(
        std::shared_ptr<core::EventBus> event_bus,
        std::shared_ptr<shared::StateManager> state_manager);

    ~RecoveryEngine() override = default;

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
    // RECOVERY ENGINE INTERFACE
    // ========================================================

    /**
     * Create recovery agent
     */
    std::shared_ptr<RecoveryAgent> createRecoveryAgent(
        const std::string& symbol,
        core::OrderType direction,
        core::Lot lot_size,
        RecoveryMissionType mission);

    /**
     * Create aggressive recovery
     */
    std::shared_ptr<RecoveryAgent> createAggressiveRecovery(
        const std::string& symbol,
        core::OrderType direction,
        core::Lot lot_size);

    /**
     * Create soft recovery
     */
    std::shared_ptr<RecoveryAgent> createSoftRecovery(
        const std::string& symbol,
        core::OrderType direction,
        core::Lot lot_size);

    /**
     * Remove recovery agent
     */
    bool removeRecoveryAgent(std::uint64_t agent_id);

    /**
     * Get recovery agent
     */
    std::shared_ptr<RecoveryAgent> getRecoveryAgent(std::uint64_t agent_id);

    /**
     * Get active recovery agents
     */
    std::vector<std::shared_ptr<RecoveryAgent>> getActiveRecoveries();

    /**
     * Calculate neutralization
     */
    NeutralizationMetrics calculateNeutralization(
        core::Lot buy_lots,
        core::Lot sell_lots,
        double buy_floating,
        double sell_floating);

    /**
     * Batch operations
     */
    void freezeRecoveriesByDirection(core::OrderType direction);
    void accelerateRecoveriesByMission(
        RecoveryMissionType mission,
        double factor);
    void recycleRecoveries(
        RecoveryMissionType from_mission,
        RecoveryMissionType to_mission);

    /**
     * Statistics
     */
    RecoveryStatistics getStatistics() const;
    void logStatistics() const;

private:
    // Dependencies
    std::shared_ptr<core::EventBus> event_bus_;
    std::shared_ptr<shared::StateManager> state_manager_;

    // Components
    std::unique_ptr<RecoveryCoordinator> coordinator_;
    std::unique_ptr<NeutralizationEngine> neutralizer_;

    // State
    std::uint64_t next_agent_id_ = 2000;
    bool initialized_ = false;

    // Internal methods
    core::EventBus* getEventBus() const override;
    void subscribeToEvents();
    void updateRecoveries();

    // Event handlers
    void onBasketNearTP(const core::Event& event);
    void onDeltaCritical(const core::Event& event);
    void onRecoveryActivated(const core::Event& event);
};

} // namespace topea::engine::recovery

#endif // TOPEA_ENGINE_RECOVERY_ENGINE_HPP
