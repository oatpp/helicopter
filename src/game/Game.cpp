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

#include "Game.hpp"

Game::Game(const oatpp::Object<GameConfigDto>& config)
  : m_config(config)
{

}

std::shared_ptr<Session> Game::createNewSession(const oatpp::String& sessionId) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_sessions.find(sessionId);
  if(it != m_sessions.end()) {
    return nullptr;
  }
  auto session = std::make_shared<Session>(sessionId, m_config);
  m_sessions.insert({sessionId, session});
  return session;
}

std::shared_ptr<Session> Game::findSession(const oatpp::String& sessionId) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_sessions.find(sessionId);
  if(it != m_sessions.end()) {
    return it->second;
  }
  return nullptr; // Session not found.
}

void Game::deleteSession(const oatpp::String& sessionId) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_sessions.erase(sessionId);
}
