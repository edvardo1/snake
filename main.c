#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

typedef unsigned int uint;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define GFX_TILE_W 64
#define GFX_TILE_H 64

#define GFX_WINDOW_W (1024)
#define GFX_WINDOW_H (768)

SDL_Window *win;
SDL_Renderer *ren;
SDL_Event ev;

typedef enum {UP, DOWN, LEFT, RIGHT} Direction;
typedef struct Input {
	Direction dir;
} Input;

typedef struct Point {
	int x, y;
} Point;

typedef struct Snake {
	Point head;
	int tail_size;
	Direction tail[1024];
} Snake;

#define D2_TO_3(x, y, width) (y * width + x)

typedef unsigned long Color;
typedef struct Game {
	Snake snake;
	Point food;
	Point *new_food_points;

	bool *occupied;
	int width;
	int height;

	bool alive;

	Color alive_snake;
	Color alive_food;
	Color alive_board_1;
	Color alive_board_2;

	Color dead_snake;
	Color dead_food;
	Color dead_board_1;
	Color dead_board_2;
} Game;

void
set_color(unsigned long color) {
	SDL_SetRenderDrawColor(
		ren,
		(color >> 24) & 0xff,
		(color >> 16) & 0xff,
		(color >>  8) & 0xff,
		(color >>  0) & 0xff);
}

void
set_occupied(Game *game, Point *p, bool b) {
	game->occupied[p->y * game->width + p->x] = b;
}

bool
is_occupied(Game *game, Point *p) {
	bool in_x = 0 <= p->x && p->x < game->width;
	bool in_y = 0 <= p->y && p->y < game->height;
	if (in_x && in_y) {
		return game->occupied[p->y * game->width + p->x];
	} else {
		return true;
	}
}

Direction
direction_opposite(Direction dir) {
	switch(dir) {
	case UP    : return DOWN;
	case DOWN  : return UP;
	case LEFT  : return RIGHT;
	case RIGHT : return LEFT;
	}

	assert(0 && "unreachable");
}

void
apply_direction(Point *p, Direction dir) {
	switch(dir) {
	case UP    : p->y -= 1; break;
	case DOWN  : p->y += 1; break;
	case LEFT  : p->x -= 1; break;
	case RIGHT : p->x += 1; break;
	}
}

bool
point_eq(Point *a, Point *b) {
	return (a->x == b->x && a->y == b->y);
}

void
food_eat(Game *game) {
	int new_food_points_n = 0;

	for (int y = 0; y < game->height; y++) {
		for (int x = 0; x < game->width; x++) {
			Point p = {x, y};
			if (!is_occupied(game, &p)) {
				game->new_food_points[new_food_points_n] = p;
				new_food_points_n += 1;
			}
		}
	}

	game->food = game->new_food_points[rand() % new_food_points_n];
}

void
game_die(Game *game) {
	game->alive = false;
	printf("u died\n");
}

void
snake_update(Snake *snake, Game *game, Input *input) {
	Point move_point = snake->head;
	apply_direction(&move_point, input->dir);

	Point occupied_point = snake->head;
	set_occupied(game, &occupied_point, true);
	for (int i = 0; i < game->snake.tail_size; i++) {
		apply_direction(&occupied_point, snake->tail[i]);
		set_occupied(game, &occupied_point, true);
	}

	if (is_occupied(game, &move_point)) {
		/* die */
		game_die(game);
	} else if(point_eq(&move_point, &game->food)) {
		/* grow */
		food_eat(game);
		snake->head = move_point;
		for (int i = snake->tail_size; i >= 1; i--) {
			snake->tail[i] = snake->tail[i - 1];
		}
		snake->tail_size += 1;
		snake->tail[0] = direction_opposite(input->dir);
	} else {
		/* move */
		snake->head = move_point;
		for (int i = snake->tail_size - 1; i >= 1; i--) {
			snake->tail[i] = snake->tail[i - 1];
		}
		snake->tail[0] = direction_opposite(input->dir);
	}

	for (int y = 0; y < game->height; y++) {
		for (int x = 0; x < game->width; x++) {
			Point p = {x, y};
			set_occupied(game, &p, false);
		}
	}
}

void
game_update(Game *game, Input *input) {
	snake_update(&game->snake, game, input);
}

const int init_gfx_tile_h = 20;
const int init_gfx_tile_w = 20;
int gfx_tile_h = init_gfx_tile_w;
int gfx_tile_w = init_gfx_tile_h;
int
get_gfx_tile_w() {
	return gfx_tile_w;
}
int
get_gfx_tile_h() {
	return gfx_tile_h;
}

void
render_point(Point *p) {
	int w = get_gfx_tile_w();
	int h = get_gfx_tile_h();
	SDL_FRect rect = {p->x * w, p->y * h, w, h};
	SDL_RenderFillRect(ren, &rect);
}

