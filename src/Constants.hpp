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

#ifndef HELICOPTER_CONSTANTS_HPP
#define HELICOPTER_CONSTANTS_HPP

class Constants {
public:

  static constexpr const char* COMPONENT_REST_API = "REST_API";
  static constexpr const char* COMPONENT_WS_API = "WS_API";

public:

  static constexpr const char* PARAM_GAME_SESSION_ID = "sessionId";
  static constexpr const char* PARAM_PEER_TYPE = "peerType";
  static constexpr const char* PARAM_PEER_TYPE_HOST = "host";
  static constexpr const char* PARAM_PEER_TYPE_CLIENT = "client";

};

#endif //HELICOPTER_CONSTANTS_HPP
