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

#include "controller/HostController.hpp"
#include "controller/ClientController.hpp"

#include "./AppComponent.hpp"

#include "oatpp/network/Server.hpp"

#include <iostream>

void run(const oatpp::base::CommandLineArguments& args) {

  /* Register Components in scope of run() method */
  AppComponent components(args);

  /* Get router component */
  OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);

  /* Create HostController and add all of its endpoints to router */
  router->addController(std::make_shared<HostController>());
  router->addController(std::make_shared<ClientController>());

  /* Get connection handler component */
  OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, connectionHandler, Constants::COMPONENT_REST_API);

  /* Get connection provider component */
  OATPP_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, connectionProvider);

  /* Create server which takes provided TCP connections and passes them to HTTP connection handler */
  oatpp::network::Server server(connectionProvider, connectionHandler);

  std::thread serverThread([&server]{
    server.run();
  });

  OATPP_COMPONENT(oatpp::Object<ConfigDto>, appConfig);

  if(appConfig->useTLS) {
    OATPP_LOGI("canchat", "clients are expected to connect at https://%s:%d/", appConfig->host->c_str(), *appConfig->port);
  } else {
    OATPP_LOGI("canchat", "clients are expected to connect at http://%s:%d/", appConfig->host->c_str(), *appConfig->port);
  }

  OATPP_LOGI("canchat", "canonical base URL=%s", appConfig->getCanonicalBaseUrl()->c_str());
  OATPP_LOGI("canchat", "statistics URL=%s", appConfig->getStatsUrl()->c_str());

  serverThread.join();

}

int main(int argc, const char * argv[]) {

  oatpp::base::Environment::init();

  run(oatpp::base::CommandLineArguments(argc, argv));

  oatpp::base::Environment::destroy();

  return 0;
}