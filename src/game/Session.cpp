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

void Session::addPeer(const std::shared_ptr<Peer>& peer, bool isHost) {

  {
    std::lock_guard<std::mutex> lock(m_peersMutex);
    m_peers.insert({peer->getPeerId(), peer});
    if (isHost) {
      m_host = peer;
    } else {
      if(m_host) {
        m_host->queueMessage(MessageDto::createShared(MessageCodes::OUTGOING_HOST_CLIENT_JOINED, oatpp::Int64(peer->getPeerId())));
      }
    }
  }

  auto hello = HelloMessageDto::createShared();
  hello->peerId = peer->getPeerId();
  hello->isHost = isHost;

  peer->queueMessage(MessageDto::createShared(MessageCodes::OUTGOING_HELLO, hello));

}

void Session::setHost(const std::shared_ptr<Peer>& peer){
  std::lock_guard<std::mutex> lock(m_peersMutex);
  m_host = peer;
}

std::shared_ptr<Peer> Session::getHost() {
  std::lock_guard<std::mutex> lock(m_peersMutex);
  return m_host;
}

bool Session::isHostPeer(v_int64 peerId) {
  std::lock_guard<std::mutex> lock(m_peersMutex);
  return m_host && m_host->getPeerId() == peerId;
}

void Session::removePeerById(v_int64 peerId, bool& isEmpty) {
  std::lock_guard<std::mutex> lock(m_peersMutex);
  if(m_host && m_host->getPeerId() == peerId) {
    m_host.reset();
  }
  m_peers.erase(peerId);
  isEmpty = m_peers.empty();
  if(m_host) {
    m_host->queueMessage(MessageDto::createShared(MessageCodes::OUTGOING_HOST_CLIENT_LEFT, oatpp::Int64(peerId)));
  }
}

std::vector<std::shared_ptr<Peer>> Session::getAllPeers() {
  std::lock_guard<std::mutex> lock(m_peersMutex);
  std::vector<std::shared_ptr<Peer>> result;
  for(auto& pair : m_peers) {
    result.emplace_back(pair.second);
  }
  return result;
}

std::vector<std::shared_ptr<Peer>> Session::getPeers(const oatpp::Vector<oatpp::Int64>& peerIds) {

  if(!peerIds) {
    return {};
  }

  std::vector<std::shared_ptr<Peer>> result;

  std::lock_guard<std::mutex> lock(m_peersMutex);

  for(auto& id : *peerIds) {
    if(id) {
      auto it = m_peers.find(*id);
      if(it != m_peers.end()) {
        result.emplace_back(it->second);
      }
    }
  }

  return result;
}

v_int64 Session::generateNewPeerId() {
  return m_peerIdCounter ++;
}

std::mutex& Session::getSessionMutex() {
  return m_peersMutex;
}