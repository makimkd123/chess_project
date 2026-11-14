package main

import (
	"bytes"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"sync"
	"testing"
)

type moveTestPlayer struct {
	ID          string `json:"id"`
	Color       string `json:"color"`
	PlayerToken string `json:"player_token"`
}

type moveTestPublicGame struct {
	ID          string `json:"id"`
	FEN         string `json:"fen"`
	Status      string `json:"status"`
	WhiteJoined bool   `json:"white_joined"`
	BlackJoined bool   `json:"black_joined"`
}

func moveTestServe(handler http.Handler, method, path string, body []byte) *httptest.ResponseRecorder {
	req := httptest.NewRequest(method, path, bytes.NewReader(body))
	if body != nil {
		req.Header.Set("Content-Type", "application/json")
	}
	response := httptest.NewRecorder()
	handler.ServeHTTP(response, req)
	return response
}

func moveTestCreateGame(t *testing.T, handler http.Handler) moveTestPlayer {
	t.Helper()
	response := moveTestServe(handler, http.MethodPost, "/games", nil)
	if response.Code != http.StatusCreated {
		t.Fatalf("create game: expected HTTP 201, got %d: %s", response.Code, response.Body.String())
	}
	var player moveTestPlayer
	if err := json.Unmarshal(response.Body.Bytes(), &player); err != nil {
		t.Fatalf("decode creator response: %v", err)
	}
	if player.ID == "" || player.PlayerToken == "" || player.Color != "white" {
		t.Fatalf("invalid creator response: %+v", player)
	}
	return player
}

func moveTestCreateActiveGame(t *testing.T, handler http.Handler) (moveTestPlayer, moveTestPlayer) {
	t.Helper()
	white := moveTestCreateGame(t, handler)
	response := moveTestServe(handler, http.MethodPost, "/games/"+white.ID+"/join", nil)
	if response.Code != http.StatusOK {
		t.Fatalf("join game: expected HTTP 200, got %d: %s", response.Code, response.Body.String())
	}
	var black moveTestPlayer
	if err := json.Unmarshal(response.Body.Bytes(), &black); err != nil {
		t.Fatalf("decode join response: %v", err)
	}
	if black.ID != white.ID || black.Color != "black" || black.PlayerToken == "" || black.PlayerToken == white.PlayerToken {
		t.Fatalf("invalid second player response: %+v", black)
	}
	return white, black
}

func moveTestSubmit(t *testing.T, handler http.Handler, id, move, token string) *httptest.ResponseRecorder {
	t.Helper()
	body, err := json.Marshal(MoveRequest{Move: move, PlayerToken: token})
	if err != nil {
		t.Fatalf("encode move request: %v", err)
	}
	return moveTestServe(handler, http.MethodPost, "/games/"+id+"/moves", body)
}

func moveTestState(t *testing.T, handler http.Handler, id string) moveTestPublicGame {
	t.Helper()
	response := moveTestServe(handler, http.MethodGet, "/games/"+id, nil)
	if response.Code != http.StatusOK {
		t.Fatalf("get game: expected HTTP 200, got %d: %s", response.Code, response.Body.String())
	}
	if bytes.Contains(response.Body.Bytes(), []byte("player_token")) {
		t.Fatal("public game response must not expose player tokens")
	}
	var state moveTestPublicGame
	if err := json.Unmarshal(response.Body.Bytes(), &state); err != nil {
		t.Fatalf("decode game state: %v", err)
	}
	return state
}

func TestMoveEndpointRejectsMalformedJSON(t *testing.T) {
	handler := newAPIServer().routes()
	white, _ := moveTestCreateActiveGame(t, handler)
	response := moveTestServe(handler, http.MethodPost, "/games/"+white.ID+"/moves", []byte(`{"move":`))
	if response.Code != http.StatusBadRequest {
		t.Fatalf("expected HTTP 400, got %d: %s", response.Code, response.Body.String())
	}
}

