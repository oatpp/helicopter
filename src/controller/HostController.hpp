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

#ifndef Helicopter_controller_HostController_hpp
#define Helicopter_controller_HostController_hpp

#include "Constants.hpp"

#include "oatpp-websocket/Handshaker.hpp"

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/network/ConnectionHandler.hpp"

#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"


#include OATPP_CODEGEN_BEGIN(ApiController) /// <-- Begin Code-Gen

class HostController : public oatpp::web::server::api::ApiController {
private:
  typedef HostController __ControllerType;
private:
  OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, websocketConnectionHandler, Constants::COMPONENT_WS_API);
public:
  HostController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper, Constants::COMPONENT_REST_API))
    : oatpp::web::server::api::ApiController(objectMapper)
  {}
public:

  /**
   * Create new host
   */
  ENDPOINT_ASYNC("GET", "api/create-game/*", WS) {

    ENDPOINT_ASYNC_INIT(WS)

    Action act() override {

      auto sessionId = request->getQueryParameter(Constants::PARAM_GAME_SESSION_ID);
      OATPP_ASSERT_HTTP(sessionId, Status::CODE_400, oatpp::String("Please specify '") + Constants::PARAM_GAME_SESSION_ID +  "' query parameter")

      /* Websocket handshake */
      auto response = oatpp::websocket::Handshaker::serversideHandshake(request->getHeaders(), controller->websocketConnectionHandler);
      auto parameters = std::make_shared<oatpp::network::ConnectionHandler::ParameterMap>();

      (*parameters)[Constants::PARAM_PEER_TYPE] = Constants::PARAM_PEER_TYPE_HOST;
      (*parameters)[Constants::PARAM_GAME_SESSION_ID] = sessionId;

      /* Set connection upgrade params */
      response->setConnectionUpgradeParameters(parameters);

      return _return(response);

    }

  };

};

#include OATPP_CODEGEN_END(ApiController) /// <-- End Code-Gen

#endif /* Helicopter_controller_HostController_hpp */
