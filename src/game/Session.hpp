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
#include "config/GamesConfig.hpp"

class Session {
private:
  oatpp::String m_id;
  oatpp::Object<GameConfigDto> m_config;
  std::atomic<v_int64> m_peerIdCounter;
  std::unordered_map<v_int64, std::shared_ptr<Peer>> m_peers;
  std::shared_ptr<Peer> m_host;
  std::mutex m_peersMutex;
private:
  v_int64 m_pingCurrentTimestamp;
  v_int64 m_pingBestTime;
  v_int64 m_pingBestPeerId;
  v_int64 m_pingBestPeerSinceTimestamp;
  std::mutex m_pingMutex;
public:

  Session(const oatpp::String& id, const oatpp::Object<GameConfigDto>& config);

  oatpp::String getId();
  oatpp::Object<GameConfigDto> getConfig();

  void addPeer(const std::shared_ptr<Peer>& peer, bool isHost = false);
  void setHost(const std::shared_ptr<Peer>& peer);
  std::shared_ptr<Peer> getHost();

  bool isHostPeer(v_int64 peerId);

  void removePeerById(v_int64 peerId, bool& isEmpty);

  std::vector<std::shared_ptr<Peer>> getAllPeers();
  std::vector<std::shared_ptr<Peer>> getPeers(const oatpp::Vector<oatpp::Int64>& peerIds);

  v_int64 generateNewPeerId();

  void checkAllPeersPings();

  void pingAllPeers();

  /**
   * Report pong from peer.
   * @param peerId
   * @param timestamp - timestamp reported in the pong (payload). If timestamp doesn't equal to the latest ping
   * timestamp - ping considered to be failed.
   * @return - peer's ping in microseconds or `-1` if ping failed.
   */
  v_int64 reportPeerPong(v_int64 peerId, v_int64 timestamp);

};


#endif //Helicopter_game_Session_hpp
