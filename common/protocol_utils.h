#ifndef PROTOCOL_UTILS_H
#define PROTOCOL_UTILS_H

#include "generated/message.pb-c.h"
#include "types.h"

// 내부 타입 <-> 프로토콜 타입 변환 함수들

// 색상 변환
Team   team_to_proto(team_t team);
team_t proto_to_team(Team proto_team);

// 기물 타입 변환
PieceType    piece_type_to_proto(piece_type_t type);
piece_type_t proto_to_piece_type(PieceType proto_type);

#endif  // PROTOCOL_UTILS_H