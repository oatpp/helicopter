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

#ifndef AppComponent_hpp
#define AppComponent_hpp

#include "config/Config.hpp"
#include "config/GamesConfig.hpp"

#include "game/Registry.hpp"

#include "oatpp-openssl/server/ConnectionProvider.hpp"
#include "oatpp-websocket/AsyncConnectionHandler.hpp"

#include "oatpp/web/server/interceptor/RequestInterceptor.hpp"
#include "oatpp/web/server/AsyncHttpConnectionHandler.hpp"
#include "oatpp/web/server/HttpRouter.hpp"

#include "oatpp/network/tcp/server/ConnectionProvider.hpp"

#include "oatpp/parser/json/mapping/ObjectMapper.hpp"

#include "oatpp/core/macro/component.hpp"
#include "oatpp/core/base/CommandLineArguments.hpp"

#include "oatpp/core/utils/ConversionUtils.hpp"

#include <cstdlib>

/**
 *  Class which creates and holds Application components and registers components in oatpp::base::Environment
 *  Order of components initialization is from top to bottom
 */
class AppComponent {
private:
  oatpp::base::CommandLineArguments m_cmdArgs;
public:
  AppComponent(const oatpp::base::CommandLineArguments& cmdArgs)
    : m_cmdArgs(cmdArgs)
  {}
public:

  /**
   * Create config component
   */
  OATPP_CREATE_COMPONENT(oatpp::Object<ConfigDto>, appConfig)([this] {
    auto config = ConfigDto::createShared();

    auto hostServer = ServerConfigDto::createShared();
    hostServer->host = "0.0.0.0";
    hostServer->port = 8000;

    auto clientServer = ServerConfigDto::createShared();
    clientServer->host = "0.0.0.0";
    clientServer->port = 8001;

    config->hostAPIServer = hostServer;
    config->clientAPIServer = clientServer;

    return config;
  }());

  /**
   * Game configs
   */
  OATPP_CREATE_COMPONENT(std::shared_ptr<GamesConfig>, gameConfig)([] {
    auto config = std::make_shared<GamesConfig>(nullptr);
    auto testGame1 = GameConfigDto::createShared();
    testGame1->gameId = "snake";
    config->putGameConfig(testGame1);
    return config;
  }());

  /**
   * Create Async Executor
   */
  OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::async::Executor>, executor)([] {
    return std::make_shared<oatpp::async::Executor>();
  }());

  /**
   *  Create Router component
   */
  OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, httpRouter)([] {
    return oatpp::web::server::HttpRouter::createShared();
  }());

  /**
   *  Create ObjectMapper component to serialize/deserialize DTOs in Contoller's API
   */
  OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, apiObjectMapper)(Constants::COMPONENT_REST_API,[] {
    auto mapper = oatpp::parser::json::mapping::ObjectMapper::createShared();
    mapper->getSerializer()->getConfig()->includeNullFields = false;
    return mapper;
  }());

  /**
   *  Create ObjectMapper component to serialize/deserialize DTOs in WS communication
   */
  OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, wsApiObjectMapper)(Constants::COMPONENT_WS_API,[] {
    auto mapper = oatpp::parser::json::mapping::ObjectMapper::createShared();
    mapper->getSerializer()->getConfig()->includeNullFields = false;
    return mapper;
  }());

  /**
   *  Create games sessions Registry component.
   */
  OATPP_CREATE_COMPONENT(std::shared_ptr<Registry>, gamesSessionsRegistry)([] {
    return std::make_shared<Registry>();
  }());

  /**
   *  Create websocket connection handler
   */
  OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, websocketConnectionHandler)(Constants::COMPONENT_WS_API, [] {
    OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, executor);
    OATPP_COMPONENT(std::shared_ptr<Registry>, registry);
    auto connectionHandler = oatpp::websocket::AsyncConnectionHandler::createShared(executor);
    connectionHandler->setSocketInstanceListener(registry);
    return connectionHandler;
  }());

};

#endif /* AppComponent_hpp */