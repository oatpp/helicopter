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

#ifndef HELICOPTER_GAMECONFIG_HPP
#define HELICOPTER_GAMECONFIG_HPP


#include "oatpp/parser/json/mapping/ObjectMapper.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include <mutex>

#include OATPP_CODEGEN_BEGIN(DTO)

/**
 * Game config
 */
class GameConfigDto : public oatpp::DTO {

  DTO_INIT(GameConfigDto, DTO)

  /**
   * Game ID.
   */
  DTO_FIELD(String, gameId);

  /**
   * Host peer can't change.
   * If host peer disconnects the game is over and all other peers are dropped.
   */
  DTO_FIELD(Boolean, staticHost) = true;

  /**
   * The maximum number of peers connected to game (including host peer).
   */
  DTO_FIELD(UInt32, maxPeers) = 10;

  /**
   * Max size of the received bytes. (the whole MessageDto structure).
   */
  DTO_FIELD(UInt64, maxMessageSizeBytes) = 4 * 1024; // Default - 4Kb

  /**
   * Max number of messages queued for the peer.
   * If exceeded messages are dropped.
   */
  DTO_FIELD(UInt32, maxQueuedMessages) = 10;

  /**
   * How often should server ping client.
   */
  DTO_FIELD(UInt64, pingIntervalMillis) = 5 * 1000; // 5 seconds

  /**
   * A failed ping is ping to which server receives no response in a 'pingIntervalMillis' interval.
   * If number of failed pings for a peer reaches 'maxFailedPings' in a row then peer is dropped.
   */
  DTO_FIELD(UInt64, maxFailedPings) = 2;

};

class GameConfig {
private:
  oatpp::String m_configFile;
  oatpp::parser::json::mapping::ObjectMapper m_mapper;
  oatpp::UnorderedFields<oatpp::Object<GameConfigDto>> m_games;
  std::mutex m_mutex;
public:

  /**
   * Path to config file containing configs for games.
   * @param configFilename
   */
  GameConfig(const oatpp::String& configFilename);

  /**
   * Put game config
   * @param gameId
   * @param config
   */
  void putGameConfig(const oatpp::Object<GameConfigDto>& config);

  /**
   * Get game config
   * @param gameId
   * @return
   */
  oatpp::Object<GameConfigDto> getGameConfig(const oatpp::String& gameId);

  /**
   * Save current state of games config to config file.
   */
  bool save();

};

#include OATPP_CODEGEN_END(DTO)

#endif //HELICOPTER_GAMECONFIG_HPP
