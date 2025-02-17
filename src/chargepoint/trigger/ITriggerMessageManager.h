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

#ifndef ITRIGGERMESSAGEMANAGER_H
#define ITRIGGERMESSAGEMANAGER_H

#include "Enums.h"

namespace ocpp
{
namespace chargepoint
{

/** @brief Interface for TriggerMessage managers implementation */
class ITriggerMessageManager
{
  public:
    // Forward declaration
    class ITriggerMessageHandler;

    /** @brief Destructor */
    virtual ~ITriggerMessageManager() { }

    /**
     * @brief Register a handler for a specific trigger request
     * @param message Type of trigger message requested
     * @param handler Handler to register
     */
    virtual void registerHandler(ocpp::types::MessageTrigger message, ITriggerMessageHandler& handler) = 0;

    /** @brief Interface for TriggerMessage handlers implementations */
    class ITriggerMessageHandler
    {
      public:
        /** @brief Destructor */
        virtual ~ITriggerMessageHandler() { }

        /**
         * @brief Called on reception of a TriggerMessage request
         * @param message Type of trigger message requested
         * @param connector_id Id of the connector concerned by the request
         * @return true if the requested message can be sent, false otherwise
         */
        virtual bool onTriggerMessage(ocpp::types::MessageTrigger message, unsigned int connector_id) = 0;
    };
};

} // namespace chargepoint
} // namespace ocpp

#endif // ITRIGGERMESSAGEMANAGER_H
