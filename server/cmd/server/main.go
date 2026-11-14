package main

import (
	"crypto/rand"
	"encoding/hex"
	"encoding/json"
	"log"
	"net/http"
	"os"
	"sync"
	"time"
)

const startingFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

type GameStatus string

const (
	StatusWaiting  GameStatus = "waiting_for_opponent"
	StatusActive   GameStatus = "active"
	StatusFinished GameStatus = "finished"
)

type game struct {
	ID          string     `json:"id"`
	FEN         string     `json:"fen"`
	Status      GameStatus `json:"status"`
	WhiteJoined bool       `json:"white_joined"`
	BlackJoined bool       `json:"black_joined"`
}

// Credentials are stored separately from public game state.
type gameSession struct {
	state      game
	whiteToken string
	blackToken string
}

// Credentials are returned only when the player creates or joins a game.
type playerResponse struct {
	game
	Color       string `json:"color"`
	PlayerToken string `json:"player_token"`
}

type apiServer struct {
	mu    sync.RWMutex
	games map[string]gameSession
}

func newAPIServer() *apiServer {
	return &apiServer{games: make(map[string]gameSession)}
}

func (s *apiServer) routes() http.Handler {
	mux := http.NewServeMux()
	mux.HandleFunc("GET /health", func(w http.ResponseWriter, _ *http.Request) {
		writeJSON(w, http.StatusOK, map[string]string{"status": "ok"})
	})
	mux.HandleFunc("POST /games", s.createGame)
	mux.HandleFunc("GET /games/{id}", s.getGame)
	mux.HandleFunc("POST /games/{id}/join", s.joinGame)
	return mux
}

func randomID() (string, error) {
	var randomBytes [16]byte
	if _, err := rand.Read(randomBytes[:]); err != nil {
		return "", err
	}
	return hex.EncodeToString(randomBytes[:]), nil
}

func (s *apiServer) createGame(w http.ResponseWriter, _ *http.Request) {
	id, err := randomID()
	if err != nil {
		writeJSON(w, http.StatusInternalServerError, map[string]string{"error": "could not create game"})
		return
	}
	token, err := randomID()
	if err != nil {
		writeJSON(w, http.StatusInternalServerError, map[string]string{"error": "could not create game"})
		return
	}

	g := game{ID: id, FEN: startingFEN, Status: StatusWaiting, WhiteJoined: true}
	s.mu.Lock()
	s.games[g.ID] = gameSession{state: g, whiteToken: token}
	s.mu.Unlock()

	w.Header().Set("Location", "/games/"+g.ID)
	writeJSON(w, http.StatusCreated, playerResponse{game: g, Color: "white", PlayerToken: token})
}

func (s *apiServer) getGame(w http.ResponseWriter, r *http.Request) {
	s.mu.RLock()
	session, exists := s.games[r.PathValue("id")]
	s.mu.RUnlock()
	if !exists {
		writeJSON(w, http.StatusNotFound, map[string]string{"error": "game not found"})
		return
	}
	writeJSON(w, http.StatusOK, session.state)
}

func (s *apiServer) joinGame(w http.ResponseWriter, r *http.Request) {
	token, err := randomID()
	if err != nil {
		writeJSON(w, http.StatusInternalServerError, map[string]string{"error": "could not join game"})
		return
	}

	s.mu.Lock()
	session, exists := s.games[r.PathValue("id")]
	if !exists {
		s.mu.Unlock()
		writeJSON(w, http.StatusNotFound, map[string]string{"error": "game not found"})
		return
	}
	if session.state.BlackJoined {
		s.mu.Unlock()
		writeJSON(w, http.StatusConflict, map[string]string{"error": "game is full"})
		return
	}

	session.state.BlackJoined = true
	session.state.Status = StatusActive
	session.blackToken = token
	s.games[session.state.ID] = session
	s.mu.Unlock()

	writeJSON(w, http.StatusOK, playerResponse{game: session.state, Color: "black", PlayerToken: token})
}

func writeJSON(w http.ResponseWriter, code int, data any) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(code)
	if err := json.NewEncoder(w).Encode(data); err != nil {
		log.Printf("write JSON response: %v", err)
	}
}

func main() {
	port := os.Getenv("PORT")
	if port == "" {
		port = "8080"
	}
	server := &http.Server{
		Addr:              ":" + port,
		Handler:           newAPIServer().routes(),
		ReadHeaderTimeout: 5 * time.Second,
	}
	log.Printf("chess server listening on :%s", port)
	log.Fatal(server.ListenAndServe())
}
