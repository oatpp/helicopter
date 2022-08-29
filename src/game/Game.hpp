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

#ifndef Helicopter_game_Game_hpp
#define Helicopter_game_Game_hpp

#include "./Session.hpp"
#include "config/GamesConfig.hpp"

class Game {
private:
  struct State {
    oatpp::Object<GameConfigDto> config;
    std::unordered_map<oatpp::String, std::shared_ptr<Session>> sessions;
    std::mutex mutex;
    bool isPingerActive;
  };
private:
  std::shared_ptr<State> m_state;
private:
  OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, m_asyncExecutor);
private:
  void startPinger();
public:

  /**
   * Constructor.
   * @param config
   */
  Game(const oatpp::Object<GameConfigDto>& config);

  /**
   * Not thread safe.
   * Create new game session.
   * @param sessionId
   * @param config
   * @return - `std::shared_ptr` to a new Session or `nullptr` if session with such ID already exists.
   */
  std::shared_ptr<Session> createNewSession(const oatpp::String& sessionId);

  /**
   * NOT thread-safe
   * @param sessionId
   * @return
   */
  std::shared_ptr<Session> findSession(const oatpp::String& sessionId);

  /**
   * NOT thread-safe
   * @param sessionId
   */
  void deleteSession(const oatpp::String& sessionId);

};

#endif //Helicopter_game_Game_hpp