func TestMoveEndpointRejectsUnknownGame(t *testing.T) {
	handler := newAPIServer().routes()
	response := moveTestSubmit(t, handler, "nonexistent", "e2e4", "some-token")
	if response.Code != http.StatusNotFound {
		t.Fatalf("expected HTTP 404, got %d: %s", response.Code, response.Body.String())
	}
}

func TestMoveEndpointRejectsInactiveGame(t *testing.T) {
	handler := newAPIServer().routes()
	white := moveTestCreateGame(t, handler)
	response := moveTestSubmit(t, handler, white.ID, "e2e4", white.PlayerToken)
	if response.Code != http.StatusConflict {
		t.Fatalf("expected HTTP 409, got %d: %s", response.Code, response.Body.String())
	}
	state := moveTestState(t, handler, white.ID)
	if state.FEN != startingFEN || state.Status != "waiting_for_opponent" {
		t.Fatalf("inactive game was changed: %+v", state)
	}
}

func TestMoveEndpointRejectsInvalidTokens(t *testing.T) {
	handler := newAPIServer().routes()
	white, _ := moveTestCreateActiveGame(t, handler)
	for _, tc := range []struct {
		name  string
		token string
	}{
		{name: "missing", token: ""},
		{name: "incorrect", token: "not-a-valid-token"},
	} {
		t.Run(tc.name, func(t *testing.T) {
			response := moveTestSubmit(t, handler, white.ID, "e2e4", tc.token)
			if response.Code != http.StatusUnauthorized {
				t.Fatalf("expected HTTP 401, got %d: %s", response.Code, response.Body.String())
			}
		})
	}
	if state := moveTestState(t, handler, white.ID); state.FEN != startingFEN {
		t.Fatalf("unauthorized requests changed FEN: %q", state.FEN)
	}
}

func TestMoveEndpointRejectsOutOfTurnMove(t *testing.T) {
	handler := newAPIServer().routes()
	white, black := moveTestCreateActiveGame(t, handler)

	// Black cannot play first.
	response := moveTestSubmit(t, handler, white.ID, "e7e5", black.PlayerToken)
	if response.Code != http.StatusForbidden {
		t.Fatalf("Black moved first: expected HTTP 403, got %d", response.Code)
	}
	if state := moveTestState(t, handler, white.ID); state.FEN != startingFEN {
		t.Fatalf("out-of-turn request changed FEN: %q", state.FEN)
	}

	// After White moves, White cannot immediately move again.
	response = moveTestSubmit(t, handler, white.ID, "e2e4", white.PlayerToken)
	if response.Code != http.StatusOK {
		t.Fatalf("White's legal move failed: HTTP %d: %s", response.Code, response.Body.String())
	}
	response = moveTestSubmit(t, handler, white.ID, "d2d4", white.PlayerToken)
	if response.Code != http.StatusForbidden {
		t.Fatalf("White moved twice: expected HTTP 403, got %d", response.Code)
	}
}

func TestMoveEndpointRejectsIllegalMoveWithoutChangingState(t *testing.T) {
	handler := newAPIServer().routes()
	white, _ := moveTestCreateActiveGame(t, handler)

	response := moveTestSubmit(t, handler, white.ID, "e2e5", white.PlayerToken)
	if response.Code != http.StatusUnprocessableEntity {
		t.Fatalf("expected HTTP 422, got %d: %s", response.Code, response.Body.String())
	}
	if state := moveTestState(t, handler, white.ID); state.FEN != startingFEN {
		t.Fatalf("illegal move changed FEN: %q", state.FEN)
	}

	// A failed move must not block the same player from trying a legal move.
	response = moveTestSubmit(t, handler, white.ID, "e2e4", white.PlayerToken)
	if response.Code != http.StatusOK {
		t.Fatalf("valid move after rejected move: HTTP %d: %s", response.Code, response.Body.String())
	}
}

