package main

import (
	"encoding/json"
	"net/http"
	"strings"
)

type MoveRequest struct {
	Move        string `json:"move"`
	PlayerToken string `json:"player_token"`
}

type MoveResponse struct {
	GameID string `json:"game_id"`
	FEN    string `json:"fen"`
	Status string `json:"status"`
}

func (s *apiServer) submitMove(w http.ResponseWriter, r *http.Request) {
	var newMove MoveRequest
	var playerColor string
	err := json.NewDecoder(r.Body).Decode(&newMove)
	if err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{
			"error": "invalid JSON request",
		})
		return
	}
	s.mu.RLock()
	session, exists := s.games[r.PathValue("id")]
	s.mu.RUnlock()
	if !exists {
		writeJSON(w, http.StatusNotFound, map[string]string{
			"error": "404 Not Found",
		})
		return
	}
	if newMove.PlayerToken == "" {
		writeJSON(w, http.StatusUnauthorized, map[string]string{
			"error": "missing player token",
		})
		return
	}
	if newMove.PlayerToken == session.whiteToken {
		playerColor = "white"
	} else if newMove.PlayerToken == session.blackToken {
		playerColor = "black"
	} else {
		writeJSON(w, http.StatusUnauthorized, map[string]string{
			"error": "401 Unauthorized",
		})
		return
	}

	if session.state.Status != "active" {
		writeJSON(w, http.StatusConflict, map[string]string{"error": "game is not active"})
		return
	}

	fenFields := strings.Fields(session.state.FEN)

	if len(fenFields) != 6 {
		writeJSON(w, http.StatusInternalServerError, map[string]string{
			"error": "invalid stored FEN",
		})
		return
	}

	var turnColor string

	if fenFields[1] == "w" {
		turnColor = "white"
	} else if fenFields[1] == "b" {
		turnColor = "black"
	} else {
		writeJSON(w, http.StatusInternalServerError, map[string]string{
			"error": "invalid FEN turn",
		})
		return
	}

	if turnColor != playerColor {
		writeJSON(w, http.StatusForbidden, map[string]string{"error": "not your turn"})
		return
	}
	_ = playerColor
}
