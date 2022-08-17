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

#ifndef Helicopter_game_Session_hpp
#define Helicopter_game_Session_hpp

#include "Peer.hpp"
#include "config/GameConfig.hpp"

class Session {
private:
  oatpp::String m_id;
  oatpp::Object<GameConfigDto> m_config;
  std::atomic<v_int64> m_peerIdCounter;
  std::unordered_map<v_int64, std::shared_ptr<Peer>> m_peers;
  std::shared_ptr<Peer> m_host;
  std::mutex m_peersMutex;
public:

  Session(const oatpp::String& id, const oatpp::Object<GameConfigDto>& config);

  oatpp::String getId();
  oatpp::Object<GameConfigDto> getConfig();

  void addPeer(const std::shared_ptr<Peer>& peer);
  void setHost(const std::shared_ptr<Peer>& peer);

  void removePeerById(v_int64 peerId);

  v_int64 generateNewPeerId();

  bool isEmpty();

};


#endif //Helicopter_game_Session_hpp
