#ifndef TOPEA_ENGINE_RECOVERY_MISSION_HPP
#define TOPEA_ENGINE_RECOVERY_MISSION_HPP

#include "../../core/types.hpp"

namespace topea::engine::recovery {

// ============================================================
// RECOVERY MISSION TYPES
// ============================================================

enum class RecoveryMissionType {
    AGGRESSIVE_RECOVERY,        // Fast, strong neutralization
    SOFT_RECOVERY,              // Gentle, patient recovery
    COUNTER_RECOVERY,           // Opposite direction protection
    DEFENSIVE_RECOVERY,         // Last line of defense
    NEUTRALIZATION,             // Mathematical balance
    TP_ASSIST,                  // Support reaching TP
    DELTA_SUPPORT,              // Balance directional pressure
    RECYCLE_SUPPORT             // Convert profit to recovery
};

inline const char* recoveryMissionTypeToString(RecoveryMissionType type) {
    switch (type) {
        case RecoveryMissionType::AGGRESSIVE_RECOVERY:
            return "AGGRESSIVE_RECOVERY";
        case RecoveryMissionType::SOFT_RECOVERY:
            return "SOFT_RECOVERY";
        case RecoveryMissionType::COUNTER_RECOVERY:
            return "COUNTER_RECOVERY";
        case RecoveryMissionType::DEFENSIVE_RECOVERY:
            return "DEFENSIVE_RECOVERY";
        case RecoveryMissionType::NEUTRALIZATION:
            return "NEUTRALIZATION";
        case RecoveryMissionType::TP_ASSIST:
            return "TP_ASSIST";
        case RecoveryMissionType::DELTA_SUPPORT:
            return "DELTA_SUPPORT";
        case RecoveryMissionType::RECYCLE_SUPPORT:
            return "RECYCLE_SUPPORT";
    }
    return "UNKNOWN";
}

// ============================================================
// RECOVERY MISSION PROPERTIES
// ============================================================

struct RecoveryMissionProfile {
    RecoveryMissionType type = RecoveryMissionType::SOFT_RECOVERY;
    
    // Recovery strength multiplier (1.0 = base, 2.0 = double)
    double strength = 1.0;
    
    // Velocity multiplier
    double velocity_factor = 1.0;
    
    // Aggression level (0.0 to 1.0)
    double aggression = 0.5;
    
    // Elasticity (ability to adapt)
    double elasticity = 0.7;
    
    // Minimum distance to maintain from price
    core::Price min_distance = 5.0;  // pips
    
    // Maximum distance to maintain
    core::Price max_distance = 50.0;  // pips
    
    // Priority (higher = more important)
    int priority = 50;
    
    // Get properties for mission type
    static RecoveryMissionProfile getProfileFor(RecoveryMissionType type) {
        RecoveryMissionProfile profile;
        
        switch (type) {
            case RecoveryMissionType::AGGRESSIVE_RECOVERY:
                profile.strength = 2.0;
                profile.velocity_factor = 1.5;
                profile.aggression = 0.9;
                profile.priority = 100;
                profile.min_distance = 3.0;
                break;
                
            case RecoveryMissionType::SOFT_RECOVERY:
                profile.strength = 1.0;
                profile.velocity_factor = 0.8;
                profile.aggression = 0.4;
                profile.priority = 60;
                profile.min_distance = 10.0;
                break;
                
            case RecoveryMissionType::COUNTER_RECOVERY:
                profile.strength = 1.5;
                profile.velocity_factor = 1.0;
                profile.aggression = 0.6;
                profile.priority = 80;
                profile.min_distance = 5.0;
                break;
                
            case RecoveryMissionType::DEFENSIVE_RECOVERY:
                profile.strength = 2.5;
                profile.velocity_factor = 1.8;
                profile.aggression = 1.0;
                profile.priority = 120;
                profile.min_distance = 2.0;
                break;
                
            case RecoveryMissionType::NEUTRALIZATION:
                profile.strength = 1.2;
                profile.velocity_factor = 1.2;
                profile.aggression = 0.7;
                profile.priority = 90;
                profile.min_distance = 4.0;
                break;
                
            case RecoveryMissionType::TP_ASSIST:
                profile.strength = 1.3;
                profile.velocity_factor = 1.4;
                profile.aggression = 0.8;
                profile.priority = 75;
                profile.min_distance = 3.0;
                break;
                
            case RecoveryMissionType::DELTA_SUPPORT:
                profile.strength = 1.5;
                profile.velocity_factor = 1.1;
                profile.aggression = 0.5;
                profile.priority = 85;
                profile.min_distance = 6.0;
                break;
                
            case RecoveryMissionType::RECYCLE_SUPPORT:
                profile.strength = 0.8;
                profile.velocity_factor = 0.7;
                profile.aggression = 0.3;
                profile.priority = 40;
                profile.min_distance = 15.0;
                break;
        }
        
        return profile;
    }
};

} // namespace topea::engine::recovery

#endif // TOPEA_ENGINE_RECOVERY_MISSION_HPP
