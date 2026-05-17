#include "recovery_state_machine.hpp"
#include "../../core/logger.hpp"
#include <algorithm>

namespace topea::engine::recovery {

RecoveryStateMachine::RecoveryStateMachine()
    : state_entry_time_(std::chrono::high_resolution_clock::now()) {
}

bool RecoveryStateMachine::transitionTo(RecoveryState new_state) {
    if (!isValidTransition(current_state_, new_state)) {
        TOPEA_WARNING("RecoveryStateMachine: Invalid transition {} -> {}",
                      recoveryStateToString(current_state_),
                      recoveryStateToString(new_state));
        return false;
    }

    previous_state_ = current_state_;
    current_state_ = new_state;
    state_entry_time_ = std::chrono::high_resolution_clock::now();
    frozen_ = false;

    TOPEA_DEBUG("RecoveryStateMachine: Transitioned to {}",
                recoveryStateToString(current_state_));

    return true;
}

RecoveryState RecoveryStateMachine::getCurrentState() const {
    return current_state_;
}

RecoveryState RecoveryStateMachine::getPreviousState() const {
    return previous_state_;
}

std::int64_t RecoveryStateMachine::getTimeInState() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - state_entry_time_);
    return duration.count();
}

bool RecoveryStateMachine::isActive() const {
    return current_state_ != RecoveryState::IDLE &&
           current_state_ != RecoveryState::COMPLETED;
}

bool RecoveryStateMachine::isDefensive() const {
    return current_state_ == RecoveryState::DEFENSIVE ||
           current_state_ == RecoveryState::FROZEN;
}

bool RecoveryStateMachine::isAggressive() const {
    return current_state_ == RecoveryState::AGGRESSIVE;
}

bool RecoveryStateMachine::isFrozen() const {
    return frozen_;
}

void RecoveryStateMachine::freeze() {
    if (!frozen_) {
        frozen_ = true;
        TOPEA_DEBUG("RecoveryStateMachine: Recovery frozen");
    }
}

void RecoveryStateMachine::unfreeze() {
    if (frozen_) {
        frozen_ = false;
        TOPEA_DEBUG("RecoveryStateMachine: Recovery unfrozen");
    }
}

std::vector<RecoveryState> RecoveryStateMachine::getAllowedNextStates() const {
    switch (current_state_) {
        case RecoveryState::IDLE:
            return {RecoveryState::SCOUTING, RecoveryState::IDLE};

        case RecoveryState::SCOUTING:
            return {RecoveryState::IDLE,
                    RecoveryState::AGGRESSIVE,
                    RecoveryState::DEFENSIVE};

        case RecoveryState::AGGRESSIVE:
            return {RecoveryState::NEUTRALIZING,
                    RecoveryState::DEFENSIVE,
                    RecoveryState::FROZEN,
                    RecoveryState::AGGRESSIVE};

        case RecoveryState::DEFENSIVE:
            return {RecoveryState::AGGRESSIVE,
                    RecoveryState::NEUTRALIZING,
                    RecoveryState::FROZEN,
                    RecoveryState::DEFENSIVE};

        case RecoveryState::NEUTRALIZING:
            return {RecoveryState::RECYCLING,
                    RecoveryState::COMPLETED,
                    RecoveryState::AGGRESSIVE};

        case RecoveryState::RECYCLING:
            return {RecoveryState::COMPLETED,
                    RecoveryState::AGGRESSIVE};

        case RecoveryState::FROZEN:
            return {RecoveryState::AGGRESSIVE,
                    RecoveryState::DEFENSIVE,
                    RecoveryState::IDLE};

        case RecoveryState::COMPLETED:
            return {RecoveryState::IDLE};

        default:
            return {RecoveryState::IDLE};
    }
}

void RecoveryStateMachine::reset() {
    current_state_ = RecoveryState::IDLE;
    previous_state_ = RecoveryState::IDLE;
    frozen_ = false;
    state_entry_time_ = std::chrono::high_resolution_clock::now();
    TOPEA_DEBUG("RecoveryStateMachine: Reset to IDLE");
}

bool RecoveryStateMachine::isValidTransition(
    RecoveryState from,
    RecoveryState to) const {

    if (from == to) return true;  // Staying in same state is valid

    auto allowed = getAllowedNextStates();
    return std::find(allowed.begin(), allowed.end(), to) != allowed.end();
}

} // namespace topea::engine::recovery
