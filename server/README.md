# Chess Go server

A minimal HTTP backend for the C++/Qt chess project. This first milestone uses Go's standard library only.

## Run

From the `server` directory:

```sh
go run ./cmd/server
```

The server listens on `http://localhost:8080` by default. Set the `PORT` environment variable to use another port.

## API (first milestone)

| Method | Path | Description |
| --- | --- | --- |
| `GET` | `/health` | Check that the server is running |
| `POST` | `/games` | Create a game with the starting FEN |
| `GET` | `/games/{id}` | Retrieve a created game |

PowerShell example:

```powershell
Invoke-RestMethod http://localhost:8080/health
$game = Invoke-RestMethod -Method Post http://localhost:8080/games
$game
Invoke-RestMethod "http://localhost:8080/games/$($game.id)"
```

## Tests

```sh
go test -race ./...
```

**Not yet implemented:** joining games, submitting or validating moves, player authentication, WebSockets, and database storage. Games are stored in memory and disappear when the process restarts. The first milestone is intended for local development, not public deployment.
