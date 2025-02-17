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

#include "AuthentCache.h"
#include "IChargePointConfig.h"
#include "IMessageDispatcher.h"
#include "IOcppConfig.h"
#include "Logger.h"

#include <sstream>

using namespace ocpp::database;
using namespace ocpp::types;
using namespace ocpp::messages;

namespace ocpp
{
namespace chargepoint
{

/** @brief Constructor */
AuthentCache::AuthentCache(const ocpp::config::IChargePointConfig&         stack_config,
                           ocpp::config::IOcppConfig&                      ocpp_config,
                           ocpp::database::Database&                       database,
                           const ocpp::messages::GenericMessagesConverter& messages_converter,
                           ocpp::messages::IMessageDispatcher&             msg_dispatcher)
    : GenericMessageHandler<ClearCacheReq, ClearCacheConf>(CLEARCACHE_ACTION, messages_converter),
      m_stack_config(stack_config),
      m_ocpp_config(ocpp_config),
      m_database(database),
      m_find_query(),
      m_delete_query(),
      m_insert_query(),
      m_update_query()
{
    initDatabaseTable();
    msg_dispatcher.registerHandler(CLEARCACHE_ACTION, *this);
}

/** @brief Destructor */
AuthentCache::~AuthentCache() { }

/** @copydoc bool GenericMessageHandler<RequestType, ResponseType>::handleMessage(const RequestType& request,
 *                                                                                ResponseType& response,
 *                                                                                const char*& error_code,
 *                                                                                std::string& error_message)
 */
bool AuthentCache::handleMessage(const ocpp::messages::ClearCacheReq& request,
                                 ocpp::messages::ClearCacheConf&      response,
                                 const char*&                         error_code,
                                 std::string&                         error_message)
{
    (void)request;
    (void)error_code;
    (void)error_message;

    LOG_INFO << "Clear cache requested";

    if (m_ocpp_config.authorizationCacheEnabled())
    {
        clear();
        response.status = ClearCacheStatus::Accepted;
    }
    else
    {
        response.status = ClearCacheStatus::Rejected;
    }

    LOG_INFO << "Clear cache status : " << ClearCacheStatusHelper.toString(response.status);

    return true;
}

/** @brief Look for a tag id in the cache */
bool AuthentCache::check(const std::string& id_tag, ocpp::types::IdTagInfo& tag_info)
{
    bool ret = false;

    if (m_find_query)
    {
        // Execute query
        m_find_query->reset();
        m_find_query->bind(0, id_tag);
        if (m_find_query->exec())
        {
            // Check if a match has been found
            ret = m_find_query->hasRows();
            if (ret)
            {
                // Extract data
                bool        expiry_valid = !m_find_query->isNull(3);
                std::time_t expiry       = m_find_query->getInt64(3);
                tag_info.parentIdTag.value().assign(m_find_query->getString(2));
                tag_info.status = static_cast<AuthorizationStatus>(m_find_query->getInt32(4));

                // Check expiry date
                std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                if (expiry_valid && (expiry < now))
                {
                    // Entry is no more valid, delete entry
                    if (m_delete_query)
                    {
                        m_delete_query->reset();
                        m_delete_query->bind(0, id_tag);
                        m_delete_query->exec();
                    }
                    ret = false;
                }
            }
        }
    }
    return ret;
}

/** @brief Update a tag id entry in the cache */
void AuthentCache::update(const std::string& id_tag, const ocpp::types::IdTagInfo& tag_info)
{
    // Look for the entry
    if (m_find_query)
    {
        // Execute query
        m_find_query->reset();
        m_find_query->bind(0, id_tag);
        if (m_find_query->exec())
        {
            if (m_find_query->hasRows())
            {
                // If new status is not Accepted, remove entry from the cache
                if (tag_info.status != AuthorizationStatus::Accepted)
                {
                    // Remove entry
                    if (m_delete_query)
                    {
                        m_delete_query->reset();
                        m_delete_query->bind(0, id_tag);
                        if (!m_delete_query->exec())
                        {
                            LOG_ERROR << "Could not delete IdTag [" << id_tag << "]";
                        }
                        else
                        {
                            LOG_DEBUG << "IdTag [" << id_tag << "] deleted";
                        }
                    }
                }
                else
                {
                    // Update entry
                    if (m_update_query)
                    {
                        int entry = m_find_query->getInt32(0);
                        m_update_query->reset();
                        m_update_query->bind(0, tag_info.parentIdTag.value());
                        if (tag_info.expiryDate.isSet())
                        {
                            m_update_query->bind(1, tag_info.expiryDate.value().timestamp());
                        }
                        else
                        {
                            m_update_query->bind(1);
                        }
                        m_update_query->bind(2, static_cast<int>(tag_info.status));
                        m_update_query->bind(3, entry);
                        if (!m_update_query->exec())
                        {
                            LOG_ERROR << "Could not update idTag [" << id_tag << "]";
                        }
                        else
                        {
                            LOG_DEBUG << "IdTag [" << id_tag << "] updated";
                        }
                    }
                }
            }
            else
            {
                // Create entry only for Accepted status since other status doesn't allow charge
                if (tag_info.status == AuthorizationStatus::Accepted)
                {
                    if (m_insert_query)
                    {
                        m_insert_query->reset();
                        m_insert_query->bind(0, id_tag);
                        m_insert_query->bind(1, tag_info.parentIdTag.value());
                        if (tag_info.expiryDate.isSet())
                        {
                            m_insert_query->bind(2, tag_info.expiryDate.value().timestamp());
                        }
                        else
                        {
                            m_insert_query->bind(2);
                        }
                        m_insert_query->bind(3, static_cast<int>(tag_info.status));
                        if (!m_insert_query->exec())
                        {
                            LOG_ERROR << "Could not insert idTag [" << id_tag << "]";
                        }
                        else
                        {
                            LOG_DEBUG << "IdTag [" << id_tag << "] inserted";
                        }
                    }
                }
            }
        }
    }
}

/** @brief Clear the cache */
void AuthentCache::clear()
{
    auto query = m_database.query("DELETE FROM AuthentCache WHERE TRUE;");
    if (query.get())
    {
        query->exec();
    }
}

/** @brief Initialize the database table */
void AuthentCache::initDatabaseTable()
{
    // Create database
    auto query = m_database.query("CREATE TABLE IF NOT EXISTS AuthentCache ("
                                  "[id]	INTEGER,"
                                  "[tag]	VARCHAR(20),"
                                  "[parent]	VARCHAR(20),"
                                  "[expiry]	INTEGER,"
                                  "[status]	INTEGER,"
                                  "PRIMARY KEY([id] AUTOINCREMENT));");
    if (query)
    {
        if (!query->exec())
        {
            LOG_ERROR << "Could not create authent cache table : " << query->lastError();
        }
    }
    std::stringstream trigger_query;
    trigger_query << "CREATE TRIGGER delete_oldest_AuthentCache AFTER INSERT ON AuthentCache WHEN "
                     " ((SELECT count() FROM AuthentCache) > ";
    trigger_query << m_stack_config.authentCacheMaxEntriesCount();
    trigger_query << ") BEGIN DELETE FROM AuthentCache WHERE ROWID IN "
                     "(SELECT ROWID FROM AuthentCache LIMIT 1);END;";
    query = m_database.query(trigger_query.str());
    if (query)
    {
        if (!query->exec())
        {
            LOG_ERROR << "Could not create authent cache trigger  : " << query->lastError();
        }
    }

    // Create parametrized queries
    m_find_query   = m_database.query("SELECT * FROM AuthentCache WHERE tag=?;");
    m_delete_query = m_database.query("DELETE FROM AuthentCache WHERE tag=?;");
    m_insert_query = m_database.query("INSERT INTO AuthentCache VALUES (NULL, ?, ?, ?, ?);");
    m_update_query = m_database.query("UPDATE AuthentCache SET [parent]=?, [expiry]=?, [status]=? WHERE id=?;");
}

} // namespace chargepoint
} // namespace ocpp
