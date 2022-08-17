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

#include "Session.hpp"

Session::Session(const oatpp::String& id, const oatpp::Object<GameConfigDto>& config)
  : m_id(id)
  , m_config(config)
  , m_peerIdCounter(0)
{}

oatpp::String Session::getId() {
  return m_id;
}

oatpp::Object<GameConfigDto> Session::getConfig() {
  return m_config;
}

void Session::addPeer(const std::shared_ptr<Peer>& peer) {
  std::lock_guard<std::mutex> guard(m_peersMutex);
  m_peers.insert({peer->getPeerId(), peer});
}

void Session::setHost(const std::shared_ptr<Peer>& peer){
  std::lock_guard<std::mutex> guard(m_peersMutex);

  if(m_host) m_host->setAsHost(false);

  m_host = peer;
  peer->setAsHost(true);
}

void Session::removePeerById(v_int64 peerId) {
  std::lock_guard<std::mutex> guard(m_peersMutex);
  if(m_host && m_host->getPeerId() == peerId) m_host.reset();
  m_peers.erase(peerId);
}

v_int64 Session::generateNewPeerId() {
  return m_peerIdCounter ++;
}

bool Session::isEmpty() {
  std::lock_guard<std::mutex> guard(m_peersMutex);
  return m_peers.empty();
}