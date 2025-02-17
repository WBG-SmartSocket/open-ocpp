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

#ifndef TRANSACTIONMANAGER_H
#define TRANSACTIONMANAGER_H

#include "Enums.h"
#include "GenericMessageHandler.h"
#include "RemoteStartTransaction.h"
#include "RemoteStopTransaction.h"
#include "RequestFifo.h"
#include "Timer.h"

namespace ocpp
{
// Forward declarations
namespace config
{
class IOcppConfig;
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

class AuthentManager;
class Connectors;
class ReservationManager;
class IChargePointEventsHandler;
class IMeterValuesManager;
class ISmartChargingManager;
class IStatusManager;

/** @brief Handle charge point transaction requests */
class TransactionManager
    : public ocpp::messages::GenericMessageHandler<ocpp::messages::RemoteStartTransactionReq, ocpp::messages::RemoteStartTransactionConf>,
      public ocpp::messages::GenericMessageHandler<ocpp::messages::RemoteStopTransactionReq, ocpp::messages::RemoteStopTransactionConf>
{
  public:
    /** @brief Constructor */
    TransactionManager(ocpp::config::IOcppConfig&                      ocpp_config,
                       IChargePointEventsHandler&                      events_handler,
                       ocpp::helpers::TimerPool&                       timer_pool,
                       ocpp::helpers::WorkerThreadPool&                worker_pool,
                       ocpp::database::Database&                       database,
                       Connectors&                                     connectors,
                       const ocpp::messages::GenericMessagesConverter& messages_converter,
                       ocpp::messages::IMessageDispatcher&             msg_dispatcher,
                       ocpp::messages::GenericMessageSender&           msg_sender,
                       IStatusManager&                                 status_manager,
                       AuthentManager&                                 authent_manager,
                       ReservationManager&                             reservation_manager,
                       IMeterValuesManager&                            meter_values_manager,
                       ISmartChargingManager&                          smart_charging_manager);

    /** @brief Destructor */
    virtual ~TransactionManager();

    /**
     * @brief Update the charge point connection status
     * @param is_connected true if the charge point is connected to the central system, false otherwise
     */
    void updateConnectionStatus(bool is_connected);

    /**
     * @brief Start a transaction
     * @param connector_id Id of the connector
     * @param id_tag Id of the user
     * @return ocpp::types::AuthorizationStatus (see AuthorizationStatus enum)
     */
    ocpp::types::AuthorizationStatus startTransaction(unsigned int connector_id, const std::string& id_tag);

    /**
     * @brief Stop a transaction
     * @param connector_id Id of the connector
     * @param id_tag Id of the user (leave empty if no id tag)
     * @param reason Stop reason
     * @return true if a corresponding transaction exist and has been stopped, false otherwise
     */
    bool stopTransaction(unsigned int connector_id, const std::string& id_tag, ocpp::types::Reason reason);

    // GenericMessageHandler interface

    /** @copydoc bool GenericMessageHandler<RequestType, ResponseType>::handleMessage(const RequestType& request,
     *                                                                                ResponseType& response,
     *                                                                                const char*& error_code,
     *                                                                                std::string& error_message)
     */
    bool handleMessage(const ocpp::messages::RemoteStartTransactionReq& request,
                       ocpp::messages::RemoteStartTransactionConf&      response,
                       const char*&                                     error_code,
                       std::string&                                     error_message) override;

    /** @copydoc bool GenericMessageHandler<RequestType, ResponseType>::handleMessage(const RequestType& request,
     *                                                                                ResponseType& response,
     *                                                                                const char*& error_code,
     *                                                                                std::string& error_message)
     */
    bool handleMessage(const ocpp::messages::RemoteStopTransactionReq& request,
                       ocpp::messages::RemoteStopTransactionConf&      response,
                       const char*&                                    error_code,
                       std::string&                                    error_message) override;

  private:
    /** @brief Standard OCPP configuration */
    ocpp::config::IOcppConfig& m_ocpp_config;
    /** @brief User defined events handler */
    IChargePointEventsHandler& m_events_handler;
    /** @brief Worker thread pool */
    ocpp::helpers::WorkerThreadPool& m_worker_pool;
    /** @brief Charge point's connectors */
    Connectors& m_connectors;
    /** @brief Message sender */
    ocpp::messages::GenericMessageSender& m_msg_sender;
    /** @brief Status manager */
    IStatusManager& m_status_manager;
    /** @brief Authentication manager */
    AuthentManager& m_authent_manager;
    /** @brief Reservation manager */
    ReservationManager& m_reservation_manager;
    /** @brief Meter values manager */
    IMeterValuesManager& m_meter_values_manager;
    /** @brief Smart charging manager */
    ISmartChargingManager& m_smart_charging_manager;

    /** @brief Transaction related requests FIFO */
    RequestFifo m_requests_fifo;
    /** @brief FIFO retry timer */
    ocpp::helpers::Timer m_request_retry_timer;
    /** @brief Retry count for the current request */
    unsigned int m_request_retry_count;

    /** @brief Process a FIFO request */
    void processFifoRequest();
};

} // namespace chargepoint
} // namespace ocpp

#endif // TRANSACTIONMANAGER_H
