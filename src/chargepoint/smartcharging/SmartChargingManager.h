/*
Copyright (c) 2020 Cedric Jimenez
This file is part of OpenOCPP.

OpenOCPP is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenOCPP is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with OpenOCPP. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SMARTCHARGINGMANAGER_H
#define SMARTCHARGINGMANAGER_H

#include "ClearChargingProfile.h"
#include "Enums.h"
#include "GenericMessageHandler.h"
#include "GetCompositeSchedule.h"
#include "ISmartChargingManager.h"
#include "ProfileDatabase.h"
#include "SetChargingProfile.h"
#include "Timer.h"

#include <mutex>

namespace ocpp
{
// Forward declarations
namespace config
{
class IOcppConfig;
class IChargePointConfig;
} // namespace config
namespace messages
{
class IMessageDispatcher;
class GenericMessagesConverter;
class GenericMessageSender;
} // namespace messages
namespace helpers
{
class WorkerThreadPool;
} // namespace helpers

// Main namespace
namespace chargepoint
{

class Connectors;
struct Connector;

/** @brief Handle smart charging for the charge point */
class SmartChargingManager
    : public ISmartChargingManager,
      public ocpp::messages::GenericMessageHandler<ocpp::messages::ClearChargingProfileReq, ocpp::messages::ClearChargingProfileConf>,
      public ocpp::messages::GenericMessageHandler<ocpp::messages::SetChargingProfileReq, ocpp::messages::SetChargingProfileConf>,
      public ocpp::messages::GenericMessageHandler<ocpp::messages::GetCompositeScheduleReq, ocpp::messages::GetCompositeScheduleConf>

{
  public:
    /** @brief Constructor */
    SmartChargingManager(const ocpp::config::IChargePointConfig&         stack_config,
                         ocpp::config::IOcppConfig&                      ocpp_config,
                         ocpp::database::Database&                       database,
                         ocpp::helpers::TimerPool&                       timer_pool,
                         ocpp::helpers::WorkerThreadPool&                worker_pool,
                         Connectors&                                     connectors,
                         const ocpp::messages::GenericMessagesConverter& messages_converter,
                         ocpp::messages::IMessageDispatcher&             msg_dispatcher);

    /** @brief Destructor */
    virtual ~SmartChargingManager();

    // ISmartChargingManager interface

    /** @copydoc bool ISmartChargingManager::getSetpoint(unsigned int,
                                                         ocpp::types::Optional<ocpp::types::SmartChargingSetpoint>&,
                                                         ocpp::types::Optional<ocpp::types::SmartChargingSetpoint>&,
                                                         ocpp::types::ChargingRateUnitType) */
    bool getSetpoint(unsigned int                                               connector_id,
                     ocpp::types::Optional<ocpp::types::SmartChargingSetpoint>& charge_point_setpoint,
                     ocpp::types::Optional<ocpp::types::SmartChargingSetpoint>& connector_setpoint,
                     ocpp::types::ChargingRateUnitType                          unit) override;

    /** @copydoc bool ISmartChargingManager::installTxProfile(unsigned int, const ocpp::types::ChargingProfile&) */
    bool installTxProfile(unsigned int connector_id, const ocpp::types::ChargingProfile& profile) override;

    /** @copydoc void ISmartChargingManager::assignPendingTxProfiles(unsigned int), unsigned int) */
    void assignPendingTxProfiles(unsigned int connector_id, int transaction_id) override;

    /** @copydoc void ISmartChargingManager::clearTxProfiles(unsigned int) */
    void clearTxProfiles(unsigned int connector_id) override;

    // GenericMessageHandler interface

    /** @copydoc bool GenericMessageHandler<RequestType, ResponseType>::handleMessage(const RequestType& request,
     *                                                                                ResponseType& response,
     *                                                                                const char*& error_code,
     *                                                                                std::string& error_message)
     */
    bool handleMessage(const ocpp::messages::ClearChargingProfileReq& request,
                       ocpp::messages::ClearChargingProfileConf&      response,
                       const char*&                                   error_code,
                       std::string&                                   error_message) override;

    /** @copydoc bool GenericMessageHandler<RequestType, ResponseType>::handleMessage(const RequestType& request,
     *                                                                                ResponseType& response,
     *                                                                                const char*& error_code,
     *                                                                                std::string& error_message)
     */
    bool handleMessage(const ocpp::messages::SetChargingProfileReq& request,
                       ocpp::messages::SetChargingProfileConf&      response,
                       const char*&                                 error_code,
                       std::string&                                 error_message) override;

    /** @copydoc bool GenericMessageHandler<RequestType, ResponseType>::handleMessage(const RequestType& request,
     *                                                                                ResponseType& response,
     *                                                                                const char*& error_code,
     *                                                                                std::string& error_message)
     */
    bool handleMessage(const ocpp::messages::GetCompositeScheduleReq& request,
                       ocpp::messages::GetCompositeScheduleConf&      response,
                       const char*&                                   error_code,
                       std::string&                                   error_message) override;

  private:
    /** @brief Stack configuration */
    const ocpp::config::IChargePointConfig& m_stack_config;
    /** @brief Standard OCPP configuration */
    ocpp::config::IOcppConfig& m_ocpp_config;
    /** @brief Worker thread pool */
    ocpp::helpers::WorkerThreadPool& m_worker_pool;
    /** @brief Connectors */
    Connectors& m_connectors;

    /** @brief Profile database */
    ProfileDatabase m_profile_db;

    /** @brief Protect simultaneous access to profiles */
    std::mutex m_mutex;
    /** @brief Profile cleanup timer */
    ocpp::helpers::Timer m_cleanup_timer;

    /** @brief Periodically cleanup expired profiles */
    void cleanupProfiles();

    /** @brief Compute the setpoint of a given connector with a profile list */
    void computeSetpoint(Connector*                                                 connector,
                         ocpp::types::Optional<ocpp::types::SmartChargingSetpoint>& connector_setpoint,
                         ocpp::types::ChargingRateUnitType                          unit,
                         const ProfileDatabase::ChargingProfileList&                profiles_list);

    /** @brief Check if the given profile is active */
    bool isProfileActive(Connector*                                  connector,
                         const ocpp::types::ChargingProfile&         profile,
                         const ocpp::types::ChargingSchedulePeriod*& period);

    /** @brief Fill a setpoint structure with a charging profile and a charging schedule period */
    void fillSetpoint(ocpp::types::SmartChargingSetpoint&        setpoint,
                      ocpp::types::ChargingRateUnitType          unit,
                      const ocpp::types::ChargingProfile&        profile,
                      const ocpp::types::ChargingSchedulePeriod& period);

    /** @brief Convert charging rate units */
    float convertToUnit(float value, ocpp::types::ChargingRateUnitType unit, unsigned int number_phases);
};

} // namespace chargepoint
} // namespace ocpp

#endif // SMARTCHARGINGMANAGER_H