void
snake_render(Snake *snake, Game *game) {
	if (game->alive) {
		set_color(game->alive_snake);
	} else {
		set_color(game->dead_snake);
	}
	Point p = snake->head;
	render_point(&p);
	for (int i = 0; i < snake->tail_size; i++) {
		apply_direction(&p, snake->tail[i]);
		render_point(&p);
	}
}

void
game_render(Game *game) {
	for (int y = 0; y < game->height; y++) {
		for (int x = 0; x < game->width; x++) {
			switch ((((x + y) & 1) << 1) | game->alive) {
			case 1 : {
				set_color(game->alive_board_1);
			} break;
			case 3 : {
				set_color(game->alive_board_2);
			} break;
			case 0 : {
				set_color(game->dead_board_1);
			} break;
			case 2 : {
				set_color(game->dead_board_2);
			} break;
			}
			Point p = {x, y};
			render_point(&p);
		}
	}

	if (game->alive) {
		set_color(game->alive_food);
	} else {
		set_color(game->dead_food);
	}

	render_point(&game->food);
	snake_render(&game->snake, game);
}

void
game_reset(Game *game) {
	game->alive = true;

	game->snake.head.x = game->width * 0.4;
	game->snake.head.y = game->height * 0.5;
	game->snake.tail_size = game->width * 0.3;
	game->food.x = game->width * 0.6;
	game->food.y = game->height * 0.5;
	for (int i = 0; i < 80; i++) {
		game->snake.tail[i] = LEFT;
	}
}

void
game_init(Game *game) {
	game->alive_board_1 = 0x484848ff;
	game->alive_board_2 = 0x383838ff;
	game->alive_snake = 0xdead00ff;
	game->alive_food = 0xcc33bbff;

	game->dead_board_1 = 0x884848ff;
	game->dead_board_2 = 0x883838ff;
	game->dead_snake   = 0xff2222ff;
	game->dead_food    = 0xff1111ff;

	game->width = 30;
	game->height = 30;
	game->occupied = calloc(game->width * game->height, sizeof(bool));
	game->new_food_points = calloc(game->width * game->height, sizeof(Point));
	game_reset(game);
}

int
main() {
	int current_tick = 0;
	int last_tick = 0;
	int tick = 0;
	bool running = 1;
	static Game game;
	game_init(&game);
	static Input input;
	Direction newdir = RIGHT;
	input.dir = RIGHT;

	SDL_Init(SDL_INIT_VIDEO);
	win = SDL_CreateWindow("title", 600, 600, 0 | SDL_WINDOW_RESIZABLE);
	assert(win != NULL);
	ren = SDL_CreateRenderer(win, NULL);
	assert(ren != NULL);
	SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

	while(running) {
		current_tick = SDL_GetTicks();
		if(current_tick - last_tick > (1000/60.0)) {
			tick++;
			last_tick = current_tick;

			ev.type = SDL_EVENT_LAST + 1;

			while(SDL_PollEvent(&ev)) {
				switch(ev.type) {
				case SDL_EVENT_WINDOW_RESIZED : {
					int w = ev.window.data1;
					int h = ev.window.data2;
					gfx_tile_h = h * (init_gfx_tile_h/600.0);
					gfx_tile_w = w * (init_gfx_tile_w/600.0);
				} break;
				case SDL_EVENT_QUIT : {
					running = 0;
				} break;
				case SDL_EVENT_KEY_DOWN : {
					switch (ev.key.key) {
					case SDLK_UP : newdir = UP; break;
					case SDLK_DOWN : newdir = DOWN; break;
					case SDLK_LEFT : newdir = LEFT; break;
					case SDLK_RIGHT : newdir = RIGHT; break;
					}
				} break;
				case SDL_EVENT_KEY_UP : {
				} break;
				case SDL_EVENT_MOUSE_MOTION :
				case SDL_EVENT_MOUSE_BUTTON_UP :
				case SDL_EVENT_MOUSE_BUTTON_DOWN :
				default : {
				} break;
				}
			}

			if ((tick % 12) == 0) {
				if (!game.alive) {
					game_reset(&game);
					input.dir = RIGHT;
					newdir = RIGHT;
				}

				if (direction_opposite(input.dir) != newdir) {
					input.dir = newdir;
				}
				game_update(&game, &input);
			}

			SDL_SetRenderDrawColor(ren, 0, 0, 0, 0xff);
			SDL_RenderClear(ren);

			game_render(&game);

			SDL_RenderPresent(ren);
		} else {
			SDL_Delay(1000/60.0 - (current_tick - last_tick));
		}
	}

	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();

	return 0;
}
