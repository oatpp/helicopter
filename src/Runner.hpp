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

#ifndef HELICOPTER_RUNNER_HPP
#define HELICOPTER_RUNNER_HPP

#include "config/Config.hpp"

#include "oatpp/web/server/HttpRouter.hpp"

#include "oatpp/network/ConnectionHandler.hpp"
#include "oatpp/network/ConnectionProvider.hpp"

#include "oatpp/core/async/Executor.hpp"

class APIServer {
private:
  std::shared_ptr<oatpp::web::server::HttpRouter> m_router;
  std::shared_ptr<oatpp::network::ServerConnectionProvider> m_connectionProvider;
  std::shared_ptr<oatpp::network::ConnectionHandler> m_connectionHandler;
private:
  std::thread m_serverThread;
public:

  APIServer(const oatpp::Object<ServerConfigDto>& config,
            const std::shared_ptr<oatpp::async::Executor>& executor);

  std::shared_ptr<oatpp::web::server::HttpRouter> getRouter();

  void start();

  void join();

};

class Runner {
private:
  std::list<std::shared_ptr<APIServer>> m_servers;
private:
  void assertServerConfig(const oatpp::Object<ServerConfigDto>& config,
                          const oatpp::String& serverName,
                          bool checkTls);
public:

  Runner(const oatpp::Object<ConfigDto>& config,
         const std::shared_ptr<oatpp::async::Executor>& executor);

  void start();

  void join();

};


#endif //HELICOPTER_RUNNER_HPP
