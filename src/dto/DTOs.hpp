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

ENUM(MessageCodes, v_int32,
  VALUE(CODE_INFO, 0),

  VALUE(CODE_PEER_JOINED, 1),
  VALUE(CODE_PEER_LEFT, 2),
  VALUE(CODE_PEER_MESSAGE, 3),
  VALUE(CODE_PEER_MESSAGE_FILE, 4),
  VALUE(CODE_PEER_IS_TYPING, 5),

  VALUE(CODE_FILE_SHARE, 6),
  VALUE(CODE_FILE_REQUEST_CHUNK, 7),
  VALUE(CODE_FILE_CHUNK_DATA, 8),

  VALUE(CODE_API_ERROR, 9)
);



#include OATPP_CODEGEN_END(DTO)

#endif // DTOs_hpp
