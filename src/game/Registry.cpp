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
        return call.next(m_websocket->sendCloseAsync()).next(finish());
      }

      return call.next(finish());

    }

  };

  auto message = MessageDto::createShared();
  message->code = MessageCodes::OUTGOING_ERROR;
  message->payload = error;

  m_asyncExecutor->execute<SendErrorCoroutine>(socket, m_objectMapper->writeToString(message), fatal);

}

oatpp::String Registry::getRequiredParameter(const oatpp::String& name, const std::shared_ptr<const ParameterMap>& params, SessionInfo& sessionInfo) {
  auto it = params->find(name);
  if(it != params->end() && it->second) {
    return it->second;
  }
  sessionInfo.error = ErrorDto::createShared(ErrorCodes::BAD_REQUEST, "Missing required parameter - '" + name + "'.");
  return nullptr;
}

Registry::SessionInfo Registry::getSessionForPeer(
  const std::shared_ptr<AsyncWebSocket>& socket,
  const std::shared_ptr<const ParameterMap>& params)
{

  SessionInfo result;

  auto gameId = getRequiredParameter(Constants::PARAM_GAME_ID, params, result);
  if(result.error) return result;

  auto sessionId = getRequiredParameter(Constants::PARAM_GAME_SESSION_ID, params, result);
  if(result.error) return result;

  auto peerType = getRequiredParameter(Constants::PARAM_PEER_TYPE, params, result);
  if(result.error) return result;

  result.isHost = peerType == Constants::PARAM_PEER_TYPE_HOST;

  auto game = getGameById(gameId);
  if(!game) {
    result.error = ErrorDto::createShared(ErrorCodes::GAME_NOT_FOUND, "Game config not found. Game config should be present on the server.");
    return result;
  }

  if(result.isHost) {
    result.session = game->createNewSession(sessionId);
    if(!result.session) {
      result.error = ErrorDto::createShared(ErrorCodes::OPERATION_NOT_PERMITTED, "Session with such ID already exists. Can't create new session session.");
      return result;
    }
  } else {
    result.session = game->findSession(sessionId);
    if(!result.session) {
      result.error = ErrorDto::createShared(ErrorCodes::SESSION_NOT_FOUND, "No game session found for given sessionId.");
      return result;
    }
  }

  return result;

}

std::shared_ptr<Game> Registry::getGameById(const oatpp::String& gameId) {

  std::lock_guard<std::mutex> lock(m_mutex);

  auto it = m_games.find(gameId);
  if(it != m_games.end()) {
    return it->second;
  }

  auto config = m_gameConfig->getGameConfig(gameId);
  if(config) {
    auto game = std::make_shared<Game>(config);
    m_games.insert({config->gameId, game});
    return game;
  }

  return nullptr;

}

void Registry::onAfterCreate_NonBlocking(const std::shared_ptr<AsyncWebSocket>& socket, const std::shared_ptr<const ParameterMap>& params) {

  OATPP_LOGD("Registry", "socket created - %d", socket.get())

  auto sessionInfo = getSessionForPeer(socket, params);

  if(sessionInfo.error) {
    sendSocketErrorAsync(socket, sessionInfo.error, true);
    return;
  }

  auto peer = std::make_shared<Peer>(
    socket,
    sessionInfo.session,
    sessionInfo.session->generateNewPeerId()
  );

  socket->setListener(peer);

  OATPP_LOGD("Registry", "peer created for socket - %d", socket.get())

  sessionInfo.session->addPeer(peer, sessionInfo.isHost);

}

void Registry::onBeforeDestroy_NonBlocking(const std::shared_ptr<AsyncWebSocket>& socket) {

  OATPP_LOGD("Registry", "destroying socket - %d", socket.get())

  auto peer = std::static_pointer_cast<Peer>(socket->getListener());
  if(peer) {
    peer->invalidateSocket();

    auto session = peer->getGameSession();

    bool isEmptySession;
    session->removePeerById(peer->getPeerId(), isEmptySession);

    if (isEmptySession) {
      auto game = getGameById(session->getConfig()->gameId);
      game->deleteSession(session->getId());
      OATPP_LOGD("Registry", "Session deleted - %d", session.get())
    }

  } else {
    socket->getConnection().invalidate();
  }

}