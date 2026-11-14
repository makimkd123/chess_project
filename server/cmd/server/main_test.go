package main

import (
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"sync"
	"testing"
)

func TestHealth(t *testing.T) {
	response := httptest.NewRecorder()
	newAPIServer().routes().ServeHTTP(response, httptest.NewRequest(http.MethodGet, "/health", nil))
	if response.Code != http.StatusOK {
		t.Fatalf("health status = %d, want 200", response.Code)
	}
	if response.Body.String() != "{\"status\":\"ok\"}\n" {
		t.Fatalf("health body = %q", response.Body.String())
	}
}

func TestCreateAndGetGame(t *testing.T) {
	handler := newAPIServer().routes()
	created := httptest.NewRecorder()
	handler.ServeHTTP(created, httptest.NewRequest(http.MethodPost, "/games", nil))
	if created.Code != http.StatusCreated {
		t.Fatalf("create status = %d, want 201", created.Code)
	}
	var g game
	if err := json.Unmarshal(created.Body.Bytes(), &g); err != nil {
		t.Fatal(err)
	}
	if g.ID == "" || g.FEN != startingFEN || g.Status != "waiting_for_opponent" {
		t.Fatalf("unexpected game: %+v", g)
	}
	location := "/games/" + g.ID
	if created.Header().Get("Location") != location {
		t.Fatalf("location = %q, want %q", created.Header().Get("Location"), location)
	}
	fetched := httptest.NewRecorder()
	handler.ServeHTTP(fetched, httptest.NewRequest(http.MethodGet, location, nil))
	if fetched.Code != http.StatusOK {
		t.Fatalf("get status = %d, want 200", fetched.Code)
	}
	var actual game
	if err := json.Unmarshal(fetched.Body.Bytes(), &actual); err != nil {
		t.Fatal(err)
	}
	if actual != g {
		t.Fatalf("retrieved game = %+v, want %+v", actual, g)
	}
}

func TestMissingGame(t *testing.T) {
	response := httptest.NewRecorder()
	newAPIServer().routes().ServeHTTP(response, httptest.NewRequest(http.MethodGet, "/games/unknown", nil))
	if response.Code != http.StatusNotFound {
		t.Fatalf("missing game status = %d, want 404", response.Code)
	}
}

func TestConcurrentGameCreation(t *testing.T) {
	handler := newAPIServer().routes()
	const count = 50
	var wg sync.WaitGroup
	ids := make(chan string, count)
	errors := make(chan string, count)
	for i := 0; i < count; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			response := httptest.NewRecorder()
			handler.ServeHTTP(response, httptest.NewRequest(http.MethodPost, "/games", nil))
			if response.Code != http.StatusCreated {
				errors <- response.Body.String()
				return
			}
			var g game
			if err := json.Unmarshal(response.Body.Bytes(), &g); err != nil {
				errors <- err.Error()
				return
			}
			ids <- g.ID
		}()
	}
	wg.Wait()
	close(ids)
	close(errors)
	for err := range errors {
		t.Errorf("creating game: %s", err)
	}
	seen := make(map[string]bool)
	for id := range ids {
		if id == "" || seen[id] {
			t.Fatalf("empty or duplicate id: %q", id)
		}
		seen[id] = true
	}
	if len(seen) != count {
		t.Fatalf("created %d games, want %d", len(seen), count)
	}
}
