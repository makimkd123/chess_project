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

type game struct {
	ID     string `json:"id"`
	FEN    string `json:"fen"`
	Status string `json:"status"`
}

type apiServer struct {
	mu    sync.RWMutex
	games map[string]game
}

func newAPIServer() *apiServer {
	return &apiServer{games: make(map[string]game)}
}

func (s *apiServer) routes() http.Handler {
	mux := http.NewServeMux()
	mux.HandleFunc("GET /health", func(w http.ResponseWriter, _ *http.Request) {
		writeJSON(w, http.StatusOK, map[string]string{"status": "ok"})
	})
	mux.HandleFunc("POST /games", s.createGame)
	mux.HandleFunc("GET /games/{id}", s.getGame)
	return mux
}

func (s *apiServer) createGame(w http.ResponseWriter, _ *http.Request) {
	var randomBytes [16]byte
	if _, err := rand.Read(randomBytes[:]); err != nil {
		writeJSON(w, http.StatusInternalServerError, map[string]string{"error": "could not create game"})
		return
	}

	g := game{
		ID:     hex.EncodeToString(randomBytes[:]),
		FEN:    startingFEN,
		Status: "waiting_for_opponent",
	}

	s.mu.Lock()
	s.games[g.ID] = g
	s.mu.Unlock()

	w.Header().Set("Location", "/games/"+g.ID)
	writeJSON(w, http.StatusCreated, g)
}

func (s *apiServer) getGame(w http.ResponseWriter, r *http.Request) {
	s.mu.RLock()
	g, exists := s.games[r.PathValue("id")]
	s.mu.RUnlock()
	if !exists {
		writeJSON(w, http.StatusNotFound, map[string]string{"error": "game not found"})
		return
	}
	writeJSON(w, http.StatusOK, g)
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
