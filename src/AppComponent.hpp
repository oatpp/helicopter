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
#include "config/GameConfig.hpp"

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

    config->host = std::getenv("EXTERNAL_ADDRESS");
    if (!config->host) {
      config->host = m_cmdArgs.getNamedArgumentValue("--host", "localhost");
    }

    const char* portText = std::getenv("EXTERNAL_PORT");
    if(!portText) {
      portText = m_cmdArgs.getNamedArgumentValue("--port", "8443");
    }

    bool success;
    auto port = oatpp::utils::conversion::strToUInt32(portText, success);
    if(!success || port > 65535) {
      throw std::runtime_error("Invalid port!");
    }
    config->port = (v_uint16) port;

    config->tlsPrivateKeyPath = std::getenv("TLS_FILE_PRIVATE_KEY");
    if(!config->tlsPrivateKeyPath) {
      config->tlsPrivateKeyPath = m_cmdArgs.getNamedArgumentValue("--tls-key", "" CERT_PEM_PATH);
    }

    config->tlsCertificateChainPath = std::getenv("TLS_FILE_CERT_CHAIN");
    if(!config->tlsCertificateChainPath) {
      config->tlsCertificateChainPath = m_cmdArgs.getNamedArgumentValue("--tls-chain", "" CERT_CRT_PATH);
    }

    config->statisticsUrl = std::getenv("URL_STATS_PATH");
    if(!config->statisticsUrl) {
      config->statisticsUrl = m_cmdArgs.getNamedArgumentValue("--url-stats", "admin/stats.json");
    }



    return config;

  }());

  /**
   * Game configs
   */
  OATPP_CREATE_COMPONENT(std::shared_ptr<GameConfig>, gameConfig)([] {
    auto config = std::make_shared<GameConfig>(nullptr);
    auto testGame1 = GameConfigDto::createShared();
    config->putGameConfig("snake", testGame1);
    return config;
  }());

  /**
   * Create Async Executor
   */
  OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::async::Executor>, executor)([] {
    return std::make_shared<oatpp::async::Executor>();
  }());

  /**
   *  Create ConnectionProvider component which listens on the port
   */
  OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, serverConnectionProvider)([] {

    OATPP_COMPONENT(oatpp::Object<ConfigDto>, appConfig);

    std::shared_ptr<oatpp::network::ServerConnectionProvider> result;

    if(appConfig->useTLS) {

      OATPP_LOGD("oatpp::libressl::Config", "key_path='%s'", appConfig->tlsPrivateKeyPath->c_str());
      OATPP_LOGD("oatpp::libressl::Config", "chn_path='%s'", appConfig->tlsCertificateChainPath->c_str());

      auto config = oatpp::openssl::Config::createDefaultServerConfigShared(
              appConfig->tlsCertificateChainPath->c_str(),
              appConfig->tlsPrivateKeyPath->c_str());
      result = oatpp::openssl::server::ConnectionProvider::createShared(config, {"0.0.0.0", appConfig->port, oatpp::network::Address::IP_4});
    } else {
      result = oatpp::network::tcp::server::ConnectionProvider::createShared({"0.0.0.0", appConfig->port, oatpp::network::Address::IP_4});
    }

    return result;

  }());

  /**
   *  Create Router component
   */
  OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, httpRouter)([] {
    return oatpp::web::server::HttpRouter::createShared();
  }());

  /**
   *  Create ConnectionHandler component which uses Router component to route requests
   */
  OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, serverConnectionHandler)(Constants::COMPONENT_REST_API, [] {
    OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router); // get Router component
    OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, executor); // get Async executor component
    return oatpp::web::server::AsyncHttpConnectionHandler::createShared(router, executor);
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
   *  Create ObjectMapper component to serialize/deserialize DTOs in Contoller's API
   */
  OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, wsApiObjectMapper)(Constants::COMPONENT_WS_API,[] {
    auto mapper = oatpp::parser::json::mapping::ObjectMapper::createShared();
    mapper->getSerializer()->getConfig()->includeNullFields = false;
    return mapper;
  }());

  /**
   *  Create chat lobby component.
   */
  OATPP_CREATE_COMPONENT(std::shared_ptr<Registry>, lobby)([] {
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