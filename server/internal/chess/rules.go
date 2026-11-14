// Package chess implements server-side chess rules independently of the C++ client.
//
// Positions are exchanged as FEN and moves as UCI (e.g. "e2e4", "e7e8q").
// Package functions do not mutate shared state; the HTTP layer must serialize
// validation and updates for each game session.
package chess

import (
	"errors"
	"fmt"
	"strconv"
	"strings"
)

const StartingFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

const (
	whiteKingSide uint8 = 1 << iota
	whiteQueenSide
	blackKingSide
	blackQueenSide
)

var (
	knightOffsets = [...]int{-33, -31, -18, -14, 14, 18, 31, 33}
	bishopOffsets = [...]int{-17, -15, 15, 17}
	rookOffsets   = [...]int{-16, -1, 1, 16}
	kingOffsets   = [...]int{-17, -16, -15, -1, 1, 15, 16, 17}
)

// Position uses 0x88 indexing, which makes board-boundary checks inexpensive.
type position struct {
	board     [128]byte
	turn      byte // 'w' or 'b'
	castling  uint8
	enPassant int // -1 means none
	halfmove  int
	fullmove  int
}

type move struct {
	from, to  int
	promotion byte // 'q', 'r', 'b' or 'n'; zero means none
	enPassant bool
	castle    bool
}

func validSquare(i int) bool { return i >= 0 && i < 128 && i&0x88 == 0 }

func square(s string) (int, bool) {
	if len(s) != 2 || s[0] < 'a' || s[0] > 'h' || s[1] < '1' || s[1] > '8' {
		return 0, false
	}
	return int('8'-s[1])*16 + int(s[0]-'a'), true
}

func squareName(i int) string {
	return string([]byte{byte(i&7) + 'a', byte(8-(i>>4)) + '0'})
}

func isWhite(p byte) bool { return p >= 'A' && p <= 'Z' }
func isBlack(p byte) bool { return p >= 'a' && p <= 'z' }
func color(p byte) byte {
	if isWhite(p) {
		return 'w'
	}
	if isBlack(p) {
		return 'b'
	}
	return 0
}
func lower(p byte) byte {
	if isWhite(p) {
		return p + ('a' - 'A')
	}
	return p
}
func opponent(c byte) byte {
	if c == 'w' {
		return 'b'
	}
	return 'w'
}

func parseFEN(fen string) (position, error) {
	var p position
	for i := range p.board {
		p.board[i] = '.'
	}
	p.enPassant = -1
	fields := strings.Fields(fen)
	if len(fields) != 6 {
		return p, errors.New("FEN must have six fields")
	}
	rows := strings.Split(fields[0], "/")
	if len(rows) != 8 {
		return p, errors.New("FEN must have eight ranks")
	}
	kings := [2]int{}
	for row, text := range rows {
		file := 0
		for i := 0; i < len(text); i++ {
			ch := text[i]
			if ch >= '1' && ch <= '8' {
				file += int(ch - '0')
				continue
			}
			if !strings.ContainsRune("PNBRQKpnbrqk", rune(ch)) || file >= 8 {
				return p, fmt.Errorf("invalid piece placement at rank %d", 8-row)
			}
			p.board[row*16+file] = ch
			if ch == 'K' {
				kings[0]++
			}
			if ch == 'k' {
				kings[1]++
			}
			file++
		}
		if file != 8 {
			return p, fmt.Errorf("rank %d does not contain eight squares", 8-row)
		}
	}
	if kings != [2]int{1, 1} {
		return p, errors.New("FEN requires exactly one king of each color")
	}
	if fields[1] != "w" && fields[1] != "b" {
		return p, errors.New("invalid side to move")
	}
	p.turn = fields[1][0]
	if fields[2] != "-" {
		for i := 0; i < len(fields[2]); i++ {
			var bit uint8
			switch fields[2][i] {
			case 'K':
				bit = whiteKingSide
			case 'Q':
				bit = whiteQueenSide
			case 'k':
				bit = blackKingSide
			case 'q':
				bit = blackQueenSide
			default:
				return p, errors.New("invalid castling rights")
			}
			if p.castling&bit != 0 {
				return p, errors.New("duplicate castling right")
			}
			p.castling |= bit
		}
	}
	if fields[3] != "-" {
		ep, ok := square(fields[3])
		if !ok || (fields[3][1] != '3' && fields[3][1] != '6') {
			return p, errors.New("invalid en passant square")
		}
		p.enPassant = ep
	}
	var err error
	p.halfmove, err = strconv.Atoi(fields[4])
	if err != nil || p.halfmove < 0 {
		return p, errors.New("invalid halfmove clock")
	}
	p.fullmove, err = strconv.Atoi(fields[5])
	if err != nil || p.fullmove < 1 {
		return p, errors.New("invalid fullmove number")
	}
	return p, nil
}

