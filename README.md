# Helicopter

WebSocket server for multiplayer games.

:warning: **This project is in development and is NOT ready to use.**

## API 

### Brief

There are two types of clients in Helicopter server:

- `Game Host` - the one who creates the game.
- `Game Client` - the one who joins the already created game.

`Game Host` is responsible for managing game state and calculating physics.  
:point_right: Helicopter server is agnostic of the game logic/physics/state etc. :point_left:  

`Game Client` communicates with `Game Host` through Helicopter server using WebSocket API provided.

### WebSocket API

#### Create Game


```
ws://<host>:<port>/api/create-game/?gameId=<gameId>&sessionId=<sessionId>`
```

Where:

- `gameId` is the id of the game from game config - stored on Helicopter server.
- `sessionId` is an arbitrary unique string which identifies the given game session. 

#### Join Game

```
ws://<host>:<port>/api/join-game/?gameId=<gameId>&sessionId=<sessionId>`
```

Where:

- `gameId` is the id of the game from game config - stored on Helicopter server.
- `sessionId` is an identifier of the game session created by the `Game Host`.

#### Messaging

Helicopter server is using `JSON` for messaging.

All messages have the same structure (both incoming and outgoing):

```json
{
  "code":    integer,
  "ocid":    string or null,
  "payload": content type depends on message code
}
```

Where:
- `code` - operation/message code.
- `ocid` - Operation Correlation ID - used to correlate operation and an error message.
- `payload` - message payload

##### Message Codes

Legend:

- :arrow_left: - incoming
- :arrow_right: - outgoing
- `H` - Game Host - can send this message
- `C` - Game Client - can send this message
- `HC` - both Game Host and a Game Client can send this message

|Code|Direction|Peer Role|Description|Payload Type|
|:---:|:---:|:---:|:---:|:---:|
|0|:arrow_left:|`HC`|**Hello Message** <br> Once connected client will receive this message providing client with its `peerId` and its role (`isHost` - `true` or `false`) |object: `{"peerId": integer, "isHost": boolean}`|
|1|:arrow_left:|`HC`|**Ping** <br> Once received peer MUST respond with the proper Pong message | `integer` |
|2|:arrow_right:|`HC`|**Pong** <br> Peer responds with Pong message to server's Ping. Peer MUST include the same value it received in Ping to the Pong payload| `integer` |
|3|:arrow_left:|`HC`|**Error Message** <br> See Error-Codes for error messages|object: `{"code": integer, "message": string}`|
|5|:arrow_left:|`HC`|**Incoming Message** from other peer|object: `{"peerId": integer, "data": string}`|
|6|:arrow_right:|`HC`|**Broadcast** <br> Peer broadcasts message to all other peers|`string`|
|7|:arrow_right:|`HC`|**Direct Message** <br> Peer sends message to another peer or to a group of peers.|object: `{"peerIds": [integer, ...], "data": string}`|
|8|:arrow_right:|`HC`|**Synchronized Event** <br> Synchronized event will be broadcasted to ALL peers, including the sender of this event. All peers are guaranteed to receive synchronized events in the same order except for cases where peer's messages were discarded due to poor connection (message queue overflow).|`string`|
