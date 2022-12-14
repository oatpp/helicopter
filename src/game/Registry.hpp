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

#ifndef Helicopter_game_Registry_hpp
#define Helicopter_game_Registry_hpp

#include "./Game.hpp"

#include "oatpp-websocket/AsyncConnectionHandler.hpp"

#include <unordered_map>
#include <mutex>


class Registry : public oatpp::websocket::AsyncConnectionHandler::SocketInstanceListener {
private:

  struct SessionInfo {
    std::shared_ptr<Session> session;
    oatpp::Object<ErrorDto> error;
    bool isHost;
  };

private:
  std::unordered_map<oatpp::String, std::shared_ptr<Game>> m_games;
  std::mutex m_mutex;
private:
  /* Inject application components */
  OATPP_COMPONENT(oatpp::Object<ConfigDto>, m_config);
  OATPP_COMPONENT(std::shared_ptr<GamesConfig>, m_gameConfig);
  OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, m_asyncExecutor);
  OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, m_objectMapper, Constants::COMPONENT_WS_API);
private:
  oatpp::String getRequiredParameter(const oatpp::String& name, const std::shared_ptr<const ParameterMap>& params, SessionInfo& sessionInfo);
private:
  void sendSocketErrorAsync(const std::shared_ptr<AsyncWebSocket>& socket, const oatpp::Object<ErrorDto>& error, bool fatal = false);
  SessionInfo getSessionForPeer(const std::shared_ptr<AsyncWebSocket>& socket, const std::shared_ptr<const ParameterMap>& params);
public:

  Registry();

  /**
   * Get all sessions of the game.
   * @param gameId
   * @return
   */
  std::shared_ptr<Game> getGameById(const oatpp::String& gameId);

public:

  /**
   *  Called when socket is created
   */
  void onAfterCreate_NonBlocking(const std::shared_ptr<AsyncWebSocket>& socket, const std::shared_ptr<const ParameterMap>& params) override;

  /**
   *  Called before socket instance is destroyed.
   */
  void onBeforeDestroy_NonBlocking(const std::shared_ptr<AsyncWebSocket>& socket) override;

};


#endif //Helicopter_game_Registry_hpp