func (p position) fen() string {
	var b strings.Builder
	for row := 0; row < 8; row++ {
		if row > 0 {
			b.WriteByte('/')
		}
		empty := 0
		for col := 0; col < 8; col++ {
			ch := p.board[row*16+col]
			if ch == '.' {
				empty++
				continue
			}
			if empty > 0 {
				b.WriteByte(byte('0' + empty))
				empty = 0
			}
			b.WriteByte(ch)
		}
		if empty > 0 {
			b.WriteByte(byte('0' + empty))
		}
	}
	b.WriteByte(' ')
	b.WriteByte(p.turn)
	b.WriteByte(' ')
	if p.castling == 0 {
		b.WriteByte('-')
	} else {
		if p.castling&whiteKingSide != 0 {
			b.WriteByte('K')
		}
		if p.castling&whiteQueenSide != 0 {
			b.WriteByte('Q')
		}
		if p.castling&blackKingSide != 0 {
			b.WriteByte('k')
		}
		if p.castling&blackQueenSide != 0 {
			b.WriteByte('q')
		}
	}
	b.WriteByte(' ')
	if p.enPassant < 0 {
		b.WriteByte('-')
	} else {
		b.WriteString(squareName(p.enPassant))
	}
	fmt.Fprintf(&b, " %d %d", p.halfmove, p.fullmove)
	return b.String()
}

func (m move) uci() string {
	s := squareName(m.from) + squareName(m.to)
	if m.promotion != 0 {
		s += string(m.promotion)
	}
	return s
}

// LegalMoves reports all legal moves for the player to move, in UCI notation.
func LegalMoves(fen string) ([]string, error) {
	p, err := parseFEN(fen)
	if err != nil {
		return nil, err
	}
	moves := p.legalMoves()
	result := make([]string, 0, len(moves))
	for _, m := range moves {
		result = append(result, m.uci())
	}
	return result, nil
}

// ApplyUCI rejects illegal moves and returns the resulting complete FEN.
// Caller must atomically read, validate and update the game under a write lock.
func ApplyUCI(fen, uci string) (string, error) {
	p, err := parseFEN(fen)
	if err != nil {
		return "", err
	}
	if len(uci) != 4 && len(uci) != 5 {
		return "", errors.New("UCI move must contain four or five characters")
	}
	if _, ok := square(uci[:2]); !ok {
		return "", errors.New("invalid source square")
	}
	if _, ok := square(uci[2:4]); !ok {
		return "", errors.New("invalid destination square")
	}
	if len(uci) == 5 && !strings.ContainsRune("qrbn", rune(uci[4])) {
		return "", errors.New("invalid promotion piece")
	}
	for _, m := range p.legalMoves() {
		if m.uci() == uci {
			return p.apply(m).fen(), nil
		}
	}
	return "", errors.New("illegal move")
}

