#include "protocol_utils.h"

// 색상 변환: 내부 -> 프로토콜
Team team_to_proto(team_t team) {
    switch (team) {
        case TEAM_WHITE:
            return TEAM__TEAM_WHITE;
        case TEAM_BLACK:
            return TEAM__TEAM_BLACK;
        default:
            return TEAM__TEAM_WHITE;  // 기본값
    }
}

// 색상 변환: 프로토콜 -> 내부
team_t proto_to_team(Team proto_team) {
    switch (proto_team) {
        case TEAM__TEAM_WHITE:
            return TEAM_WHITE;
        case TEAM__TEAM_BLACK:
            return TEAM_BLACK;
        default:
            return TEAM_WHITE;  // 기본값
    }
}

// 기물 타입 변환: 내부 -> 프로토콜
PieceType piece_type_to_proto(piece_type_t type) {
    switch (type) {
        case PIECE_PAWN:
            return PIECE_TYPE__PT_PAWN;
        case PIECE_KNIGHT:
            return PIECE_TYPE__PT_KNIGHT;
        case PIECE_BISHOP:
            return PIECE_TYPE__PT_BISHOP;
        case PIECE_ROOK:
            return PIECE_TYPE__PT_ROOK;
        case PIECE_QUEEN:
            return PIECE_TYPE__PT_QUEEN;
        case PIECE_KING:
            return PIECE_TYPE__PT_KING;
        default:
            return PIECE_TYPE__PT_NONE;
    }
}

// 기물 타입 변환: 프로토콜 -> 내부
piece_type_t proto_to_piece_type(PieceType proto_type) {
    switch (proto_type) {
        case PIECE_TYPE__PT_PAWN:
            return PIECE_PAWN;
        case PIECE_TYPE__PT_KNIGHT:
            return PIECE_KNIGHT;
        case PIECE_TYPE__PT_BISHOP:
            return PIECE_BISHOP;
        case PIECE_TYPE__PT_ROOK:
            return PIECE_ROOK;
        case PIECE_TYPE__PT_QUEEN:
            return PIECE_QUEEN;
        case PIECE_TYPE__PT_KING:
            return PIECE_KING;
        default:
            return PIECE_PAWN;  // 기본값
    }
}