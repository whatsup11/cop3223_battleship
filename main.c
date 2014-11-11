//
//  main.c
//  battleship
//
//  Created by Catherine Ninah and Steven Petryk
//  Copyright (c) 2014 Catherine Ninah and Steven Petryk. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define BOARD_SIZE 10
#define MAX_ATTACKS BOARD_SIZE*BOARD_SIZE
#define NUM_SHIP_TYPES 5
#define MAX_NAME_SIZE 100

typedef enum {

  NORTH = 0,
  EAST,
  SOUTH,
  WEST,
  INVALID_DIRECTION

} direction_t;

typedef enum {

  PATROL_BOAT = 2,
  DESTROYER,
  SUBMARINE,
  BATTLESHIP,
  AIRCRAFT_CARRIER

} ship_size_type;

typedef enum {

  MISS = 0,
  HIT

} attack_result;

typedef enum {

  HUNT,
  TARGET

} approach_t;

typedef struct {

  int x, y;

} Point;

typedef struct {

  Point * loc;
  attack_result result;

} Attack;

typedef struct {

  Point * start, * end;
  int size;
  int numHits;
  int isSunken;

} Ship;

typedef struct {

  Ship * ships[5];

} Board;

typedef struct {

  char name[MAX_NAME_SIZE];
  Board * board;
  Attack * attacks[MAX_ATTACKS];
  int numAttacks;

} Player;

typedef struct {

  approach_t approach;

} AIState;

typedef struct {

  Player * comp;
  Player * real;
  AIState * aiState;

} Game;

// Starting the game
Game * game_init(Player *);

// Creation utilities
AIState * ai_init();
Player  * player_create(char[MAX_NAME_SIZE]);
Board   * board_create();
Ship    * ship_create(Point *, Point *, int);
Attack  * attack_create(Point *, attack_result);
Point   * point_create(int, int);
Point   * point_create_random(int, int);

attack_result player_attackPlayer(Player *, Player *, Point *);
Attack * player_getAttackAt(Player *, Point *);

void board_placeShips(Board *);
int board_canPlaceShip(Board *, Point *, Point *);
Ship * board_getShipAt(Board *, Point *);

void displayBoard();

int ai_attack(Game *);
void ai_attackHunt(Game *);
void ai_attackTarget(Game *);
Attack * ai_firstHitInStreak(Player *);

char * randomPunnyName();
Point * getAdjacentPoint(Point *, direction_t);
direction_t directionBetweenPoints(Point *, Point *);
direction_t invertDirection(direction_t);
int lineIntersectsLine(Point *, Point *, Point *, Point *);
int pointIntersectsLine(Point *, Point *, Point *);
int pointsEqual(Point *, Point *);
int clamp(int, int, int);
int isWithin(int, int, int);



/**=============================================================================
//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------
//============================================================================*/

int main() {
  srand((int)time(NULL));

  Game * game = game_init(player_create("Steven"));
  board_placeShips(game->real->board);
  board_placeShips(game->comp->board);

  while(ai_attack(game));

  return 0;
}

Game * game_init(Player * real) {
  Game * game = malloc(sizeof(Game));

  game->real = real;
  game->comp = player_create(randomPunnyName());
  game->aiState = ai_init();

  return game;
}



/**=============================================================================
//------------------------------------------------------------------------------
// Struct Factories
//------------------------------------------------------------------------------
//============================================================================*/

AIState * ai_init() {
  AIState * aiState = malloc(sizeof(AIState));

  aiState->approach = HUNT;

  return aiState;
}

Player * player_create(char name[MAX_NAME_SIZE]) {
  Player * player = malloc(sizeof(Player));

  strcpy(player->name, name);
  player->board = board_create();
  player->numAttacks = 0;

  int i;
  for(i = 0; i < MAX_ATTACKS; i++)
    player->attacks[i] = NULL;

  return player;
}

Board * board_create() {
  Board * board = malloc(sizeof(Board));

  int i;
  for(i = 0; i < NUM_SHIP_TYPES; i++)
    board->ships[i] = NULL; // get rid of garbage

  return board;
}

Ship * ship_create(Point * start, Point * end, int size) {
  Ship * ship = malloc(sizeof(Ship));
  ship->start = start;
  ship->end   = end;
  ship->size  = size;
  ship->isSunken = 0;
  ship->numHits = 0;

  return ship;
}

Attack * attack_create(Point * point, attack_result result) {
  Attack * attack = malloc(sizeof(Attack));
  attack->loc = point;
  attack->result = result;

  return attack;
}

Point * point_create(int x, int y) {
  Point * point = malloc(sizeof(Point));
  point->x = x;
  point->y = y;
  return point;
}

