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
 * TLS config.
 */
class TLSConfigDto : public oatpp::DTO {

  DTO_INIT(TLSConfigDto, DTO)

  /**
   * Path to private key file.
   */
  DTO_FIELD(String, pkFile);

  /**
   * Path to full chain file.
   */
  DTO_FIELD(String, chainFile);

};

/**
 * Config where to serve controller's endpoints.
 */
class ServerConfigDto : public oatpp::DTO {

  DTO_INIT(ServerConfigDto, DTO)

  /**
   * Host
   */
  DTO_FIELD(String, host);

  /**
   * Port
   */
  DTO_FIELD(UInt16, port);

  /**
   * TLS config. If null - do not use TLS.
   */
  DTO_FIELD(Object<TLSConfigDto>, tls);

};

class ConfigDto : public oatpp::DTO {
public:

  DTO_INIT(ConfigDto, DTO)

  /**
   * Config for Host API Server (create game functionality).
   */
  DTO_FIELD(Object<ServerConfigDto>, hostAPIServer);

  /**
   * Config for Client API Server (join game functionality).
   */
  DTO_FIELD(Object<ServerConfigDto>, clientAPIServer);

  /**
   * Path to games config file.
   */
  DTO_FIELD(String, gamesConfigFile);

};

#include OATPP_CODEGEN_END(DTO)

#endif // Config_hpp
