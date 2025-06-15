#ifndef REPLAY_H
#define REPLAY_H

/**
 * Start replaying the PGN file at the given path.
 * Blocks until user exits replay (e.g., pressing 'q').
 */
void start_replay(const char* filename);

#endif // REPLAY_H