Point * point_create_random(int limitX, int limitY) {
  if(limitX < 0) limitX = BOARD_SIZE;
  if(limitY < 0) limitY = BOARD_SIZE;

  return point_create(rand() % limitX, rand() % limitY);
}



/**=============================================================================
//------------------------------------------------------------------------------
// Player actions
//------------------------------------------------------------------------------
//============================================================================*/

attack_result player_attackPlayer(Player * offense, Player * defense, Point * point) {
  Attack * attack;

  // First, ensure an attack here hasn't already been attempted
  if((attack = player_getAttackAt(offense, point))) {
    return attack->result;
  }

  // Look for a ship at the point of attack
  Ship * ship = board_getShipAt(defense->board, point);
  // If it's a hit, the ship won't be NULL
  attack_result result = ship != NULL ? HIT : MISS;

  attack = attack_create(point, result);
  offense->numAttacks++;

  // Add the attack to the array of attacks
  int i;
  for(i = 0; i < MAX_ATTACKS; i++) {
    if(offense->attacks[i] == NULL) {
      offense->attacks[i] = attack;
      break;
    }
  }

  // If we had a hit, make the ship closer to sinking
  if(result == HIT) {
    ship->numHits++;
    ship->isSunken = (ship->numHits == ship->size);
  }

  return result;
}

Attack * player_getAttackAt(Player * player, Point * point) {
  // Disregard NULL points and points that are outside the game board
  if(point == NULL) return NULL;
  if(!isWithin(point->x, 0, BOARD_SIZE) || !isWithin(point->y, 0, BOARD_SIZE))
    return NULL;

  int i;
  for(i = 0; i < player->numAttacks; i++) {
    Attack * attack = player->attacks[i];

    if(attack->loc->x == point->x && attack->loc->y == point->y) {
      return attack;
    }
  }

  return NULL;
}



/**=============================================================================
//------------------------------------------------------------------------------
// Boards
//------------------------------------------------------------------------------
//============================================================================*/

void board_placeShips(Board * board) {
  int shipSize = 6, i = 0;

  while(shipSize-- >= 2) {
    int limit = BOARD_SIZE-shipSize;
    Point * start = point_create_random(limit, limit);
    Point * end;

    // Horizontal
    if(rand() % 2) end = point_create(start->x+shipSize, start->y);
    // Vertical
    else           end = point_create(start->x, start->y+shipSize);

    if(board_canPlaceShip(board, start, end)) {
      board->ships[i] = ship_create(start, end, shipSize);
      i++;
    }
    else {
      shipSize++;
      continue;
    }
  }
}

int board_canPlaceShip(Board * board, Point * start, Point * end) {
  int i;
  for(i = 0; i < NUM_SHIP_TYPES; i++) {
    if(board->ships[i] != NULL) {
      Ship * ship = board->ships[i];
      if(lineIntersectsLine(ship->start, ship->end, start, end))
        return 0;
    }
  }

  return 1;
}

Ship * board_getShipAt(Board * board, Point * point) {
  int i;
  for(i = 0; i < NUM_SHIP_TYPES; i++) {
    Point * start = board->ships[i]->start;
    Point * end   = board->ships[i]->end;
    if(pointIntersectsLine(point, start, end)) return board->ships[i];
  }

  return NULL;
}



/**=============================================================================
//------------------------------------------------------------------------------
// AI
//------------------------------------------------------------------------------
//============================================================================*/

int ai_attack(Game * game) {
  // Can't make any more moves
  if(game->comp->numAttacks == 100) return 0;

  if(game->aiState->approach == HUNT)
    ai_attackHunt(game);
  else {
//    ai_attackTarget(game);
    return 0;
  }

  return 1;
}

void ai_attackHunt(Game * game) {
  Point * point;

  // Keep on generating random points until we find one we haven't tried to
  // attack before
  while(1) {
    point = point_create_random(-1, -1);
    if(player_getAttackAt(game->comp, point) == NULL) break;
    else free(point);
  }

  // If we have a hit, switch to target mode
  attack_result result = player_attackPlayer(game->comp, game->real, point);
  if(result == HIT) {
    game->aiState->approach = TARGET;
  }
}

