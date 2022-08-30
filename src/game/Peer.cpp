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
           v_int64 peerId)
  : m_socket(socket)
  , m_gameSession(gameSession)
  , m_peerId(peerId)
  , m_messageQueue(std::make_shared<MessageQueue>())
  , m_pingTime(-1)
  , m_failedPings(0)
  , m_lastPingTimestamp(-1)
{}

oatpp::async::CoroutineStarter Peer::sendMessageAsync(const oatpp::Object<MessageDto>& message) {

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

  std::lock_guard<std::mutex> socketLock(m_socketMutex);
  if (m_socket) {
    return SendMessageCoroutine::start(&m_writeLock, m_socket, m_objectMapper->writeToString(message));
  }

  return nullptr;

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
               .next(m_websocket->sendCloseAsync()).next(finish());
               //.next(new oatpp::async::Error("API Error"));
      }

      return call.next(finish());

    }

  };

  auto message = MessageDto::createShared();
  message->code = MessageCodes::OUTGOING_ERROR;
  message->payload = error;

  std::lock_guard<std::mutex> socketLock(m_socketMutex);
  if (m_socket) {
    return SendErrorCoroutine::start(&m_writeLock, m_socket, m_objectMapper->writeToString(message), fatal);
  }

  return nullptr;

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
        std::lock_guard<std::mutex> socketLock(m_socketMutex);
        if (m_socket) {
          m_asyncExecutor->execute<SendMessageCoroutine>(m_objectMapper, &m_writeLock, m_socket, m_messageQueue);
        }
      }
      return true;
    }
  }
  return false;
}