// Outcome is based on the current position. Repetition and claimable 50-move
// draws require game history and must be tracked by the server separately.
// Values: ongoing, check, checkmate, stalemate, insufficient_material, draw_75_moves.
func Outcome(fen string) (string, error) {
	p, err := parseFEN(fen)
	if err != nil {
		return "", err
	}
	moves := p.legalMoves()
	if len(moves) == 0 {
		if p.inCheck(p.turn) {
			return "checkmate", nil
		}
		return "stalemate", nil
	}
	if p.insufficientMaterial() {
		return "insufficient_material", nil
	}
	if p.halfmove >= 150 {
		return "draw_75_moves", nil
	}
	if p.inCheck(p.turn) {
		return "check", nil
	}
	return "ongoing", nil
}

func (p position) inCheck(turn byte) bool {
	king := byte('K')
	if turn == 'b' {
		king = 'k'
	}
	for i := 0; i < 128; i++ {
		if validSquare(i) && p.board[i] == king {
			return p.attacked(i, opponent(turn))
		}
	}
	return true // parseFEN prevents missing kings
}

func (p position) attacked(target int, by byte) bool {
	for from := 0; from < 128; from++ {
		if !validSquare(from) || color(p.board[from]) != by {
			continue
		}
		kind := lower(p.board[from])
		if kind == 'p' {
			dir := 16
			if by == 'w' {
				dir = -16
			}
			if target == from+dir-1 || target == from+dir+1 {
				return true
			}
			continue
		}
		offsets := kingOffsets[:]
		switch kind {
		case 'n':
			offsets = knightOffsets[:]
		case 'b':
			offsets = bishopOffsets[:]
		case 'r':
			offsets = rookOffsets[:]
		case 'q':
			offsets = append(append([]int{}, bishopOffsets[:]...), rookOffsets[:]...)
		}
		for _, d := range offsets {
			for to := from + d; validSquare(to); to += d {
				if to == target {
					return true
				}
				if p.board[to] != '.' || kind == 'k' || kind == 'n' {
					break
				}
			}
		}
	}
	return false
}

func (p position) pseudoMoves() []move {
	moves := make([]move, 0, 64)
	add := func(from, to int, ep, castle bool) {
		if lower(p.board[to]) == 'k' {
			return
		} // kings cannot be captured
		if lower(p.board[from]) == 'p' && (to>>4 == 0 || to>>4 == 7) {
			for _, promotion := range "qrbn" {
				moves = append(moves, move{from: from, to: to, promotion: byte(promotion), enPassant: ep})
			}
		} else {
			moves = append(moves, move{from: from, to: to, enPassant: ep, castle: castle})
		}
	}
	for from := 0; from < 128; from++ {
		if !validSquare(from) || color(p.board[from]) != p.turn {
			continue
		}
		kind := lower(p.board[from])
		if kind == 'p' {
			dir, home := 16, 1
			if p.turn == 'w' {
				dir, home = -16, 6
			}
			to := from + dir
			if validSquare(to) && p.board[to] == '.' {
				add(from, to, false, false)
				two := to + dir
				if from>>4 == home && validSquare(two) && p.board[two] == '.' {
					add(from, two, false, false)
				}
			}
			for _, delta := range []int{-1, 1} {
				to := from + dir + delta
				if !validSquare(to) {
					continue
				}
				if color(p.board[to]) == opponent(p.turn) {
					add(from, to, false, false)
				} else if to == p.enPassant && p.board[to] == '.' {
					captured := to - dir
					expected := byte('p')
					if p.turn == 'b' {
						expected = 'P'
					}
					if p.board[captured] == expected {
						add(from, to, true, false)
					}
				}
			}
			continue
		}
		offsets := kingOffsets[:]
		switch kind {
		case 'n':
			offsets = knightOffsets[:]
		case 'b':
			offsets = bishopOffsets[:]
		case 'r':
			offsets = rookOffsets[:]
		case 'q':
			offsets = append(append([]int{}, bishopOffsets[:]...), rookOffsets[:]...)
		}
		for _, d := range offsets {
			for to := from + d; validSquare(to); to += d {
				if color(p.board[to]) == p.turn {
					break
				}
				add(from, to, false, false)
				if p.board[to] != '.' || kind == 'k' || kind == 'n' {
					break
				}
			}
		}
		if kind == 'k' {
			p.castlingMoves(from, add)
		}
	}
	return moves
}

