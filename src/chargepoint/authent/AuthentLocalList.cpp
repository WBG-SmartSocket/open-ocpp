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

#include "AuthentLocalList.h"
#include "IChargePointConfig.h"
#include "IInternalConfigManager.h"
#include "IMessageDispatcher.h"
#include "IOcppConfig.h"
#include "InternalConfigKeys.h"
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
AuthentLocalList::AuthentLocalList(ocpp::config::IOcppConfig&                      ocpp_config,
                                   ocpp::database::Database&                       database,
                                   IInternalConfigManager&                         internal_config,
                                   const ocpp::messages::GenericMessagesConverter& messages_converter,
                                   ocpp::messages::IMessageDispatcher&             msg_dispatcher)
    : GenericMessageHandler<GetLocalListVersionReq, GetLocalListVersionConf>(GET_LOCAL_LIST_VERSION_ACTION, messages_converter),
      GenericMessageHandler<SendLocalListReq, SendLocalListConf>(SEND_LOCAL_LIST_ACTION, messages_converter),
      m_ocpp_config(ocpp_config),
      m_database(database),
      m_internal_config(internal_config),
      m_local_list_version(0),
      m_find_query(),
      m_delete_query(),
      m_insert_query(),
      m_update_query()
{
    initDatabaseTable();
    msg_dispatcher.registerHandler(GET_LOCAL_LIST_VERSION_ACTION,
                                   *dynamic_cast<GenericMessageHandler<GetLocalListVersionReq, GetLocalListVersionConf>*>(this));
    msg_dispatcher.registerHandler(SEND_LOCAL_LIST_ACTION,
                                   *dynamic_cast<GenericMessageHandler<SendLocalListReq, SendLocalListConf>*>(this));

    // Get current local list version
    std::string local_list_version;
    if (m_internal_config.getKey(LOCAL_LIST_VERSION_KEY, local_list_version))
    {
        m_local_list_version = std::atoi(local_list_version.c_str());
        LOG_DEBUG << "Authent local list version : " << m_local_list_version;
    }
    else
    {
        LOG_ERROR << "Unable to retrieve current authent local list version";
    }
}

/** @brief Destructor */
AuthentLocalList::~AuthentLocalList() { }

/** @copydoc bool GenericMessageHandler<RequestType, ResponseType>::handleMessage(const RequestType& request,
 *                                                                                ResponseType& response,
 *                                                                                const char*& error_code,
 *                                                                                std::string& error_message)
 */
bool AuthentLocalList::handleMessage(const ocpp::messages::GetLocalListVersionReq& request,
                                     ocpp::messages::GetLocalListVersionConf&      response,
                                     const char*&                                  error_code,
                                     std::string&                                  error_message)
{
    (void)request;
    (void)error_code;
    (void)error_message;

    LOG_INFO << "Local list version requested : " << m_local_list_version;
    response.listVersion = m_local_list_version;

    return true;
}

/** @copydoc bool GenericMessageHandler<RequestType, ResponseType>::handleMessage(const RequestType& request,
 *                                                                                ResponseType& response,
 *                                                                                const char*& error_code,
 *                                                                                std::string& error_message)
 */
bool AuthentLocalList::handleMessage(const ocpp::messages::SendLocalListReq& request,
                                     ocpp::messages::SendLocalListConf&      response,
                                     const char*&                            error_code,
                                     std::string&                            error_message)
{
    (void)request;
    (void)response;
    (void)error_code;
    (void)error_message;

    LOG_INFO << "Local list update requested : listVersion = " << request.listVersion
             << " - updateType = " << UpdateTypeHelper.toString(request.updateType);

    // Check local list activation
    if (m_ocpp_config.localAuthListEnabled())
    {
        // Check local list version
        if (request.listVersion <= m_local_list_version)
        {
            // Check update type
            bool success;
            if (request.updateType == UpdateType::Full)
            {
                success = performFullUpdate(request.localAuthorizationList);
            }
            else
            {
                success = performPartialUpdate(request.localAuthorizationList);
            }
            if (success)
            {
                response.status = UpdateStatus::Accepted;
            }
            else
            {
                response.status = UpdateStatus::Failed;
            }

            // Update local list version
            m_local_list_version = request.listVersion;
            if (!m_internal_config.setKey(LOCAL_LIST_VERSION_KEY, std::to_string(m_local_list_version)))
            {
                LOG_ERROR << "Unable to save authent local list version";
            }
        }
        else
        {
            response.status = UpdateStatus::VersionMismatch;
        }
    }
    else
    {
        response.status = UpdateStatus::NotSupported;
    }

    LOG_INFO << "Local list update status : " << UpdateStatusHelper.toString(response.status);

    return true;
}

/** @brief Look for a tag id in the local list */
bool AuthentLocalList::check(const std::string& id_tag, ocpp::types::IdTagInfo& tag_info)
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
                    // Entry is no more valid
                    ret = false;
                }
            }
        }
    }
    return ret;
}

