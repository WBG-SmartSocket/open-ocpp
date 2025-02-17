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

#ifndef IMETERVALUESMANAGER_H
#define IMETERVALUESMANAGER_H

#include "IRequestFifo.h"
#include "MeterValue.h"

#include <vector>

namespace ocpp
{
namespace chargepoint
{

/** @brief Interface for charge point meter values requests handler */
class IMeterValuesManager
{
  public:
    /** @brief Destructor */
    virtual ~IMeterValuesManager() { }

    /**
     * @brief Set the transaction related requests FIFO to use
     * @param requests_fifo Transaction related requests FIFO to use
     */
    virtual void setTransactionFifo(ocpp::messages::IRequestFifo& requests_fifo) = 0;

    /**
     * @brief Send meter values to Central System for a given connector
     * @param connector_id Id of the connector
     * @param values Meter values to send
     * @return true if the meter values have been sent, false otherwise
     */
    virtual bool sendMeterValues(unsigned int connector_id, const std::vector<ocpp::types::MeterValue>& values) = 0;

    /**
     * @brief Start sending sampled meter values for a given connector
     * @param connector_id Id of the connector
     */
    virtual void startSampledMeterValues(unsigned int connector_id) = 0;

    /**
     * @brief Stop sending sampled meter values for a given connector
     * @param connector_id Id of the connector
     */
    virtual void stopSampledMeterValues(unsigned int connector_id) = 0;

    /**
     * @brief Get the transaction meter values for a given connector
     * @param connector_id Id of the connector
     * @param meter_values Transaction meter values
     */
    virtual void getTxStopMeterValues(unsigned int connector_id, std::vector<ocpp::types::MeterValue>& meter_values) = 0;
};

} // namespace chargepoint
} // namespace ocpp

#endif // IMETERVALUESMANAGER_H