void ai_attackTarget(Game * game) {
  Player * comp = game->comp;

  Attack * latestAttack = comp->attacks[comp->numAttacks-1];
  Attack * firstStreak = ai_firstHitInStreak(comp);

  Point * point = NULL;
  int i;

  // If we've only hit the ship in one spot
  if(pointsEqual(latestAttack->loc, firstStreak->loc)) {
    // Check all four sides of the point for valid attacks
    for(i = 0; i < 4; i++) {
      point = getAdjacentPoint(latestAttack->loc, (direction_t)i);
      if(player_getAttackAt(comp, point) == NULL) break;
      else free(point);
    }
  }
  // If we've hit the ship in more than one spot, but the last attack was a
  // miss, we must try again on the other end of the ship
  else if(latestAttack->result == MISS) {
    direction_t direction = directionBetweenPoints(firstStreak->loc, latestAttack->loc);
    point = getAdjacentPoint(firstStreak->loc, invertDirection(direction));
  }
  // If we've hit the ship in more than one spot, and the last attack was a hit,
  // just continue in the same direction.
  else {
    direction_t direction = directionBetweenPoints(firstStreak->loc, latestAttack->loc);
    point = getAdjacentPoint(latestAttack->loc, direction);

    // If doing this runs us off the map, or the point has already been
    // attacked, go back to the first hit of the streak and go the opposite way.
    if(point == NULL || player_getAttackAt(comp, point) != NULL) {
      free(point);
      point = getAdjacentPoint(firstStreak->loc, invertDirection(direction));
    }
  }

  // If none of that works, go back to hunting.
  if(point == NULL) {
    game->aiState->approach = HUNT;
    ai_attackHunt(game);
    return;
  }

  player_attackPlayer(comp, game->real, point);
}

Attack * ai_firstHitInStreak(Player * player) {
  int i;
  Attack ** attacks = player->attacks;

  for(i = player->numAttacks-1; i > 0; i--) {
    if(attacks[i]->result == HIT &&
      (attacks[i-1]->result == MISS || i == player->numAttacks-1))
      return attacks[i];
  }
  return NULL; // didn't find anything
}


/**=============================================================================
//------------------------------------------------------------------------------
// Utils
//------------------------------------------------------------------------------
//============================================================================*/

char * randomPunnyName() {
  char punnyNames[][MAX_NAME_SIZE] = {
    "Sinkin' About You",
    "The Iceburg",
    "Under the C",
    "Aboat Time",
    "Pier Pressure",
    "Moor Often than Knot",
    "Miley's ShipWrecking Ball",
    "Aqua-Holic" };

  char * randomName = malloc(sizeof(char) * MAX_NAME_SIZE);
  strcpy(randomName, punnyNames[rand() % 8]);
  return randomName;
}

Point * getAdjacentPoint(Point * point, direction_t direction) {
  int x = point->x, y = point->y;

  switch(direction) {
    case NORTH: y--; break;
    case EAST:  x++; break;
    case SOUTH: y++; break;
    case WEST:  x--; break;
    default: break;
  }

  if(!isWithin(x, 0, BOARD_SIZE) || !isWithin(y, 0, BOARD_SIZE)) return NULL;
  return point_create(x, y);
}

direction_t directionBetweenPoints(Point * p1, Point * p2) {
  if(p2->y > p1->y) return NORTH;
  if(p2->x > p1->x) return EAST;
  if(p2->y < p1->y) return SOUTH;
  if(p2->x < p1->x) return WEST;
  return INVALID_DIRECTION;
}

direction_t invertDirection(direction_t direction) {
  if(direction == INVALID_DIRECTION) return INVALID_DIRECTION;
  return (direction_t)((direction + 2) % 4);
}

int lineIntersectsLine(Point * s1, Point * e1, Point * s2, Point * e2) {
  // When line 1 is horzontal
  if(s1->x == e1->x) {
    int y;
    for(y = s1->y; y <= e1->y; y++) {
      Point * currentPoint = point_create(s1->x, y);
      int doesIntersect = pointIntersectsLine(currentPoint, s2, e2);
      free(currentPoint);
      if(doesIntersect) return 1;
    }
  }
  // When line 1 is vertical
  else {
    int x;
    for(x = s1->x; x <= e1->x; x++) {
      Point * currentPoint = point_create(x, s1->y);
      int doesIntersect = pointIntersectsLine(currentPoint, s2, e2);
      free(currentPoint);
      if(doesIntersect) return 1;
    }
  }

  return 0;
}

int pointIntersectsLine(Point * point, Point * a, Point * b) {
  int sameX = a->x == point->x && b->x == point->x;
  int sameY = a->y == point->y && b->y == point->y;
  int vertical   = sameX && isWithin(point->y, a->y, b->y);
  int horizontal = sameY && isWithin(point->x, a->x, b->x);

  return vertical || horizontal;
}

int pointsEqual(Point * a, Point * b) {
  return a->x == b->x && a->y == b->y;
}

int clamp(int n, int a, int b) {
  if(a < b) { int tmp = a; a = b; b = tmp; }
  if(a > n) return a;
  if(b < n) return b;
  return n;
}

int isWithin(int n, int a, int b) {
  if(a > b) {
    int tmp = a; b = a; a = tmp;
  }
  return a <= n && n <= b;
}
