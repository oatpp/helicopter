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

#include "GamesConfig.hpp"

GamesConfig::GamesConfig(const oatpp::String& configFilename)
  : m_configFile(configFilename)
{
  if(configFilename) {
    auto json = oatpp::String::loadFromFile(configFilename->c_str());
    m_games = m_mapper.readFromString<oatpp::UnorderedFields<oatpp::Object<GameConfigDto>>>(json);
  }
}

void GamesConfig::putGameConfig(const oatpp::Object<GameConfigDto>& config) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if(m_games == nullptr) {
    m_games = oatpp::UnorderedFields<oatpp::Object<GameConfigDto>>({});
  }
  m_games->insert({config->gameId, config});
}

oatpp::Object<GameConfigDto> GamesConfig::getGameConfig(const oatpp::String& gameId) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if(m_games) {
    auto it = m_games->find(gameId);
    if(it != m_games->end()) {
      return it->second;
    }
  }
  return nullptr;
}

bool GamesConfig::save() {
  oatpp::String json;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    json = m_mapper.writeToString(m_games);
  }
  if(m_configFile) {
    json.saveToFile(m_configFile->c_str());
    return true;
  }
  return false;
}
