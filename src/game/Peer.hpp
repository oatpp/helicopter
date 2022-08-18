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

#ifndef Helicopter_game_Peer_hpp
#define Helicopter_game_Peer_hpp

#include "Constants.hpp"

#include "config/Config.hpp"
#include "config/GameConfig.hpp"

#include "dto/DTOs.hpp"

#include "oatpp-websocket/AsyncWebSocket.hpp"

#include "oatpp/network/ConnectionProvider.hpp"

#include "oatpp/core/async/Lock.hpp"
#include "oatpp/core/async/Executor.hpp"
#include "oatpp/core/data/mapping/ObjectMapper.hpp"
#include "oatpp/core/macro/component.hpp"

class Session; // FWD

class Peer : public oatpp::websocket::AsyncWebSocket::Listener {
private:

  struct MessageQueue {
    std::list<oatpp::Object<MessageDto>> queue;
    std::mutex mutex;
    bool active = false;
  };

private:

  /**
   * Buffer for messages. Needed for multi-frame messages.
   */
  oatpp::data::stream::BufferOutputStream m_messageBuffer;

  /**
   * Lock for synchronization of writes to the web socket.
   */
  oatpp::async::Lock m_writeLock;

private:
  std::shared_ptr<AsyncWebSocket> m_socket;
  std::shared_ptr<Session> m_gameSession;
  v_int64 m_peerId;
  bool m_isHost;
private:
  std::shared_ptr<MessageQueue> m_messageQueue;
private:

  /* Inject application components */
  OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, m_asyncExecutor);
  OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, m_objectMapper, Constants::COMPONENT_WS_API);

private:

  CoroutineStarter handleMessage(const oatpp::Object<MessageDto>& message);

public:

  Peer(const std::shared_ptr<AsyncWebSocket>& socket,
       const std::shared_ptr<Session>& gameSession,
       v_int64 peerId,
       bool isHost);

  /**
   * Send message to peer.
   * @param message
   */
  void sendMessageAsync(const oatpp::String& message);

  /**
   * Send error message to peer.
   * @param error
   * @param fatal
   */
  oatpp::async::CoroutineStarter sendErrorAsync(const oatpp::Object<ErrorDto>& error, bool fatal = false);

  /**
   * Queue message to send to peer.
   * @param message
   * @return
   */
  bool queueMessage(const oatpp::Object<MessageDto>& message);

  /**
   * Get the game session the peer associated with.
   * @return
   */
  std::shared_ptr<Session> getGameSession();

  /**
   * Set if peer is the game host.
   * @param isHost
   */
  void setAsHost(bool isHost);

  /**
   * Check if peer is the game host.
   * @return
   */
  bool isHost();

  /**
   * Get peer peerId.
   * @return
   */
  v_int64 getPeerId();

  /**
   * Remove circle `std::shared_ptr` dependencies
   */
  void invalidateSocket();

public: // WebSocket Listener methods

  CoroutineStarter onPing(const std::shared_ptr<AsyncWebSocket>& socket, const oatpp::String& message) override;
  CoroutineStarter onPong(const std::shared_ptr<AsyncWebSocket>& socket, const oatpp::String& message) override;
  CoroutineStarter onClose(const std::shared_ptr<AsyncWebSocket>& socket, v_uint16 code, const oatpp::String& message) override;
  CoroutineStarter readMessage(const std::shared_ptr<AsyncWebSocket>& socket, v_uint8 opcode, p_char8 data, oatpp::v_io_size size) override;

};


#endif //Helicopter_game_Peer_hpp