/** @brief Initialize the database table */
void AuthentLocalList::initDatabaseTable()
{
    // Create database
    auto query = m_database.query("CREATE TABLE IF NOT EXISTS AuthentLocalList ("
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
            LOG_ERROR << "Could not create authent local list table : " << query->lastError();
        }
    }

    // Create parametrized queries
    m_find_query   = m_database.query("SELECT * FROM AuthentLocalList WHERE tag=?;");
    m_delete_query = m_database.query("DELETE FROM AuthentLocalList WHERE tag=?;");
    m_insert_query = m_database.query("INSERT INTO AuthentLocalList VALUES (NULL, ?, ?, ?, ?);");
    m_update_query = m_database.query("UPDATE AuthentLocalList SET [parent]=?, [expiry]=?, [status]=? WHERE id=?;");

    // Local list version
    if (!m_internal_config.keyExist(LOCAL_LIST_VERSION_KEY))
    {
        m_internal_config.createKey(LOCAL_LIST_VERSION_KEY, std::to_string(m_local_list_version));
    }
}

/** @brief Perform the full update of the local list */
bool AuthentLocalList::performFullUpdate(const std::vector<ocpp::types::AuthorizationData>& authorization_datas)
{
    bool ret = true;

    // Clear local list
    auto query = m_database.query("DELETE FROM AuthentLocalList WHERE TRUE;");
    if (query)
    {
        ret = query->exec();
        if (!ret)
        {
            LOG_ERROR << "Could not clear authent local list table";
        }
    }
    if (ret)
    {
        // Insert new list
        if (m_insert_query)
        {
            for (const AuthorizationData& authorization_data : authorization_datas)
            {
                m_insert_query->reset();
                m_insert_query->bind(0, authorization_data.idTag);
                m_insert_query->bind(1, authorization_data.idTagInfo.value().parentIdTag.value());
                if (authorization_data.idTagInfo.value().expiryDate.isSet())
                {
                    m_insert_query->bind(2, authorization_data.idTagInfo.value().expiryDate.value().timestamp());
                }
                else
                {
                    m_insert_query->bind(2);
                }
                m_insert_query->bind(3, static_cast<int>(authorization_data.idTagInfo.value().status));
                if (!m_insert_query->exec())
                {
                    LOG_ERROR << "Could not insert idTag [" << authorization_data.idTag.str() << "]";
                    ret = false;
                }
                else
                {
                    LOG_DEBUG << "IdTag [" << authorization_data.idTag.str() << "] inserted";
                }
            }
        }
    }

    return ret;
}
/** @brief Perform the partial update of the local list */
bool AuthentLocalList::performPartialUpdate(const std::vector<ocpp::types::AuthorizationData>& authorization_datas)
{
    bool ret = true;

    if (m_delete_query && m_find_query && m_update_query && m_insert_query)
    {
        // Far all idTags
        for (const AuthorizationData& authorization_data : authorization_datas)
        {
            // Check if the idTag must be deleted
            if (!authorization_data.idTagInfo.isSet())
            {
                // Delete entry
                m_delete_query->reset();
                m_delete_query->bind(0, authorization_data.idTag);
                if (!m_delete_query->exec())
                {
                    LOG_ERROR << "Could not delete idTag [" << authorization_data.idTag.str() << "]";
                    ret = false;
                }
                else
                {
                    LOG_DEBUG << "IdTag [" << authorization_data.idTag.str() << "] deleted";
                }
            }
            else
            {
                // Create or update, check if the entry exists
                m_find_query->reset();
                m_find_query->bind(0, authorization_data.idTag);
                if (m_find_query->exec())
                {
                    if (m_find_query->hasRows())
                    {
                        // Update
                        int entry = m_find_query->getInt32(0);
                        m_update_query->reset();
                        m_update_query->bind(0, authorization_data.idTagInfo.value().parentIdTag.value());
                        if (authorization_data.idTagInfo.value().expiryDate.isSet())
                        {
                            m_update_query->bind(1, authorization_data.idTagInfo.value().expiryDate.value().timestamp());
                        }
                        else
                        {
                            m_update_query->bind(1);
                        }
                        m_update_query->bind(2, static_cast<int>(authorization_data.idTagInfo.value().status));
                        m_update_query->bind(3, entry);
                        if (!m_update_query->exec())
                        {
                            LOG_ERROR << "Could not update idTag [" << authorization_data.idTag.str() << "]";
                        }
                        else
                        {
                            LOG_DEBUG << "IdTag [" << authorization_data.idTag.str() << "] updated";
                        }
                    }
                    else
                    {
                        // Insert
                        m_insert_query->reset();
                        m_insert_query->bind(0, authorization_data.idTag);
                        m_insert_query->bind(1, authorization_data.idTagInfo.value().parentIdTag.value());
                        if (authorization_data.idTagInfo.value().expiryDate.isSet())
                        {
                            m_insert_query->bind(2, authorization_data.idTagInfo.value().expiryDate.value().timestamp());
                        }
                        else
                        {
                            m_insert_query->bind(2);
                        }
                        m_insert_query->bind(3, static_cast<int>(authorization_data.idTagInfo.value().status));
                        if (!m_insert_query->exec())
                        {
                            LOG_ERROR << "Could not insert idTag [" << authorization_data.idTag.str() << "]";
                            ret = false;
                        }
                        else
                        {
                            LOG_DEBUG << "IdTag [" << authorization_data.idTag.str() << "] inserted";
                        }
                    }
                }
                else
                {
                    ret = false;
                }
            }
        }
    }

    return ret;
}

} // namespace chargepoint
} // namespace ocpp