func TestMoveEndpointUpdatesFENAndAlternatesTurns(t *testing.T) {
	handler := newAPIServer().routes()
	white, black := moveTestCreateActiveGame(t, handler)
	tests := []struct {
		name    string
		move    string
		token   string
		wantFEN string
	}{
		{
			name: "White e2e4", move: "e2e4", token: white.PlayerToken,
			wantFEN: "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
		},
		{
			name: "Black e7e5", move: "e7e5", token: black.PlayerToken,
			wantFEN: "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2",
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			response := moveTestSubmit(t, handler, white.ID, tc.move, tc.token)
			if response.Code != http.StatusOK {
				t.Fatalf("expected HTTP 200, got %d: %s", response.Code, response.Body.String())
			}
			var result MoveResponse
			if err := json.Unmarshal(response.Body.Bytes(), &result); err != nil {
				t.Fatalf("decode move response: %v", err)
			}
			if result.GameID != white.ID || result.FEN != tc.wantFEN || result.Status != "active" {
				t.Fatalf("incorrect move result: %+v", result)
			}
			state := moveTestState(t, handler, white.ID)
			if state.FEN != tc.wantFEN || state.Status != "active" || !state.WhiteJoined || !state.BlackJoined {
				t.Fatalf("game was not saved correctly: %+v", state)
			}
		})
	}
}

func TestMoveEndpointFinishesOnCheckmate(t *testing.T) {
	handler := newAPIServer().routes()
	white, black := moveTestCreateActiveGame(t, handler)

	// Fool's mate: 1. f3 e5 2. g4 Qh4#.
	moves := []struct {
		uci   string
		token string
	}{
		{uci: "f2f3", token: white.PlayerToken},
		{uci: "e7e5", token: black.PlayerToken},
		{uci: "g2g4", token: white.PlayerToken},
		{uci: "d8h4", token: black.PlayerToken},
	}
	for i, move := range moves {
		response := moveTestSubmit(t, handler, white.ID, move.uci, move.token)
		if response.Code != http.StatusOK {
			t.Fatalf("move %d (%s): expected HTTP 200, got %d: %s", i+1, move.uci, response.Code, response.Body.String())
		}
		if i == len(moves)-1 {
			var result MoveResponse
			if err := json.Unmarshal(response.Body.Bytes(), &result); err != nil {
				t.Fatalf("decode final move: %v", err)
			}
			if result.Status != "finished" {
				t.Fatalf("checkmate must finish game, got status %q", result.Status)
			}
		}
	}
	state := moveTestState(t, handler, white.ID)
	if state.Status != "finished" {
		t.Fatalf("stored game status after checkmate = %q", state.Status)
	}
	response := moveTestSubmit(t, handler, white.ID, "e2e4", white.PlayerToken)
	if response.Code != http.StatusConflict {
		t.Fatalf("finished game accepted a move: expected HTTP 409, got %d", response.Code)
	}
}

func TestMoveEndpointConcurrentSubmissionsAllowOnlyOneMove(t *testing.T) {
	handler := newAPIServer().routes()
	white, _ := moveTestCreateActiveGame(t, handler)

	const attempts = 32
	body, err := json.Marshal(MoveRequest{Move: "e2e4", PlayerToken: white.PlayerToken})
	if err != nil {
		t.Fatal(err)
	}

	var wg sync.WaitGroup
	statuses := make(chan int, attempts)
	for i := 0; i < attempts; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			response := moveTestServe(handler, http.MethodPost, "/games/"+white.ID+"/moves", body)
			statuses <- response.Code
		}()
	}
	wg.Wait()
	close(statuses)

	accepted, forbidden := 0, 0
	for code := range statuses {
		switch code {
		case http.StatusOK:
			accepted++
		case http.StatusForbidden:
			forbidden++
		default:
			t.Errorf("unexpected HTTP status from concurrent request: %d", code)
		}
	}
	if accepted != 1 || forbidden != attempts-1 {
		t.Fatalf("expected exactly one accepted move and %d forbidden, got %d accepted and %d forbidden", attempts-1, accepted, forbidden)
	}

	state := moveTestState(t, handler, white.ID)
	wantFEN := "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"
	if state.FEN != wantFEN {
		t.Fatalf("concurrent submissions corrupted game state: got %q, want %q", state.FEN, wantFEN)
	}
}
