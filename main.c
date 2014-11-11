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

typedef struct {

  int x, y;

} Point;

typedef struct {

  Point * location;
  attack_result result;

} Attack;

typedef struct {

  Point * start, * end;
  int isSunken;

} Ship;

typedef struct {

  Ship * ships[5];

} Board;

typedef struct {

  char name[30];
  Board * board;
  Attack * attacks[MAX_ATTACKS];

} Player;

typedef struct {

  Player * comp;
  Player * real;

} Game;

// Starting the game
Game * game_init(Player *);

// Creation utilities
Player * player_create(char[20]);
Board  * board_create();
Ship   * ship_create(Point *, Point *);
Attack * attack_create(Point *, attack_result);
Point  * point_create(int x, int y);

attack_result player_attackPlayer(Player *, Player *, Point *);

void board_placeShips(Board *);
int board_canPlaceShip(Board *, Point *, Point *);
Ship * board_shipAt(Board *, Point *);

void displayBoard();

int utils_lineIntersectsLine(Point *, Point *, Point *, Point *);
int utils_pointIntersectsLine(Point *, Point *, Point *);
int utils_isWithin(int, int, int);

//==============================================================================
//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------
//==============================================================================

int main() {
  srand((int)time(NULL));

  Game * game = game_init(player_create("Steven"));
  board_placeShips(game->comp->board);

  player_attackPlayer(game->real, game->comp, game->comp->board->ships[0]->start);

  return 0;
}

//==============================================================================
//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
//==============================================================================

Game * game_init(Player * real) {
  Game * game = malloc(sizeof(Game));

  game->real = real;
  game->comp = player_create("Sinkin' About You");

  return game;
}

Player * player_create(char name[20]) {
  Player * player = malloc(sizeof(Player));

  strcpy(player->name, name);
  player->board = board_create();

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

Ship * ship_create(Point * start, Point * end) {
  Ship * ship = malloc(sizeof(Ship));
  ship->start = start;
  ship->end   = end;
  ship->isSunken = 0;

  return ship;
}

Attack * attack_create(Point * point, attack_result result) {
  Attack * attack = malloc(sizeof(Attack));
  attack->location = point;
  attack->result = result;

  return attack;
}

Point * point_create(int x, int y) {
  Point * point = malloc(sizeof(Point));
  point->x = x;
  point->y = y;
  return point;
}

attack_result player_attackPlayer(Player * offense, Player * defense, Point * point) {
  Ship * ship = board_shipAt(defense->board, point);
  attack_result result = (attack_result)(ship != NULL);

  Attack * attack = attack_create(point, result);

  int i;
  for(i = 0; i < MAX_ATTACKS; i++) {
    if(offense->attacks[i] == NULL) {
      offense->attacks[i] = attack;
      break;
    }
  }

  return result;
}

void board_placeShips(Board * board) {
  int shipSize = 6, i = 0;

  while(shipSize-- >= 2) {
    Point * start = point_create(
      rand() % (BOARD_SIZE-shipSize), rand() % (BOARD_SIZE-shipSize));
    Point * end;

    // Horizontal
    if(rand() % 2) end = point_create(start->x+shipSize, start->y);
    // Vertical
    else           end = point_create(start->x, start->y+shipSize);

    if(board_canPlaceShip(board, start, end)) {
      board->ships[i] = ship_create(start, end);
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
      if(utils_lineIntersectsLine(ship->start, ship->end, start, end))
        return 0;
    }
  }

  return 1;
}

Ship * board_shipAt(Board * board, Point * point) {
  int i;
  for(i = 0; i < NUM_SHIP_TYPES; i++) {
    Point * start = board->ships[i]->start;
    Point * end   = board->ships[i]->end;
    if(utils_pointIntersectsLine(point, start, end)) return board->ships[i];
  }

  return NULL;
}

int utils_lineIntersectsLine(Point * s1, Point * e1, Point * s2, Point * e2) {
  // When line 1 is horzontal
  if(s1->x == e1->x) {
    int y;
    for(y = s1->y; y <= e1->y; y++) {
      Point * currentPoint = point_create(s1->x, y);
      int doesIntersect = utils_pointIntersectsLine(currentPoint, s2, e2);
      free(currentPoint);
      if(doesIntersect) return 1;
    }
  }
  // When line 1 is vertical
  else {
    int x;
    for(x = s1->x; x <= e1->x; x++) {
      Point * currentPoint = point_create(x, s1->y);
      int doesIntersect = utils_pointIntersectsLine(currentPoint, s2, e2);
      free(currentPoint);
      if(doesIntersect) return 1;
    }
  }

  return 0;
}

int utils_pointIntersectsLine(Point * point, Point * a, Point * b) {
  int sameX = a->x == point->x && b->x == point->x;
  int sameY = a->y == point->y && b->y == point->y;
  int vertical   = sameX && utils_isWithin(point->y, a->y, b->y);
  int horizontal = sameY && utils_isWithin(point->x, a->x, b->x);

  return vertical || horizontal;
}

int utils_isWithin(int n, int a, int b) {
  if(a > b) {
    int tmp = a; b = a; a = tmp;
  }
  return a <= n && n <= b;
}
