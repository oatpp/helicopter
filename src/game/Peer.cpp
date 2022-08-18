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
#include "Session.hpp"

#include "oatpp/core/utils/ConversionUtils.hpp"

Peer::Peer(const std::shared_ptr<AsyncWebSocket>& socket,
           const std::shared_ptr<Session>& gameSession,
           v_int64 peerId,
           bool isHost)
  : m_socket(socket)
  , m_gameSession(gameSession)
  , m_peerId(peerId)
  , m_isHost(isHost)
  , m_messageQueue(std::make_shared<MessageQueue>())
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

bool Peer::queueMessage(const oatpp::Object<MessageDto>& message) {

  class SendMessageCoroutine : public oatpp::async::Coroutine<SendMessageCoroutine> {
  private:
    std::shared_ptr<oatpp::data::mapping::ObjectMapper> m_mapper;
    oatpp::async::Lock* m_lock;
    std::shared_ptr<AsyncWebSocket> m_websocket;
    std::shared_ptr<MessageQueue> m_queue;
  public:

    SendMessageCoroutine(const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& mapper,
                         oatpp::async::Lock* lock,
                         const std::shared_ptr<AsyncWebSocket>& websocket,
                         const std::shared_ptr<MessageQueue>& queue)
      : m_mapper(mapper)
      , m_lock(lock)
      , m_websocket(websocket)
      , m_queue(queue)
    {}

    Action act() override {

      std::unique_lock<std::mutex> lock(m_queue->mutex);
      if(m_queue->queue.empty()) {
        m_queue->active = false;
        return finish();
      }
      auto msg = m_queue->queue.back();
      m_queue->queue.pop_back();
      lock.unlock();

      auto json = m_mapper->writeToString(msg);
      return oatpp::async::synchronize(m_lock,m_websocket->sendOneFrameTextAsync(json)).next(repeat());

    }

    Action handleError(oatpp::async::Error* error) override {
      return yieldTo(&SendMessageCoroutine::act);
    }

  };

  if(message) {
    std::lock_guard<std::mutex> lock(m_messageQueue->mutex);
    if(m_messageQueue->queue.size() < m_gameSession->getConfig()->maxQueuedMessages) {
      m_messageQueue->queue.push_front(message);
      if (!m_messageQueue->active) {
        m_messageQueue->active = true;
        m_asyncExecutor->execute<SendMessageCoroutine>(m_objectMapper, &m_writeLock, m_socket, m_messageQueue);
      }
      return true;
    }
  }
  return false;
}

std::shared_ptr<Session> Peer::getGameSession() {
  return m_gameSession;
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
    {
      std::lock_guard<std::mutex> lock(m_messageQueue->mutex);
      m_messageQueue->queue.clear();
    }
    m_socket->getConnection().invalidate();
  }
  m_socket.reset();
}

oatpp::async::CoroutineStarter Peer::handleMessage(const oatpp::Object<MessageDto>& message) {
  return nullptr;
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

  if(m_messageBuffer.getCurrentPosition() + size >  m_gameSession->getConfig()->maxMessageSizeBytes) {
    auto err = ErrorDto::createShared(
      ErrorCodes::BAD_MESSAGE,
      "Fatal Error. Serialized message size shouldn't exceed " +
      oatpp::utils::conversion::int64ToStdStr(m_gameSession->getConfig()->maxMessageSizeBytes) + " bytes.");
    return sendErrorAsync(err, true);
  }

  if(size == 0) { // message transfer finished

    auto wholeMessage = m_messageBuffer.toString();
    m_messageBuffer.setCurrentPosition(0);

    oatpp::Object<MessageDto> message;

    try {
      message = m_objectMapper->readFromString<oatpp::Object<MessageDto>>(wholeMessage);
    } catch (const std::runtime_error& e) {
      auto err = ErrorDto::createShared(
        ErrorCodes::BAD_MESSAGE,
        "Fatal Error. Can't parse message.");
      return sendErrorAsync(err, true);
    }

    return handleMessage(message);

  } else if(size > 0) { // message frame received
    m_messageBuffer.writeSimple(data, size);
  }

  return nullptr; // do nothing

}