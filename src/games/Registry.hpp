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

#ifndef Helicopter_games_Registry_hpp
#define Helicopter_games_Registry_hpp

#include "./Game.hpp"

#include "oatpp-websocket/AsyncConnectionHandler.hpp"

#include <unordered_map>
#include <mutex>


class Registry : public oatpp::websocket::AsyncConnectionHandler::SocketInstanceListener {
public:
  std::unordered_map<oatpp::String, std::shared_ptr<Game>> m_games;
  std::mutex m_gamesMutex;
public:

  Registry();

  std::shared_ptr<Game> createGame(const oatpp::String& gameId);

  std::shared_ptr<Game> getGame(const oatpp::String& gameId);

  void deleteGame(const oatpp::String& gameId);

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


#endif //Helicopter_games_Registry_hpp
