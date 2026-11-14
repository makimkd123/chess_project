package main

import (
	"bytes"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"sync"
	"testing"
)

func TestJoiningGameAndPrivateTokens(t *testing.T) {
	handler := newAPIServer().routes()
	create := httptest.NewRecorder()
	handler.ServeHTTP(create, httptest.NewRequest(http.MethodPost, "/games", nil))
	if create.Code != http.StatusCreated {
		t.Fatalf("create returned %d", create.Code)
	}
	var creator playerResponse
	if err := json.Unmarshal(create.Body.Bytes(), &creator); err != nil {
		t.Fatal(err)
	}
	if creator.Color != "white" || creator.PlayerToken == "" || !creator.WhiteJoined || creator.BlackJoined {
		t.Fatalf("invalid creator: %+v", creator)
	}
	path := "/games/" + creator.ID
	join := httptest.NewRecorder()
	handler.ServeHTTP(join, httptest.NewRequest(http.MethodPost, path+"/join", nil))
	if join.Code != http.StatusOK {
		t.Fatalf("join returned %d: %s", join.Code, join.Body.String())
	}
	var guest playerResponse
	if err := json.Unmarshal(join.Body.Bytes(), &guest); err != nil {
		t.Fatal(err)
	}
	if guest.ID != creator.ID || guest.Color != "black" || guest.PlayerToken == "" || guest.PlayerToken == creator.PlayerToken || guest.Status != "active" || !guest.BlackJoined {
		t.Fatalf("invalid guest: %+v", guest)
	}
	public := httptest.NewRecorder()
	handler.ServeHTTP(public, httptest.NewRequest(http.MethodGet, path, nil))
	if public.Code != http.StatusOK || !bytes.Contains(public.Body.Bytes(), []byte(`"status":"active"`)) {
		t.Fatalf("public state: %d %s", public.Code, public.Body.String())
	}
	if bytes.Contains(public.Body.Bytes(), []byte("player_token")) || bytes.Contains(public.Body.Bytes(), []byte(creator.PlayerToken)) || bytes.Contains(public.Body.Bytes(), []byte(guest.PlayerToken)) {
		t.Fatal("credentials leaked into public game state")
	}
	third := httptest.NewRecorder()
	handler.ServeHTTP(third, httptest.NewRequest(http.MethodPost, path+"/join", nil))
	if third.Code != http.StatusConflict {
		t.Fatalf("third player returned %d, want 409", third.Code)
	}
	missing := httptest.NewRecorder()
	handler.ServeHTTP(missing, httptest.NewRequest(http.MethodPost, "/games/missing/join", nil))
	if missing.Code != http.StatusNotFound {
		t.Fatalf("missing game returned %d, want 404", missing.Code)
	}
}

func TestConcurrentJoiningAllowsOnlyOnePlayer(t *testing.T) {
	handler := newAPIServer().routes()
	create := httptest.NewRecorder()
	handler.ServeHTTP(create, httptest.NewRequest(http.MethodPost, "/games", nil))
	var g game
	if err := json.Unmarshal(create.Body.Bytes(), &g); err != nil {
		t.Fatal(err)
	}

	const attempts = 50
	results := make(chan int, attempts)
	var wg sync.WaitGroup
	for i := 0; i < attempts; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			r := httptest.NewRecorder()
			handler.ServeHTTP(r, httptest.NewRequest(http.MethodPost, "/games/"+g.ID+"/join", nil))
			results <- r.Code
		}()
	}
	wg.Wait()
	close(results)
	wins, conflicts := 0, 0
	for code := range results {
		switch code {
		case http.StatusOK:
			wins++
		case http.StatusConflict:
			conflicts++
		default:
			t.Errorf("unexpected join status %d", code)
		}
	}
	if wins != 1 || conflicts != attempts-1 {
		t.Fatalf("%d successes, %d conflicts; expected 1 and %d", wins, conflicts, attempts-1)
	}
}
