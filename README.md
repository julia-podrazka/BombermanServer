# Bomberman

Implementation of a client and server for a simplified net game - Bomberman in C++.

**Rules of the game:**
- the game is played in turns
- in each turn, every user can make one of the following moves:
  - do nothing
  - move one field (if it's not blocked)
  - put a bomb underneath (with a given timer)
  - block a field underneath
- the number of turns is known from the start
- the game is played recurrently (after gathering enough players, a new game starts)
- there is at least one player in a game.

You can find a detailed description of the game and GUI at: https://github.com/agluszak/mimuw-sik-2022-public.
