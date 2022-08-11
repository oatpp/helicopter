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

#ifndef DTOs_hpp
#define DTOs_hpp

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

/**
 * host <--> server <--> client
 * message codes.
 * - 0..99 general messages
 * - 100 - 199 outgoing host messages
 * - 200 - 299 incoming host messages
 * - 300 - 399 outgoing client messages
 * - 400 - 499 incoming client messages
 */
ENUM(MessageCodes, v_int32,

///////////////////////////////////////////////////////////////////
//// 0..99 general messages

     /**
      * Server sends ping message to peer.
      */
     VALUE(OUTGOING_PING, 0),

     /**
      * Peer responds to server with ping message.
      */
     VALUE(INCOMING_PING, 1),

     /**
      * Sent to peer to indicate operation error.
      */
     VALUE(OUTGOING_ERROR, 2),

     /**
      * Server notifies peers that new host has been elected.
      */
     VALUE(OUTGOING_NEW_HOST, 3),

     /**
      * Server sends message to peer.
      */
     VALUE(OUTGOING_MESSAGE, 4),

     /**
      * Peer broadcasts message to all clients.
      */
     VALUE(INCOMING_BROADCAST, 5),

     /**
      * Peer sends message to a client or to a group of clients.
      */
     VALUE(INCOMING_DIRECT_MESSAGE, 6),

///////////////////////////////////////////////////////////////////
//// 100 - 199 outgoing host messages

     /**
      * Sent to a peer once connected as a game host.
      * Also sent to a peer when it was elected as a new host.
      */
     VALUE(OUTGOING_HOST_HELLO, 100),

     /**
      * Sent to host when new client joined the game.
      */
     VALUE(OUTGOING_HOST_CLIENT_JOINED, 101),

     /**
      * Sent to host when client left the game.
      */
     VALUE(OUTGOING_HOST_CLIENT_LEFT, 102),

///////////////////////////////////////////////////////////////////
//// 200 - 299 incoming host messages

     /**
      * Host sends to server to kick client or a group of clients.
      */
     VALUE(INCOMING_HOST_KICK_CLIENTS, 200),

///////////////////////////////////////////////////////////////////
//// 300 - 399 outgoing client messages

     /**
      * Sent to a peer once connected and joined the game.
      */
     VALUE(OUTGOING_CLIENT_HELLO, 300),

///////////////////////////////////////////////////////////////////
//// 400 - 499 incoming client messages

     /**
      * Client sends direct message to host.
      */
     VALUE(INCOMING_CLIENT_MESSAGE, 400)

);

/**
 * Error codes.
 */
ENUM(ErrorCodes, v_int32,

     /**
      * Operation not permitted.
      */
     VALUE(OPERATION_NOT_PERMITTED, 0)

);

/**
 * Direct message
 */
class ErrorDto : public oatpp::DTO {

  DTO_INIT(ErrorDto, DTO)

  /**
   * Error code
   */
  DTO_FIELD(Enum<ErrorCodes>::AsNumber::NotNull, code);

  /**
   * Error text message
   */
  DTO_FIELD(String, message);

};

/**
 * Direct message
 */
class DirectMessageDto : public oatpp::DTO {

  DTO_INIT(DirectMessageDto, DTO)

  /**
   * PeerIds of recipients
   */
  DTO_FIELD(Vector<Int64>, peerIds) = {};

  /**
   * Message data
   */
  DTO_FIELD(String, data);

};

/**
 * Message
 */
class MessageDto : public oatpp::DTO {

  DTO_INIT(MessageDto, DTO)

  /**
   * Message code
   */
  DTO_FIELD(Enum<MessageCodes>::AsNumber::NotNull, code);

  /**
   * Operation Correlation ID
   */
  DTO_FIELD(oatpp::String, ocid);

  /**
   * Message payload
   */
  DTO_FIELD(oatpp::Any, payload);

  DTO_FIELD_TYPE_SELECTOR(payload) {

    if(!code) return Void::Class::getType();

    switch (*code) {

      case MessageCodes::OUTGOING_ERROR:
        return oatpp::Object<ErrorDto>::Class::getType();

      case MessageCodes::INCOMING_BROADCAST:
        return oatpp::String::Class::getType();

      case MessageCodes::INCOMING_DIRECT_MESSAGE:
        return oatpp::Object<DirectMessageDto>::Class::getType();


      default:
        throw std::runtime_error("not implemented");

    }

  }

};


#include OATPP_CODEGEN_END(DTO)

#endif // DTOs_hpp