void Peer::ping(v_int64 timestampMicroseconds) {

  class PingCoroutine : public oatpp::async::Coroutine<PingCoroutine> {
  private:
    oatpp::async::Lock* m_lock;
    std::shared_ptr<AsyncWebSocket> m_websocket;
    oatpp::String m_message;
  public:

    PingCoroutine(oatpp::async::Lock* lock,
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

  auto message = MessageDto::createShared(MessageCodes::OUTGOING_PING, oatpp::Int64(timestampMicroseconds));

  std::lock_guard<std::mutex> socketLock(m_socketMutex);
  if (m_socket) {
    m_asyncExecutor->execute<PingCoroutine>(&m_writeLock, m_socket, m_objectMapper->writeToString(message));
  }

}

void Peer::kick() {

  class KickCoroutine : public oatpp::async::Coroutine<KickCoroutine> {
  private:
    oatpp::async::Lock* m_lock;
    std::shared_ptr<AsyncWebSocket> m_websocket;
    oatpp::String m_message;
  public:

    KickCoroutine(oatpp::async::Lock* lock,
                  const std::shared_ptr<AsyncWebSocket>& websocket,
                  const oatpp::String& message)
      : m_lock(lock)
      , m_websocket(websocket)
      , m_message(message)
    {}

    Action act() override {
      return oatpp::async::synchronize(m_lock, m_websocket->sendOneFrameTextAsync(m_message))
        .next(yieldTo(&KickCoroutine::onMessageSent));
    }

    Action onMessageSent() {
      m_websocket->getConnection().invalidate();
      return finish();
    }

  };

  auto message = MessageDto::createShared(MessageCodes::OUTGOING_CLIENT_KICKED, oatpp::String("you were kicked."));

  std::lock_guard<std::mutex> socketLock(m_socketMutex);
  if (m_socket) {
    m_asyncExecutor->execute<KickCoroutine>(&m_writeLock, m_socket, m_objectMapper->writeToString(message));
  }
}

std::shared_ptr<Session> Peer::getGameSession() {
  return m_gameSession;
}

v_int64 Peer::getPeerId() {
  return m_peerId;
}

void Peer::invalidateSocket() {
  {
    std::lock_guard<std::mutex> socketLock(m_socketMutex);
    if (m_socket) {
      m_socket->getConnection().invalidate();
      m_socket.reset();
    }
  }

  {
    std::lock_guard<std::mutex> lock(m_messageQueue->mutex);
    m_messageQueue->queue.clear();
  }
}

void Peer::checkPingsRules(const v_int64 currentPingSessionTimestamp) {

  std::lock_guard<std::mutex> pingLock(m_pingMutex);

  if(m_lastPingTimestamp != currentPingSessionTimestamp) {
    m_failedPings ++;
  }

  OATPP_LOGD("Peer", "failed pings=%d", m_failedPings)

  if (m_failedPings >= m_gameSession->getConfig()->maxFailedPings) {
    OATPP_LOGD("Peer", "maxFailedPings exceeded. PeerId=%lld. Peer dropped.", m_peerId);
    invalidateSocket();
  }

}

oatpp::async::CoroutineStarter Peer::handlePong(const oatpp::Object<MessageDto>& message) {

  auto timestamp = message->payload.retrieve<oatpp::Int64>();

  if(!timestamp) {
    return sendErrorAsync(ErrorDto::createShared(ErrorCodes::BAD_MESSAGE, "Message MUST contain 'payload.'"));
  }

  v_int64 pt = m_gameSession->reportPeerPong(m_peerId, timestamp);

  {
    std::lock_guard<std::mutex> pingLock(m_pingMutex);
    m_pingTime = pt;
    if (m_pingTime >= 0) {
      m_failedPings = 0;
      m_lastPingTimestamp = timestamp;
    }
  }

  return nullptr;
}

oatpp::async::CoroutineStarter Peer::handleBroadcast(const oatpp::Object<MessageDto>& message) {

  auto peers = m_gameSession->getAllPeers();

  for(auto peer : peers) {

    if(peer->getPeerId() != m_peerId) {
      auto payload = OutgoingMessageDto::createShared();
      payload->peerId = m_peerId;
      payload->data = message->payload.retrieve<oatpp::String>();

      peer->queueMessage(MessageDto::createShared(MessageCodes::OUTGOING_MESSAGE, payload));
    }

  }

  return nullptr;

}

oatpp::async::CoroutineStarter Peer::handleDirectMessage(const oatpp::Object<MessageDto>& message) {

  auto dm = message->payload.retrieve<oatpp::Object<DirectMessageDto>>();

  if(!dm) {
    return sendErrorAsync(ErrorDto::createShared(ErrorCodes::BAD_MESSAGE, "Message MUST contain 'payload.'"));
  }

  if(!dm->peerIds || dm->peerIds->empty()) {
    return sendErrorAsync(ErrorDto::createShared(ErrorCodes::BAD_MESSAGE, "Payload MUST contain array of peerIds of recipients."));
  }

  auto peers = m_gameSession->getPeers(dm->peerIds);

  for(auto peer : peers) {

    if(peer->getPeerId() != m_peerId) {
      auto payload = OutgoingMessageDto::createShared();
      payload->peerId = m_peerId;
      payload->data = dm->data;

      peer->queueMessage(MessageDto::createShared(MessageCodes::OUTGOING_MESSAGE, payload));
    }

  }

  return nullptr;

}

oatpp::async::CoroutineStarter Peer::handleSynchronizedEvent(const oatpp::Object<MessageDto>& message) {
  m_gameSession->broadcastSynchronizedEvent(m_peerId, message->payload.retrieve<oatpp::String>());
  return nullptr;
}

oatpp::async::CoroutineStarter Peer::handleKickMessage(const oatpp::Object<MessageDto>& message) {

  auto host = m_gameSession->getHost();
  if(host == nullptr) {
    return sendErrorAsync(ErrorDto::createShared(ErrorCodes::INVALID_STATE, "There is no game host."));
  }

  if(host->getPeerId() != m_peerId) {
    return sendErrorAsync(ErrorDto::createShared(ErrorCodes::OPERATION_NOT_PERMITTED, "Only Host peer can kick others."));
  }

  auto ids = message->payload.retrieve<oatpp::Vector<oatpp::Int64>>();

  if(!ids || ids->empty()) {
    return sendErrorAsync(ErrorDto::createShared(ErrorCodes::BAD_MESSAGE, "Payload MUST contain array of peerIds to kick from session.'"));
  }

  auto peers = m_gameSession->getPeers(ids);

  for(auto peer : peers) {

    if(peer->getPeerId() != m_peerId) {
      peer->kick();
    }

  }

  return nullptr;

}

oatpp::async::CoroutineStarter Peer::handleClientMessage(const oatpp::Object<MessageDto>& message) {

  auto host = m_gameSession->getHost();
  if(host == nullptr) {
    return sendErrorAsync(ErrorDto::createShared(ErrorCodes::INVALID_STATE, "There is no game host. No one will receive this message."));
  }

  if(host->getPeerId() == m_peerId) {
    return sendErrorAsync(ErrorDto::createShared(ErrorCodes::OPERATION_NOT_PERMITTED, "Host can't send message to itself."));
  }

  auto payload = OutgoingMessageDto::createShared();
  payload->peerId = m_peerId;
  payload->data = message->payload.retrieve<oatpp::String>();

  host->queueMessage(MessageDto::createShared(MessageCodes::OUTGOING_MESSAGE, payload));

  return nullptr;

}

oatpp::async::CoroutineStarter Peer::handleMessage(const oatpp::Object<MessageDto>& message) {

  if(!message->code) {
    return sendErrorAsync(ErrorDto::createShared(ErrorCodes::BAD_MESSAGE, "Message MUST contain 'code' field."));
  }

  switch (*message->code) {

    case MessageCodes::INCOMING_PONG: return handlePong(message);
    case MessageCodes::INCOMING_BROADCAST: return handleBroadcast(message);
    case MessageCodes::INCOMING_DIRECT_MESSAGE: return handleDirectMessage(message);
    case MessageCodes::INCOMING_SYNCHRONIZED_EVENT: return handleSynchronizedEvent(message);
    case MessageCodes::INCOMING_HOST_KICK_CLIENTS: return handleKickMessage(message);
    case MessageCodes::INCOMING_CLIENT_MESSAGE: return handleClientMessage(message);

    default:
      return sendErrorAsync(ErrorDto::createShared(ErrorCodes::OPERATION_NOT_PERMITTED, "Invalid operation code."));

  }
  
  return nullptr;

}

oatpp::async::CoroutineStarter Peer::onPing(const std::shared_ptr<AsyncWebSocket>& socket, const oatpp::String& message) {
  return oatpp::async::synchronize(&m_writeLock, socket->sendPongAsync(message));
}

oatpp::async::CoroutineStarter Peer::onPong(const std::shared_ptr<AsyncWebSocket>& socket, const oatpp::String& message) {
  return nullptr; // do nothing
}

oatpp::async::CoroutineStarter Peer::onClose(const std::shared_ptr<AsyncWebSocket>& socket, v_uint16 code, const oatpp::String& message) {
  OATPP_LOGD("Peer", "onClose received.")
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