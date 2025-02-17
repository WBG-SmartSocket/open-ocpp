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

#ifndef IREQUESTFIFO_H
#define IREQUESTFIFO_H

#include "json.h"

#include <string>

namespace ocpp
{
namespace messages
{

/** @brief Interface for request FIFO implementations */
class IRequestFifo
{
  public:
    /** @brief Destructor */
    virtual ~IRequestFifo() { }

    /**
     * @brief Queue a request inside the FIFO
     * @param action RPC action for the request
     * @param payload JSON payload of the request
     */
    virtual void push(const std::string& action, const rapidjson::Document& payload) = 0;

    /**
     * @brief Get the first request from the FIFO
     * @param action RPC action for the request
     * @param payload JSON payload of the request
     * @return true if a request has been retrived, false if the FIFO is empty
     */
    virtual bool front(std::string& action, rapidjson::Document& payload) = 0;

    /** @brief Delete the first request from the FIFO */
    virtual void pop() = 0;

    /**
     * @brief Get the number of requests inside the FIFO
     * @return Number of requests inside the FIFO
     */
    virtual size_t size() const = 0;
};

} // namespace messages
} // namespace ocpp

#endif // IREQUESTFIFO_H
