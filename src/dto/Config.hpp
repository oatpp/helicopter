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

#ifndef Config_hpp
#define Config_hpp

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include "oatpp/core/data/stream/BufferStream.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

/**
 * Game config
 */
class GameConfigDto : public oatpp::DTO {

  DTO_INIT(GameConfigDto, DTO)

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
  DTO_FIELD(UInt64, maxMessageSizeBytes) = 8 * 1024; // Default - 8Kb

};

class ConfigDto : public oatpp::DTO {
public:

  DTO_INIT(ConfigDto, DTO)

  DTO_FIELD(String, statisticsUrl);

  DTO_FIELD(String, host);
  DTO_FIELD(UInt16, port);
  DTO_FIELD(Boolean, useTLS) = false;

  /**
   * Path to TLS private key file.
   */
  DTO_FIELD(String, tlsPrivateKeyPath);

  /**
   * Path to TLS certificate chain file.
   */
  DTO_FIELD(String, tlsCertificateChainPath);

  /**
   * Game configs
   */
  DTO_FIELD(UnorderedFields<Object<GameConfigDto>>, games);

public:

  oatpp::String getHostString() {
    oatpp::data::stream::BufferOutputStream stream(256);
    v_uint16 defPort;
    if(useTLS) {
      defPort = 443;
    } else {
      defPort = 80;
    }
    stream << host;
    if(!port || defPort != port) {
      stream << ":" << port;
    }
    return stream.toString();
  }

  oatpp::String getCanonicalBaseUrl() {
    oatpp::data::stream::BufferOutputStream stream(256);
    v_uint16 defPort;
    if(useTLS) {
      stream << "https://";
      defPort = 443;
    } else {
      stream << "http://";
      defPort = 80;
    }
    stream << host;
    if(!port || defPort != port) {
      stream << ":" << port;
    }
    return stream.toString();
  }

  oatpp::String getWebsocketBaseUrl() {
    oatpp::data::stream::BufferOutputStream stream(256);
    if(useTLS) {
      stream << "wss://";
    } else {
      stream << "ws://";
    }
    stream << host << ":" << port;
    return stream.toString();
  }

  oatpp::String getStatsUrl() {
    return getCanonicalBaseUrl() + "/" + statisticsUrl;
  }

};

#include OATPP_CODEGEN_END(DTO)

#endif // Config_hpp
