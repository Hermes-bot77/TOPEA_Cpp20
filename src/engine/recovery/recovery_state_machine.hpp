#ifndef TOPEA_ENGINE_RECOVERY_STATE_MACHINE_HPP
#define TOPEA_ENGINE_RECOVERY_STATE_MACHINE_HPP

#include <cstdint>
#include <chrono>
#include <vector>

namespace topea::engine::recovery {

// ============================================================
// RECOVERY STATE MACHINE STATES
// ============================================================

enum class RecoveryState {
    IDLE,                       // Not active
    SCOUTING,                   // Looking for entry
    AGGRESSIVE,                 // Full force recovery
    DEFENSIVE,                  // Protecting against loss
    NEUTRALIZING,               // Balancing positions
    RECYCLING,                  // Converting profit to recovery
    FROZEN,                     // Temporarily stopped
    COMPLETED,                  // Mission accomplished
    UNKNOWN
};

inline const char* recoveryStateToString(RecoveryState state) {
    switch (state) {
        case RecoveryState::IDLE:
            return "IDLE";
        case RecoveryState::SCOUTING:
            return "SCOUTING";
        case RecoveryState::AGGRESSIVE:
            return "AGGRESSIVE";
        case RecoveryState::DEFENSIVE:
            return "DEFENSIVE";
        case RecoveryState::NEUTRALIZING:
            return "NEUTRALIZING";
        case RecoveryState::RECYCLING:
            return "RECYCLING";
        case RecoveryState::FROZEN:
            return "FROZEN";
        case RecoveryState::COMPLETED:
            return "COMPLETED";
        default:
            return "UNKNOWN";
    }
}

// ============================================================
// RECOVERY STATE MACHINE
// ============================================================

class RecoveryStateMachine {
public:
    RecoveryStateMachine();

    /**
     * Transition to new state
     */
    bool transitionTo(RecoveryState new_state);

    /**
     * Get current state
     */
    RecoveryState getCurrentState() const;

    /**
     * Get previous state
     */
    RecoveryState getPreviousState() const;

    /**
     * Get time in current state (milliseconds)
     */
    std::int64_t getTimeInState() const;

    /**
     * Check if state is active (not idle/completed)
     */
    bool isActive() const;

    /**
     * Check if defensive
     */
    bool isDefensive() const;

    /**
     * Check if aggressive
     */
    bool isAggressive() const;

    /**
     * Check if frozen
     */
    bool isFrozen() const;

    /**
     * Freeze recovery (temporary stop)
     */
    void freeze();

    /**
     * Unfreeze recovery
     */
    void unfreeze();

    /**
     * Get allowed next states
     */
    std::vector<RecoveryState> getAllowedNextStates() const;

    /**
     * Reset to idle
     */
    void reset();

private:
    RecoveryState current_state_ = RecoveryState::IDLE;
    RecoveryState previous_state_ = RecoveryState::IDLE;
    bool frozen_ = false;
    std::chrono::high_resolution_clock::time_point state_entry_time_;

    bool isValidTransition(RecoveryState from, RecoveryState to) const;
};

} // namespace topea::engine::recovery

#endif // TOPEA_ENGINE_RECOVERY_STATE_MACHINE_HPP
