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

std::shared_ptr<Game> Registry::createGame(const oatpp::String& gameId) {
  std::lock_guard<std::mutex> lock(m_gamesMutex);
  auto it = m_games.find(gameId);
  if(it != m_games.end()) {
    return nullptr; // Won't create new game - such game already exists.
  }
  auto game = std::make_shared<Game>(gameId);
  m_games.insert({gameId, game});
  return game;
}

std::shared_ptr<Game> Registry::getGame(const oatpp::String& gameId) {
  std::lock_guard<std::mutex> lock(m_gamesMutex);
  auto it = m_games.find(gameId);
  if(it != m_games.end()) {
    return it->second;
  }
  return nullptr; // Game not found.
}

void Registry::deleteGame(const oatpp::String& gameId) {
  std::lock_guard<std::mutex> lock(m_gamesMutex);
  m_games.erase(gameId);
}

void Registry::onAfterCreate_NonBlocking(const std::shared_ptr<AsyncWebSocket>& socket, const std::shared_ptr<const ParameterMap>& params) {

  auto gameId = params->find(Constants::PARAM_GAME_ID)->second;
  auto peerType = params->find(Constants::PARAM_PEER_TYPE)->second;

  std::shared_ptr<Game> game;
  bool isHostPeer = false;
  if(peerType == Constants::PARAM_PEER_TYPE_HOST) {
    game = createGame(gameId);
    isHostPeer = true;
  } else if(peerType == Constants::PARAM_PEER_TYPE_CLIENT) {
    game = getGame(gameId);
  }

  auto peer = std::make_shared<Peer>(socket, game, 0, isHostPeer);
  socket->setListener(peer);

}

void Registry::onBeforeDestroy_NonBlocking(const std::shared_ptr<AsyncWebSocket>& socket) {

  auto peer = std::static_pointer_cast<Peer>(socket->getListener());
  auto game = peer->getGame();

  game->removePeerById(peer->getPeerId());
  peer->invalidateSocket();

  if(game->isEmpty()) {
    deleteGame(game->getId());
  }

}