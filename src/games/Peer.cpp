/***************************************************************************
 *
 * Project:               _ _                 _
 *              /\  /\___| (_) ___ ___  _ __ | |_ ___ _ __
 *             / /_/ / _ \ | |/ __/ _ \| '_ \| __/ _ \ '__|
 *            / __  /  __/ | | (_| (_) | |_) | ||  __/ |
 *            \/ /_/ \___|_|_|\___\___/| .__/ \__\___|_|
 *                                     |_|
 *
 *
 * Copyright 2022-present, Leonid Stryzhevskyi <lganzzzo@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************************/

#include "Peer.hpp"
#include "Game.hpp"

#include "oatpp/network/tcp/Connection.hpp"
#include "oatpp/encoding/Base64.hpp"

Peer::Peer(const std::shared_ptr<AsyncWebSocket>& socket,
           const std::shared_ptr<Game>& game,
           v_int64 peerId,
           bool isHost)
  : m_socket(socket)
  , m_game(game)
  , m_peerId(peerId)
  , m_isHost(isHost)
{}

void Peer::sendMessageAsync(const oatpp::String& message) {

  class SendMessageCoroutine : public oatpp::async::Coroutine<SendMessageCoroutine> {
  private:
    oatpp::async::Lock* m_lock;
    std::shared_ptr<AsyncWebSocket> m_websocket;
    oatpp::String m_message;
  public:

    SendMessageCoroutine(oatpp::async::Lock* lock,
                         const std::shared_ptr<AsyncWebSocket>& websocket,
                         const oatpp::String& message)
      : m_lock(lock)
      , m_websocket(websocket)
      , m_message(message)
    {}

    Action act() override {
      return oatpp::async::synchronize(m_lock, m_websocket->sendOneFrameTextAsync(m_message)).next(finish());
    }

  };

  if(m_socket) {
    m_asyncExecutor->execute<SendMessageCoroutine>(&m_writeLock, m_socket, message);
  }

}

oatpp::async::CoroutineStarter Peer::sendErrorAsync(const oatpp::Object<ErrorDto>& error, bool fatal) {

  class SendErrorCoroutine : public oatpp::async::Coroutine<SendErrorCoroutine> {
  private:
    oatpp::async::Lock* m_lock;
    std::shared_ptr<AsyncWebSocket> m_websocket;
    oatpp::String m_message;
    bool m_fatal;
  public:

    SendErrorCoroutine(oatpp::async::Lock* lock,
                       const std::shared_ptr<AsyncWebSocket>& websocket,
                       const oatpp::String& message,
                       bool fatal)
            : m_lock(lock)
            , m_websocket(websocket)
            , m_message(message)
            , m_fatal(fatal)
    {}

    Action act() override {

      /* synchronized async pipeline */
      auto call = oatpp::async::synchronize(m_lock,m_websocket->sendOneFrameTextAsync(m_message));

      if(m_fatal) {
        return call
               .next(m_websocket->sendCloseAsync())
               .next(new oatpp::async::Error("API Error"));
      }

      return call.next(finish());

    }

  };

  auto message = MessageDto::createShared();
  message->code = MessageCodes::OUTGOING_ERROR;
  message->payload = error;

  return SendErrorCoroutine::start(&m_writeLock, m_socket, m_objectMapper->writeToString(message), fatal);

}

std::shared_ptr<Game> Peer::getGame() {
  return m_game;
}

v_int64 Peer::getPeerId() {
  return m_peerId;
}

void Peer::setAsHost(bool isHost) {
  m_isHost = isHost;
}

bool Peer::isHost() {
  return m_isHost;
}

void Peer::invalidateSocket() {
  if(m_socket) {
    m_socket->getConnection().invalidate();
  }
  m_socket.reset();
}

oatpp::async::CoroutineStarter Peer::onPing(const std::shared_ptr<AsyncWebSocket>& socket, const oatpp::String& message) {
  return oatpp::async::synchronize(&m_writeLock, socket->sendPongAsync(message));
}

oatpp::async::CoroutineStarter Peer::onPong(const std::shared_ptr<AsyncWebSocket>& socket, const oatpp::String& message) {
  return nullptr; // do nothing
}

oatpp::async::CoroutineStarter Peer::onClose(const std::shared_ptr<AsyncWebSocket>& socket, v_uint16 code, const oatpp::String& message) {
  return nullptr; // do nothing
}

oatpp::async::CoroutineStarter Peer::readMessage(const std::shared_ptr<AsyncWebSocket>& socket, v_uint8 opcode, p_char8 data, oatpp::v_io_size size) {

  if(size == 0) { // message transfer finished

    auto wholeMessage = m_messageBuffer.toString();
    m_messageBuffer.setCurrentPosition(0);
    auto msg = m_messageBuffer.toString();

    OATPP_LOGD("Peer", "message='%s'", msg->c_str())

    return nullptr;

  } else if(size > 0) { // message frame received
    m_messageBuffer.writeSimple(data, size);
  }

  return nullptr; // do nothing

}