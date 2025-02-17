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

#ifndef REQUESTFIFO_H
#define REQUESTFIFO_H

#include "Database.h"
#include "IRequestFifo.h"

#include <mutex>
#include <queue>

namespace ocpp
{
namespace chargepoint
{

/** @brief Handle in-order retransmission of request and persistency across reboots */
class RequestFifo : public ocpp::messages::IRequestFifo
{
  public:
    /** @brief Constructor */
    RequestFifo(ocpp::database::Database& database);

    /** @brief Destructor */
    virtual ~RequestFifo();

    // IRequestFifo interface

    /** @copydoc void IRequestFifo::push(const std::string&, const rapidjson::Document&) const */
    void push(const std::string& action, const rapidjson::Document& payload) override;

    /** @copydoc bool IRequestFifo::front(std::string&, const rapidjson::Document&) const */
    bool front(std::string& action, rapidjson::Document& payload) override;

    /** @@copydoc void IRequestFifo::pop() */
    void pop() override;

    /** @copydoc size_t IRequestFifo::size() const */
    size_t size() const override;

    // RequestFifo interface

  private:
    /** @brief FIFO entry */
    struct Entry
    {
        /** @brief Default constructor */
        Entry() : id(0), action(), request() { }
        /** @brief Constructor */
        Entry(unsigned int _id, std::string _action, std::string _request) : id(_id), action(_action), request(_request) { }

        /** @brief Id */
        unsigned int id;
        /** @brief Action */
        std::string action;
        /** @brief Request */
        std::string request;
    };

    /** @brief Charge point's database */
    ocpp::database::Database& m_database;

    /** @brief Query to delete a request */
    std::unique_ptr<ocpp::database::Database::Query> m_delete_query;
    /** @brief Query to insert a request */
    std::unique_ptr<ocpp::database::Database::Query> m_insert_query;

    /** @brief Protect simultaneous access to FIFO */
    mutable std::mutex m_mutex;
    /** @brief FIFO */
    std::queue<Entry> m_fifo;
    /** @brief Current id of the request */
    unsigned int m_id;

    /** @brief Initialize the database table */
    void initDatabaseTable();

    /** @brief Load requests from the database */
    void load();
};

} // namespace chargepoint
} // namespace ocpp

#endif // REQUESTFIFO_H
