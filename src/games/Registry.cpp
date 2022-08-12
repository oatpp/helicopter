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

#include "Registry.hpp"

#include "Constants.hpp"

Registry::Registry()
{}

void Registry::sendSocketErrorAsync(const std::shared_ptr<AsyncWebSocket>& socket, const oatpp::Object<ErrorDto>& error, bool fatal) {

  class SendErrorCoroutine : public oatpp::async::Coroutine<SendErrorCoroutine> {
  private:
    std::shared_ptr<AsyncWebSocket> m_websocket;
    oatpp::String m_message;
    bool m_fatal;
  public:

    SendErrorCoroutine(const std::shared_ptr<AsyncWebSocket>& websocket,
                       const oatpp::String& message,
                       bool fatal)
            : m_websocket(websocket)
            , m_message(message)
            , m_fatal(fatal)
    {}

    Action act() override {

      /* synchronized async pipeline */
      auto call = m_websocket->sendOneFrameTextAsync(m_message);

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

  m_asyncExecutor->execute<SendErrorCoroutine>(socket, m_objectMapper->writeToString(message), fatal);

}

std::shared_ptr<Game> Registry::createGame(const oatpp::String& gameId) {
  auto it = m_games.find(gameId);
  if(it != m_games.end()) {
    throw std::runtime_error("Game with such ID already exists. Can't create new game.");
  }
  auto game = std::make_shared<Game>(gameId);
  m_games.insert({gameId, game});
  return game;
}

std::shared_ptr<Game> Registry::getGame(const oatpp::String& gameId) {
  auto it = m_games.find(gameId);
  if(it != m_games.end()) {
    return it->second;
  }
  return nullptr; // Game not found.
}

void Registry::deleteGame(const oatpp::String& gameId) {
  m_games.erase(gameId);
}

std::mutex& Registry::getRegistryMutex() {
  return m_mutex;
}

void Registry::onAfterCreate_NonBlocking(const std::shared_ptr<AsyncWebSocket>& socket, const std::shared_ptr<const ParameterMap>& params) {

  OATPP_LOGD("Registry", "socket created - %d", socket.get())

  auto gameId = params->find(Constants::PARAM_GAME_ID)->second;
  auto peerType = params->find(Constants::PARAM_PEER_TYPE)->second;

  bool isHostPeer = peerType == Constants::PARAM_PEER_TYPE_HOST;
  std::shared_ptr<Game> game;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    game = getGame(gameId);
    if(isHostPeer) {
      if(game) {
        sendSocketErrorAsync(socket,ErrorDto::createShared(ErrorCodes::OPERATION_NOT_PERMITTED, "Game with such ID already exists. Can't create game."),true);
        return;
      } else {
        game = createGame(gameId);
        OATPP_LOGD("Registry", "Game created - %d", game.get())
      }
    }
  }

  auto peer = std::make_shared<Peer>(socket, game, 0, isHostPeer);
  socket->setListener(peer);

  game->addPeer(peer);
  if(isHostPeer) game->setHost(peer);

  OATPP_LOGD("Registry", "peer created for socket - %d", socket.get())

}

void Registry::onBeforeDestroy_NonBlocking(const std::shared_ptr<AsyncWebSocket>& socket) {

  OATPP_LOGD("Registry", "destroying socket - %d", socket.get())

  auto peer = std::static_pointer_cast<Peer>(socket->getListener());
  if(peer) {

    auto game = peer->getGame();

    game->removePeerById(peer->getPeerId());
    peer->invalidateSocket();

    if (game->isEmpty()) {
      std::lock_guard<std::mutex> lock(m_mutex);
      deleteGame(game->getId());
      OATPP_LOGD("Registry", "Game deleted - %d", game.get())
    }
  } else {
    socket->getConnection().invalidate();
  }

}