func (p position) castlingMoves(from int, add func(int, int, bool, bool)) {
	home := 116 // e1
	rook := byte('R')
	kBit, qBit := whiteKingSide, whiteQueenSide
	if p.turn == 'b' {
		home, rook, kBit, qBit = 4, 'r', blackKingSide, blackQueenSide
	}
	if from != home || p.inCheck(p.turn) {
		return
	}
	enemy := opponent(p.turn)
	if p.castling&kBit != 0 && p.board[from+3] == rook && p.board[from+1] == '.' && p.board[from+2] == '.' && !p.attacked(from+1, enemy) && !p.attacked(from+2, enemy) {
		add(from, from+2, false, true)
	}
	if p.castling&qBit != 0 && p.board[from-4] == rook && p.board[from-1] == '.' && p.board[from-2] == '.' && p.board[from-3] == '.' && !p.attacked(from-1, enemy) && !p.attacked(from-2, enemy) {
		add(from, from-2, false, true)
	}
}

func (p position) legalMoves() []move {
	legal := make([]move, 0, 64)
	for _, m := range p.pseudoMoves() {
		if !p.apply(m).inCheck(p.turn) {
			legal = append(legal, m)
		}
	}
	return legal
}

func (p position) apply(m move) position {
	next := p
	piece := p.board[m.from]
	captured := p.board[m.to]
	next.board[m.from] = '.'
	next.board[m.to] = piece
	if m.promotion != 0 {
		promoted := m.promotion
		if p.turn == 'w' {
			promoted -= 'a' - 'A'
		}
		next.board[m.to] = promoted
	}
	if m.enPassant {
		dir := 16
		if p.turn == 'w' {
			dir = -16
		}
		next.board[m.to-dir] = '.'
	}
	if m.castle {
		rookFrom, rookTo := m.from+3, m.from+1
		if m.to < m.from {
			rookFrom, rookTo = m.from-4, m.from-1
		}
		next.board[rookTo] = next.board[rookFrom]
		next.board[rookFrom] = '.'
	}
	if piece == 'K' {
		next.castling &^= whiteKingSide | whiteQueenSide
	}
	if piece == 'k' {
		next.castling &^= blackKingSide | blackQueenSide
	}
	for sq, bit := range map[int]uint8{119: whiteKingSide, 112: whiteQueenSide, 7: blackKingSide, 0: blackQueenSide} {
		if (m.from == sq && lower(piece) == 'r') || (m.to == sq && lower(captured) == 'r') {
			next.castling &^= bit
		}
	}
	next.enPassant = -1
	if lower(piece) == 'p' && (m.to-m.from == 32 || m.from-m.to == 32) {
		next.enPassant = (m.from + m.to) / 2
	}
	next.halfmove++
	if lower(piece) == 'p' || captured != '.' || m.enPassant {
		next.halfmove = 0
	}
	if p.turn == 'b' {
		next.fullmove++
	}
	next.turn = opponent(p.turn)
	return next
}

func (p position) insufficientMaterial() bool {
	count, knights, bishops := 0, 0, 0
	bishopParity := -1
	allSameBishopColor := true
	for i := 0; i < 128; i++ {
		if !validSquare(i) {
			continue
		}
		piece := lower(p.board[i])
		if piece == '.' || piece == 'k' {
			continue
		}
		count++
		switch piece {
		case 'n':
			knights++
		case 'b':
			bishops++
			parity := ((i >> 4) + (i & 7)) & 1
			if bishopParity < 0 {
				bishopParity = parity
			} else if bishopParity != parity {
				allSameBishopColor = false
			}
		default:
			return false
		}
	}
	return count == 0 || (count == 1 && (knights == 1 || bishops == 1)) || (bishops == count && allSameBishopColor)
}
