#ifndef TOPEA_CORE_ENGINE_MODULE_HPP
#define TOPEA_CORE_ENGINE_MODULE_HPP

#include "types.hpp"
#include "event_system.hpp"
#include <memory>
#include <string>

namespace topea::core {

// ============================================================
// ENGINE MODULE INTERFACE
// ============================================================

/**
 * Abstract base class for all engine modules (kingdoms)
 * Each module represents a "kingdom" with its own logic
 */
class IEngineModule {
public:
    virtual ~IEngineModule() = default;

    /**
     * Get module name
     */
    virtual std::string getName() const = 0;

    /**
     * Get module description
     */
    virtual std::string getDescription() const { return ""; }

    /**
     * Initialize module
     * Called during engine startup
     */
    virtual bool initialize() = 0;

    /**
     * Shutdown module
     * Called during engine shutdown
     */
    virtual bool shutdown() = 0;

    /**
     * Update module state
     * Called every cycle
     */
    virtual void update(Duration delta_time) = 0;

    /**
     * Check if module is initialized
     */
    virtual bool isInitialized() const = 0;

    /**
     * Reset module to default state
     */
    virtual void reset() {}

protected:
    /**
     * Get event bus reference
     */
    virtual EventBus* getEventBus() const = 0;

    /**
     * Emit event from module
     */
    void emitEvent(const Event& event) {
        if (auto bus = getEventBus()) {
            bus->dispatch(event);
        }
    }
};

// ============================================================
// SPECIFIC MODULE TYPES (Kingdoms to be implemented)
// ============================================================

/**
 * Pending Engine - Manages pending orders elasticity
 * (To be implemented in Phase 2)
 */
class IPendingEngine : public IEngineModule {
public:
    std::string getName() const override { return "PendingEngine"; }
};

/**
 * Recovery Engine - Manages recovery and delta neutralization
 * (To be implemented in Phase 2)
 */
class IRecoveryEngine : public IEngineModule {
public:
    std::string getName() const override { return "RecoveryEngine"; }
};

/**
 * Basket Engine - Monitors and manages basket state
 * (To be implemented in Phase 2)
 */
class IBasketEngine : public IEngineModule {
public:
    std::string getName() const override { return "BasketEngine"; }
};

/**
 * Delta Engine - Handles delta neutralization logic
 * (To be implemented in Phase 2)
 */
class IDeltaEngine : public IEngineModule {
public:
    std::string getName() const override { return "DeltaEngine"; }
};

/**
 * Risk Engine - Manages risk and position sizing
 * (To be implemented in Phase 2)
 */
class IRiskEngine : public IEngineModule {
public:
    std::string getName() const override { return "RiskEngine"; }
};

/**
 * MetaApi Engine - Handles MetaApi MT4/MT5 communication
 * (To be implemented in Phase 3)
 */
class IMetaApiEngine : public IEngineModule {
public:
    std::string getName() const override { return "MetaApiEngine"; }
};

} // namespace topea::core

#endif // TOPEA_CORE_ENGINE_MODULE_HPP
