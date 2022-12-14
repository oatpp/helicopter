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

#include "oatpp/core/utils/ConversionUtils.hpp"

Session::Session(const oatpp::String& id, const oatpp::Object<GameConfigDto>& config)
  : m_id(id)
  , m_config(config)
  , m_peerIdCounter(0)
  , m_synchronizedEventId(0)
  , m_pingCurrentTimestamp(-1)
  , m_pingBestTime(-1)
  , m_pingBestPeerId(-1)
  , m_pingBestPeerSinceTimestamp(-1)
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

void Session::broadcastSynchronizedEvent(v_int64 senderId, const oatpp::String& eventData) {

  std::lock_guard<std::mutex> lock(m_peersMutex);

  auto event = OutgoingSynchronizedMessageDto::createShared();
  event->eventId = m_synchronizedEventId ++;
  event->peerId = senderId;
  event->data = eventData;

  auto message = MessageDto::createShared(MessageCodes::OUTGOING_SYNCHRONIZED_EVENT, event);
  for(auto& peer : m_peers) {
    peer.second->queueMessage(message);
  }

}

v_int64 Session::generateNewPeerId() {
  return m_peerIdCounter ++;
}

void Session::checkAllPeersPings() {

  v_int64 currentTimestamp;
  {
    std::lock_guard<std::mutex> lock(m_pingMutex);
    currentTimestamp = m_pingCurrentTimestamp;
  }

  std::lock_guard<std::mutex> lock(m_peersMutex);
  for(auto& peer : m_peers) {
    peer.second->checkPingsRules(currentTimestamp);
  }

}

void Session::pingAllPeers() {

  auto timestamp = oatpp::base::Environment::getMicroTickCount();

  {
    std::lock_guard<std::mutex> lock(m_pingMutex);
    m_pingCurrentTimestamp = timestamp;
  }

  std::lock_guard<std::mutex> lock(m_peersMutex);
  for(auto& peer : m_peers) {
    peer.second->ping(timestamp);
  }

}

v_int64 Session::reportPeerPong(v_int64 peerId, v_int64 timestamp) {

  std::lock_guard<std::mutex> lock(m_pingMutex);
  if(timestamp != m_pingCurrentTimestamp) {
    return -1;
  }

  v_int64 pingTime = oatpp::base::Environment::getMicroTickCount() - timestamp;

  if(m_pingBestTime < 0 || m_pingBestTime > pingTime) {
    m_pingBestTime = pingTime;
    if(m_pingBestPeerId != peerId) {
      m_pingBestPeerId = peerId;
      m_pingBestPeerSinceTimestamp = timestamp;
      OATPP_LOGD("Session", "new best peer=%lld, ping=%lld", peerId, pingTime)
    }
  }

  return pingTime;

}
