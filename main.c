// Where the main loop and program entry point exists
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 320

#define GRID_ROWS 16
#define GRID_COLS 16

#define MAX_TOWERS 10
#define MAX_ENEMYS 128

#define MAX_CHECKPOINTS 10

#define MAX_TEXTURES 10

SDL_Texture *sprite_sheet[MAX_TEXTURES];

typedef enum {
	spr_empty = 0,
	spr_ship = 1,
	spr_turret = 2,
	spr_track = 3
} SPRITE_SHEET;

typedef struct {
	int x;
	int y;
} Checkpoint;

typedef struct {
	Checkpoint checkpoints[MAX_CHECKPOINTS];
	int num_checkpoints;
} Course;

typedef struct {
	int x;
	int y;
	int health;
	int damage;
	int next_checkpoint;
	Course *full_course;
} Enemy;

typedef struct {
	int x;
	int y;
	int range;
	int damage;
} Tower;

typedef struct {
	int health;
	Tower towers[MAX_TOWERS];
	int num_towers;
	char grid[GRID_ROWS][GRID_COLS];
	char background[GRID_ROWS][GRID_COLS];
} Player;

typedef struct {
	Enemy enemys[MAX_ENEMYS];
	int num_enemys;
} Opponent;

typedef struct {
	int current_wave;
	int enemys_per_wave;
	int enemys_spawned;
	Uint32 wave_start_time;
	int wave_active;
	Uint32 enemy_spawn_interval;
} WaveManager;

typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Event event;
} SDL_Settings;

// Set up wave manager
void initialize_wave_manager(WaveManager *wave_manager);
// Spawn the enemys for the wave
void spawn_wave_enemys(WaveManager *wave_manager, Opponent *opponent, Course *course);
// Starts up a new wave on command
void start_new_wave(WaveManager *wave_manager, Opponent *opponent);

// Loads all textures into sprite sheet
void load_textures(SDL_Settings *settings);
// Frees all textures created
void free_textures();

// Initialize SDL settings
int init_sdl(SDL_Settings *settings);
// Quit out of SDL
void exit_sdl(SDL_Settings *settings);

// Load sdl texture from file
SDL_Texture *load_texture(SDL_Settings *settings, const char *path);

// Map char to a color
SDL_Texture *map_textures(char c);

// Initialize the player
void initialize_player(Player *player);
// Initialize the opponent
void initialize_opponent(Opponent *opponent);

// Initialize game world
void initialize_grid(Player *player);
// Clear the game world
void clear_grid(Player *player);
// Clears the game world background
void clear_background(Player *player);

// Draw tile on background
void draw_background(Player *player, int x, int y, char tile);

// Place a tower into the game world
void place_tower(Player *player, int x, int y);

// Spawn an enemy into the game world
void spawn_enemy(Opponent *opponent, Course *course, int x, int y);
// Remove an enemy from the world
void remove_enemy(Opponent *opponent, int index);

// Update game
void update_game(Player *player, Opponent *opponent);

// Update enemy
void update_enemy(Player *player, Enemy *enemy);

// Check if an enemy is within range and reduce its health
int tower_attack(Tower *tower, Enemy *enemy);

// Show the current state of the game
void display_grid(Player *player, SDL_Settings *settings);

// Update grid based on game state
void update_grid(Player *player, Opponent *opponent);

