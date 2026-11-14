# Chess Go server

This server currently implements a minimal in-memory two-player game lobby using the Go standard library.

## Run

From the `server` directory:

```sh
go run ./cmd/server
```

The server listens at `http://localhost:8080` by default. Set `PORT` to change the port.

## Routes

| Method | Path | Description |
| --- | --- | --- |
| GET | `/health` | Check server health |
| POST | `/games` | Create a game; creator gets White and a private token |
| GET | `/games/{id}` | Read the public game state; no player tokens |
| POST | `/games/{id}/join` | Join as Black; returns a private token; 409 when full |

## Try it with PowerShell

```powershell
$white = Invoke-RestMethod -Method Post http://localhost:8080/games
$black = Invoke-RestMethod -Method Post "http://localhost:8080/games/$($white.id)/join"
$state = Invoke-RestMethod "http://localhost:8080/games/$($white.id)"
$state
```

Keep `$white.player_token` and `$black.player_token` private. They are placeholders for move authorization in a future milestone, not a complete authentication system.

## Test

```sh
go test -race ./...
```

Games and tokens are stored only in memory and disappear when the server restarts. No move validation, WebSocket sync, authentication, database, rate limiting, or TLS termination exists yet. **For local development only; do not expose this server publicly.**
