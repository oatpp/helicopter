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
  : m_state(std::make_shared<State>())
{
  m_state->config = config;
  m_state->isPingerActive = false;
}

void Game::startPinger() {

  class Pinger : public oatpp::async::Coroutine<Pinger> {
  private:
    std::shared_ptr<State> m_state;
  public:

    Pinger(const std::shared_ptr<State>& state)
      : m_state(state)
    {}

    Action act() override {

      std::lock_guard<std::mutex> lock(m_state->mutex);

      if(m_state->sessions.empty()) {
        m_state->isPingerActive = false;
        OATPP_LOGD("Pinger", "Stopped")
        return finish();
      }

      for(auto& session : m_state->sessions) {
        session.second->checkAllPeersPings();
      }

      for(auto& session : m_state->sessions) {
        session.second->pingAllPeers();
      }

      return waitRepeat(std::chrono::milliseconds(m_state->config->pingIntervalMillis));

    }

  };

  if(!m_state->isPingerActive) {
    OATPP_LOGD("Pinger", "Started")
    m_state->isPingerActive = true;
    m_asyncExecutor->execute<Pinger>(m_state);
  }
}

std::shared_ptr<Session> Game::createNewSession(const oatpp::String& sessionId) {

  std::lock_guard<std::mutex> lock(m_state->mutex);

  auto it = m_state->sessions.find(sessionId);
  if(it != m_state->sessions.end()) {
    return nullptr;
  }

  auto session = std::make_shared<Session>(sessionId, m_state->config);
  m_state->sessions.insert({sessionId, session});

  startPinger();

  return session;
}

std::shared_ptr<Session> Game::findSession(const oatpp::String& sessionId) {
  std::lock_guard<std::mutex> lock(m_state->mutex);
  auto it = m_state->sessions.find(sessionId);
  if(it != m_state->sessions.end()) {
    return it->second;
  }
  return nullptr; // Session not found.
}

void Game::deleteSession(const oatpp::String& sessionId) {
  std::lock_guard<std::mutex> lock(m_state->mutex);
  m_state->sessions.erase(sessionId);
}