int main(void)
{
	SDL_Settings sdl_settings;

	if (!init_sdl(&sdl_settings)) {
		printf("terminate\n");
		return 1;
	}

	load_textures(&sdl_settings);

	Player player;
	Opponent opponent;
	WaveManager wave_manager;
	Course course;
	
	course.num_checkpoints = 4;

	course.checkpoints[0].x = 3;
	course.checkpoints[0].y = 7;
	
	course.checkpoints[1].x = 8;
	course.checkpoints[1].y = 5;
	
	course.checkpoints[2].x = 0;
	course.checkpoints[2].y = 0;
	
	course.checkpoints[3].x = 1;
	course.checkpoints[3].y = 9;

	initialize_player(&player);
	initialize_opponent(&opponent);
	initialize_wave_manager(&wave_manager);

	draw_background(&player, 3, 7, '#');
	draw_background(&player, 8, 5, '#');
	draw_background(&player, 0, 0, '#');
	draw_background(&player, 1, 9, '#');

	place_tower(&player, 4, 5);

	int game_running = 1;
	int turn = 0;
	Uint32 turn_timer = 1000;
	Uint32 last_turn_time = SDL_GetTicks();

	while (game_running) {

		while (SDL_PollEvent(&sdl_settings.event)) {
			if (sdl_settings.event.type == SDL_QUIT) {
				game_running = 0;
			}
		}

		SDL_SetRenderDrawColor(sdl_settings.renderer, 0, 0, 0, 255);
		SDL_RenderClear(sdl_settings.renderer);

		update_grid(&player, &opponent);
		display_grid(&player, &sdl_settings);

		Uint32 current_time = SDL_GetTicks();
		if (current_time - last_turn_time >= turn_timer) {
			update_game(&player, &opponent);

			spawn_wave_enemys(&wave_manager, &opponent, &course);
			start_new_wave(&wave_manager, &opponent);

			turn += 1;
			last_turn_time = current_time;
		}

		SDL_RenderPresent(sdl_settings.renderer);

		if (player.health <= 0) {
			game_running = 0;
			printf("You died!\n");
		}
	}
	
	free_textures();
	exit_sdl(&sdl_settings);

	return 0;
}

// Set up wave manager
void initialize_wave_manager(WaveManager *wave_manager)
{
	wave_manager->current_wave = 1;
	wave_manager->enemys_per_wave = 5;
	wave_manager->enemys_spawned = 0;
	wave_manager->wave_start_time = SDL_GetTicks();
	wave_manager->wave_active = 0;
	wave_manager->enemy_spawn_interval = 2000;
}

// Spawn the enemys for the wave
void spawn_wave_enemys(WaveManager *wave_manager, Opponent *opponent, Course *course)
{
	Uint32 current_time = SDL_GetTicks();

	if (wave_manager->wave_active) {
		if (wave_manager->enemys_spawned < wave_manager->enemys_per_wave &&
			current_time - wave_manager->wave_start_time >= wave_manager->enemy_spawn_interval) {
			int start_x = course->checkpoints[0].x;
			int start_y = course->checkpoints[0].y;

			spawn_enemy(opponent, course, start_x, start_y);

			wave_manager->enemys_spawned += 1;
			wave_manager->wave_start_time = current_time;
		}
	}

	if (wave_manager->enemys_spawned >= wave_manager->enemys_per_wave &&
		opponent->num_enemys == 0) {
		wave_manager->wave_active = 0;
	}
}

// Starts up a new wave on command
void start_new_wave(WaveManager *wave_manager, Opponent *opponent)
{
	if (!wave_manager->wave_active && opponent->num_enemys == 0) {
		wave_manager->current_wave++;
		wave_manager->enemys_per_wave += 2;
		wave_manager->enemys_spawned = 0;
		wave_manager->wave_start_time = SDL_GetTicks();
		wave_manager->wave_active = 1;

		printf("Wave %d started!\n", wave_manager->current_wave);
	}
}

// Loads all textures into sprite sheet
void load_textures(SDL_Settings *settings)
{
	sprite_sheet[spr_empty] = load_texture(settings, "src/empty.png");
	sprite_sheet[spr_ship] = load_texture(settings, "src/ship.png");
	sprite_sheet[spr_turret] = load_texture(settings, "src/turret.png");
	sprite_sheet[spr_track] = load_texture(settings, "src/track.png");
}

