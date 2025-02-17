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

#ifndef LIBWEBSOCKETSERVER_H
#define LIBWEBSOCKETSERVER_H

#include "IWebsocketServer.h"
#include "Queue.h"
#include "Url.h"
#include "libwebsockets.h"

#include <array>
#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>

namespace ocpp
{
namespace websockets
{

/** @brief Websocket server implementation using libwebsockets */
class LibWebsocketServer : public IWebsocketServer
{
  public:
    /** @brief Constructor */
    LibWebsocketServer();
    /** @brief Destructor */
    virtual ~LibWebsocketServer();

    /** @copydoc bool IWebsocketServer::start(const std::string&, const std::string&, const Credentials&,
     *                                        std::chrono::milliseconds) */
    bool start(const std::string&        url,
               const std::string&        protocol,
               const Credentials&        credentials,
               std::chrono::milliseconds ping_interval = std::chrono::seconds(5)) override;

    /** @copydoc bool IWebsocketServer::stop() */
    bool stop() override;

    /** @copydoc void IWebsocketServer::registerListener(IListener&) */
    void registerListener(IListener& listener) override;

  private:
    /** @brief Message to send */
    struct SendMsg
    {
        /** @brief Constructor */
        SendMsg(const void* _data, size_t _size)
        {
            data    = new unsigned char[LWS_PRE + _size];
            size    = _size;
            payload = &data[LWS_PRE];
            memcpy(payload, _data, size);
        }
        /** @brief Destructor */
        virtual ~SendMsg() { delete[] data; }

        /** @brief Data buffer */
        unsigned char* data;
        /** @brief Payload start */
        unsigned char* payload;
        /** @brief Size in bytes */
        size_t size;
    };

    /** @brief Websocket client connection */
    class Client : public IClient
    {
        friend class LibWebsocketServer;

      public:
        /**
         * @brief Constructor
         * @param wsi Client socket
        */
        Client(struct lws* wsi);
        /** @brief Destructor */
        virtual ~Client();

        /** @copydoc bool IClient::disconnect() */
        bool disconnect() override;

        /** @copydoc bool IClient::disconnect() */
        bool isConnected() override;

        /** @copydoc bool IClient::send(const void*, size_t) */
        bool send(const void* data, size_t size) override;

        /** @copydoc bool IClient::registerListener(IListener&) */
        void registerListener(IClient::IListener& listener) override;

      private:
        /** @brief Client socket */
        struct lws* m_wsi;
        /** @brief Connection status */
        bool m_connected;
        /** @brief Listener */
        IClient::IListener* m_listener;
        /** @brief Queue of messages to send */
        ocpp::helpers::Queue<SendMsg*> m_send_msgs;
    };

    /** @brief Listener */
    IListener* m_listener;
    /** @brief Internal thread */
    std::thread* m_thread;
    /** @brief Indicate the end of processing to the thread */
    bool m_end;
    /** @brief Connection URL */
    Url m_url;
    /** @brief Name of the protocol to use */
    std::string m_protocol;
    /** @brief Credentials */
    Credentials m_credentials;

    /** @brief Websocket context */
    struct lws_context* m_context;
    /** @brief Related wsi */
    struct lws* m_wsi;
    /** @brief Retry policy */
    lws_retry_bo_t m_retry_policy;
    /** @brief Protocols */
    std::array<struct lws_protocols, 2u> m_protocols;

    /** @brief Connected clients */
    std::map<struct lws*, std::shared_ptr<IClient>> m_clients;

    /** @brief Internal thread */
    void process();

    /** @brief libwebsockets event callback */
    static int eventCallback(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len);
};

} // namespace websockets
} // namespace ocpp

#endif // LIBWEBSOCKETSERVER_H
