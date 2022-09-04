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

|code|direction|peer role|description|payload type|
|:---:|:---:|:---:|:---:|:---:|
|0|:arrow_left:|`HC`|Hello message. <br> Once connected client will receive this message providing client with its `peerId` and its role (`isHost` - `true` or `false`) |object: `{"peerId": integer, "isHost": boolean}`|