// Frees all textures created
void free_textures()
{
	SDL_DestroyTexture(sprite_sheet[spr_empty]);
	SDL_DestroyTexture(sprite_sheet[spr_ship]);
	SDL_DestroyTexture(sprite_sheet[spr_turret]);
	SDL_DestroyTexture(sprite_sheet[spr_track]);
	IMG_Quit();
}

// Initialize SDL settings
int init_sdl(SDL_Settings *settings)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		printf("init_sdl error: %s\n", SDL_GetError());
		return 0;
	}

	settings->window = SDL_CreateWindow(
		"Window",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		WINDOW_WIDTH, WINDOW_HEIGHT,
		SDL_WINDOW_SHOWN
	);
	if (!settings->window) {
		printf("init_sdl error: %s\n", SDL_GetError());
		SDL_Quit();
		return 0;
	}

	settings->renderer = SDL_CreateRenderer(
		settings->window,
		-1,
		SDL_RENDERER_ACCELERATED
	);
	if (!settings->renderer) {
		printf("init_sdl error: %s\n", SDL_GetError());
		SDL_DestroyWindow(settings->window);
		SDL_Quit();
		return 0;
	}

	if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
		printf("Failed to init sdl_img: %s\n", IMG_GetError());
		exit_sdl(settings);
		return 0;
	}
 
	return 1;
}

// Quit out of SDL
void exit_sdl(SDL_Settings *settings)
{
	SDL_DestroyRenderer(settings->renderer);
	SDL_DestroyWindow(settings->window);
	SDL_Quit();
}

// Load sdl texture from file
SDL_Texture *load_texture(SDL_Settings *settings, const char *path)
{
	SDL_Surface *surface = IMG_Load(path);
	if (!surface) {
		printf("Failed to load image: %s\n", IMG_GetError());
		return NULL;
	}

	SDL_Texture *texture = SDL_CreateTextureFromSurface(
		settings->renderer, surface
	);
	SDL_FreeSurface(surface);

	if (!texture) {
		printf("Failed to create texture: %s\n", SDL_GetError());
	}

	return texture;
}

// Map char to a color
SDL_Texture *map_textures(char c)
{
	switch (c) {
	case '.':
		return sprite_sheet[spr_empty];
		break;
	case 'T':
		return sprite_sheet[spr_turret];
		break;
	case 'E':
		return sprite_sheet[spr_ship];
		break;
	case '#':
		return sprite_sheet[spr_track];
		break;
	default:
		return sprite_sheet[spr_empty];
		break;
	}
}

// Initialize the player
void initialize_player(Player *player)
{
	initialize_grid(player);
	player->health = 5;
	player->num_towers = 0;
}

// Initialize the opponent
void initialize_opponent(Opponent *opponent)
{
	opponent->num_enemys = 0;
}

// Initialize game world
void initialize_grid(Player *player)
{
	clear_background(player);
	clear_grid(player);
}

// Clear the game world
void clear_grid(Player *player)
{
	for (int i = 0; i < GRID_ROWS; i++) {
		for (int j = 0; j < GRID_COLS; ++j) {
			player->grid[i][j] = player->background[i][j];
		}
	}
}

// Clears the game world background
void clear_background(Player *player)
{
	for (int i = 0; i < GRID_ROWS; i++) {
		for (int j = 0; j < GRID_COLS; ++j) {
			player->background[i][j] = '.';
		}
	}
}

// Draw tile on background
void draw_background(Player *player, int x, int y, char tile)
{
	player->background[x][y] = tile;
}

// Place a tower into the game world
void place_tower(Player *player, int x, int y)
{
	player->towers[player->num_towers].x = x;
	player->towers[player->num_towers].y = y;
	player->towers[player->num_towers].range = 4;
	player->towers[player->num_towers].damage = 4;

	player->num_towers += 1;
}

