package chess

import (
	"strings"
	"testing"
)

func TestStartingPosition(t *testing.T) {
	p, err := parseFEN(StartingFEN)
	if err != nil {
		t.Fatal(err)
	}
	if got := p.fen(); got != StartingFEN {
		t.Fatalf("roundtrip = %q", got)
	}
	moves, err := LegalMoves(StartingFEN)
	if err != nil {
		t.Fatal(err)
	}
	if len(moves) != 20 {
		t.Fatalf("start has %d moves, want 20", len(moves))
	}
	if n := perft(p, 2); n != 400 {
		t.Fatalf("perft depth 2 = %d, want 400", n)
	}
	if n := perft(p, 3); n != 8902 {
		t.Fatalf("perft depth 3 = %d, want 8902", n)
	}
}

func perft(p position, depth int) int {
	if depth == 0 {
		return 1
	}
	result := 0
	for _, m := range p.legalMoves() {
		result += perft(p.apply(m), depth-1)
	}
	return result
}

func TestMoves(t *testing.T) {
	tests := []struct{ name, fen, move, want string }{
		{"white pawn", "" + StartingFEN, "e2e4", "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"},
		{"black pawn", "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", "e7e5", "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2"},
		{"white castle", "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", "e1g1", "r3k2r/8/8/8/8/8/8/R4RK1 b kq - 1 1"},
		{"black castle queenside", "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", "e8c8", "2kr3r/8/8/8/8/8/8/R3K2R w KQ - 1 2"},
		{"white en passant", "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 2", "e5d6", "4k3/8/3P4/8/8/8/8/4K3 b - - 0 2"},
		{"promotion queen", "4k3/P7/8/8/8/8/8/4K3 w - - 0 1", "a7a8q", "Q3k3/8/8/8/8/8/8/4K3 b - - 0 1"},
		{"promotion knight", "4k3/P7/8/8/8/8/8/4K3 w - - 0 1", "a7a8n", "N3k3/8/8/8/8/8/8/4K3 b - - 0 1"},
		{"black promotion", "4k3/8/8/8/8/8/p7/4K3 b - - 0 1", "a2a1q", "4k3/8/8/8/8/8/8/q3K3 w - - 0 2"},
		{"rook captures enemy rook", "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", "h1h8", "r3k2R/8/8/8/8/8/8/R3K3 b Qq - 0 1"},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got, err := ApplyUCI(tt.fen, tt.move)
			if err != nil {
				t.Fatal(err)
			}
			if got != tt.want {
				t.Errorf("ApplyUCI(%q):\n got %q\nwant %q", tt.move, got, tt.want)
			}
		})
	}
}

func TestIllegalMoves(t *testing.T) {
	tests := []struct{ name, fen, move string }{
		{"moving black piece as white", StartingFEN, "e7e5"},
		{"blocked bishop", StartingFEN, "f1c4"},
		{"wrong rank", StartingFEN, "e2e9"},
		{"wrong file", StartingFEN, "i2e4"},
		{"garbage", StartingFEN, "abcd"},
		{"same square", StartingFEN, "e2e2"},
		{"no promotion", "4k3/P7/8/8/8/8/8/4K3 w - - 0 1", "a7a8"},
		{"bad promotion", "4k3/P7/8/8/8/8/8/4K3 w - - 0 1", "a7a8k"},
		{"pinned en passant", "4r1k1/8/8/3pP3/8/8/8/4K3 w - d6 0 2", "e5d6"},
		{"castle through check", "r3k2r/8/8/8/8/5r2/8/R3K2R w KQkq - 0 1", "e1g1"},
		{"castle blocked", StartingFEN, "e1g1"},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got, err := ApplyUCI(tt.fen, tt.move); err == nil {
				t.Fatalf("accepted illegal move %q: %s", tt.move, got)
			}
		})
	}
}

func TestInvalidFEN(t *testing.T) {
	tests := []string{"", "8/8/8/8/8/8/8/8 w - - 0 1", "4k3/8/8/8/8/8/8/4K3 x - - 0 1", "4k3/8/8/8/8/8/8/4K3 w KKK - 0 1", "4k3/8/8/8/8/8/8/4K3 w - - -1 1", "4k3/8/8/8/8/8/8/4K3 w - h5 0 1"}
	for _, fen := range tests {
		if _, err := LegalMoves(fen); err == nil {
			t.Errorf("accepted invalid FEN %q", fen)
		}
	}
}

func TestOutcomes(t *testing.T) {
	fen := StartingFEN
	for _, m := range []string{"f2f3", "e7e5", "g2g4", "d8h4"} {
		var err error
		fen, err = ApplyUCI(fen, m)
		if err != nil {
			t.Fatalf("%s: %v", m, err)
		}
	}
	cases := []struct{ fen, want string }{
		{fen, "checkmate"},
		{"7k/5Q2/6K1/8/8/8/8/8 b - - 0 1", "stalemate"},
		{"4k3/8/8/8/8/8/8/4K3 w - - 0 1", "insufficient_material"},
		{"4k3/8/8/8/8/8/8/3QK3 w - - 150 75", "draw_75_moves"},
		{StartingFEN, "ongoing"},
	}
	for _, tt := range cases {
		got, err := Outcome(tt.fen)
		if err != nil {
			t.Fatal(err)
		}
		if got != tt.want {
			t.Errorf("Outcome(%q) = %s, want %s", tt.fen, got, tt.want)
		}
	}
}

func TestKingSafety(t *testing.T) {
	// A pinned rook cannot move sideways, exposing its king to a rook on e8.
	fen := "4r1k1/8/8/8/8/8/4R3/4K3 w - - 0 1"
	if _, err := ApplyUCI(fen, "e2d2"); err == nil {
		t.Fatal("allowed pinned rook to move off e-file")
	}
	if _, err := ApplyUCI(fen, "e2e8"); err != nil {
		t.Fatalf("capturing pinning rook should be legal: %v", err)
	}
}

func TestAllMovesAreUCI(t *testing.T) {
	moves, err := LegalMoves(StartingFEN)
	if err != nil {
		t.Fatal(err)
	}
	for _, m := range moves {
		if len(m) != 4 && len(m) != 5 || strings.Contains(m, " ") {
			t.Errorf("bad UCI %q", m)
		}
	}
}

func TestPerftReferencePositions(t *testing.T) {
	// Standard reference positions used to test chess move generators.
	cases := []struct {
		fen    string
		counts []int
	}{
		{"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", []int{14, 191, 2812, 43238}},
		{"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", []int{44, 1486, 62379}},
	}
	for _, tc := range cases {
		p, err := parseFEN(tc.fen)
		if err != nil {
			t.Fatal(err)
		}
		for d, n := range tc.counts {
			got := perft(p, d+1)
			if got != n {
				t.Errorf("perft(%s, depth=%d) = %d, want %d", tc.fen, d+1, got, n)
			}
		}
	}
}