// Spawn an enemy into the game world
void spawn_enemy(Opponent *opponent, Course *course, int x, int y)
{
	if (opponent->num_enemys < MAX_ENEMYS) {
		opponent->enemys[opponent->num_enemys].x = x;
		opponent->enemys[opponent->num_enemys].y = y;
		opponent->enemys[opponent->num_enemys].health = 50;
		opponent->enemys[opponent->num_enemys].damage = 70;

		opponent->enemys[opponent->num_enemys].full_course = course;
		opponent->enemys[opponent->num_enemys].next_checkpoint = 0;

		opponent->num_enemys += 1;
	} else {
		printf("Max enemys reached.\n");
	}
}

// Remove an enemy from the world
void remove_enemy(Opponent *opponent, int index)
{
	if (index >= 0 && index < opponent->num_enemys) {
		for (int i = 0; i < opponent->num_enemys; i++) {
			opponent->enemys[i] = opponent->enemys[i + 1];
		}
		opponent->num_enemys -= 1;
	}
}

// Update game
void update_game(Player *player, Opponent *opponent)
{
	for (int i = 0; i < opponent->num_enemys; i++) {
		Enemy *enemy = &opponent->enemys[i];

		if (enemy->health != -1) {
			update_enemy(player, enemy);

			for (int j = 0; j < player->num_towers; j++) {
				Tower *tower = &player->towers[j];

				tower_attack(tower, enemy);
			}
		}
		else
		{
			remove_enemy(opponent, i);
		}
	}
}

// Update grid based on game state
void update_grid(Player *player, Opponent *opponent)
{
	clear_grid(player);

	for (int i = 0; i < opponent->num_enemys; i++) {
		Enemy *e = &opponent->enemys[i];
		if (e->health != -1) {
			player->grid[e->x][e->y] = 'E';
		}
	}

	for (int j = 0; j < player->num_towers; j++) {
		Tower *tower = &player->towers[j];
		player->grid[tower->x][tower->y] = 'T';
	}
}

// Update enemy
void update_enemy(Player *player, Enemy *enemy)
{
	player->grid[enemy->x][enemy->y] = '.';

	Checkpoint next_checkpoint = enemy->full_course->checkpoints[enemy->next_checkpoint];

	if (enemy->x > next_checkpoint.x) {
		enemy->x--;
	}
	else if (enemy->x < next_checkpoint.x) {
		enemy->x++;
	}

	if (enemy->y > next_checkpoint.y) {
		enemy->y--;
	}
	else if (enemy->y < next_checkpoint.y) {
		enemy->y++;
	}

	if (enemy->x == next_checkpoint.x && enemy->y == next_checkpoint.y) {
		// Reached checkpoint
		if (enemy->full_course->num_checkpoints != enemy->next_checkpoint + 1) {
			enemy->next_checkpoint += 1;
		}
		else
		{
			enemy->health = -1;
			player->health -= enemy->damage;
		}
	}

	player->grid[enemy->x][enemy->y] = 'E';
}

// Check if an enemy is within range and reduce its health
int tower_attack(Tower *tower, Enemy *enemy)
{
	int distance = abs(tower->x - enemy->x) + abs(tower->y - enemy->y);
	if (distance <= tower->range && enemy->health > 0) {
		enemy->health -= tower->damage;
		if (enemy->health <= 0) {
			printf("Enemy [%d, %d] defeated.\n", enemy->x, enemy->y);
			enemy->health = -1;
			return 1;
		}
	}
	return 0;
}

// Show the current state of the game
void display_grid(Player *player, SDL_Settings *settings)
{
	int square_width = WINDOW_WIDTH / GRID_COLS;
	int square_height = WINDOW_HEIGHT / GRID_ROWS;

	for (int row = 0; row < GRID_ROWS; row++) {
		for (int col = 0; col < GRID_COLS; col++) {
			SDL_Texture *texture = map_textures(player->grid[col][row]);

			SDL_Rect rect = {
				col * square_width,
				row * square_height,
				square_width,
				square_height
			};

			SDL_RenderCopy(settings->renderer, texture, NULL, &rect);
		}
	}
}
