#define BASE_IMPLEMENTATION
#include "base.h"

extern Context ctx;

// :flags
// #define RENDER_RECTS
// #define RENDER_HITRANGE

// :const
#define WIN_WIDTH  1280
#define WIN_HEIGHT 720
#define FPS 60

// Character constants
#define CHAR_SCALE 2
const v2 CHAR_SIZE = { CHAR_SCALE * 64, CHAR_SCALE * 64 };
const Rect CHAR_RECT = {
	.x = CHAR_SCALE * 19,
	.y = CHAR_SCALE * 19,
	.w = CHAR_SCALE * 25,
	.h = CHAR_SCALE * 30,
};

// Colors
const v4 PLAYER_TINT = { 1, 1, 1, 1 };
const v4 ENEMY_TINT  = { 0.2, 0.5, 1, 1 };
const v4 HIT_TINT    = { 1, 0, 0, 1 };

// Combat constants
#define HIT_RANGE 80.0f
#define ATK_COOLDOWN 10.0f
#define ATK_COOLDOWN_RATE 0.8f
#define KNOCKBACK 50000.0f
#define STUN_TIMEOUT 10.0f
#define STUN_TIMEOUT_RATE 0.5f

// Physics constants
#define SPEED 10000.0f
#define JUMP_ACC 30000.0f
#define GRAVITY_ACC 2000.0f
#define VERT_ACC_THRESHOLD 50000.0f
#define AIR_FRICTION 0.95f
#define GROUND_FRICTION 0.5f
#define AIRTIME_THRESHOLD 50.0f
#define AIRTIME_RATE 10.0f

// :utils
#define DA_START_CAP 50
#define da_append(da, value) do {                                                   \
	if ((da)->count + 1 >= (da)->capacity) {                                          \
		if ((da)->capacity <= 0) (da)->capacity = DA_START_CAP;                         \
		(da)->capacity *= 2;                                                            \
		(da)->items = mem_realloc((da)->items, (da)->capacity * sizeof(*(da)->items));  \
	}                                                                                 \
	(da)->items[(da)->count++] = (value);                                             \
} while(0);                                                                         \

/*
 * ---------------
 *   DEFINITIONS
 * ---------------
 */

// :ids
typedef enum {
	E_SAMURAI,
	ENTITY_CNT,
} EntityID;

typedef enum {
	IDLE,
	WALK,
	ASCENT,
	DESCENT,
	SWING_1,
	SWING_2,
	DEATH,
} AnimationID;

// :sprite def
typedef struct {
	EntityID id;
	const char* path;
	i32 x_cnt;
	i32 y_cnt;
} SpriteSheet;

static const SpriteSheet SPRITES[] = {
	{ E_SAMURAI, "assets/samurai.png", 14, 8 },
};
#define SPRITES_CNT sizeof(SPRITES) / sizeof(SPRITES[0])

// :animator def
typedef struct {
	Rect* items;
	i32 count;
	i32 capacity;
} Frames;

typedef struct {
	i32 id;
	f32 duration;
	i32 curr_frame;
	Frames frames;
} AnimationEntry;

typedef struct {
	i32 curr_state;
	f64 start_time;

	AnimationEntry* entries;
	i32 entries_cnt;
} Animator;

Animator animator_new(AnimationEntry* entries, i32 entries_cnt, i32 starting_state);
void animator_delete(Animator* animator);
AnimationEntry* animator_get_entry(Animator* animator, i32 state);
void animator_switch_frame(Animator* animator, i32 state);
Rect animator_get_frame(Animator* animator);

// :spritemanager def
typedef struct {
	// This is the size of entity and not the sprite count
	// as the texture is accessed through the entity id
	Texture sprites[ENTITY_CNT];
	Animator animators[ENTITY_CNT];
} SpriteManager;

SpriteManager load_sprites();
void delete_sprites(SpriteManager* sm);

// :entity def
typedef enum {
	UP,
	LEFT,
	RIGHT,
	DIRS,
} Dir;

typedef enum {
	JS_ASCENT,
	JS_DESCENT,
	JS_STILL,
} JumpState;

typedef struct {
	v3 pos;
	v2 size;
	Rect rect;

	// gameplay
	b32 attack;
	b32 can_atk;
	b32 try_atk;
	f32 atk_cooldown;
	AnimationID prev_atk_frame;

	b32 hit;
	f32 stun_timeout;

	// render
	Texture texture;

	// animation
	AnimationID anim_state;
	Animator animator;
	Dir face;
	JumpState jump_state;

	// physics
	v2 vel;
	v2 acc;
	f32 airtime;
	b32 move[DIRS];
} Entity;

Rect entity_get_rect(Entity* ent);

// :physics def
void physics_movement(Entity* ent, f64 dt);
void physics_compute(Entity* ent, f64 dt);
void physics_resolve(Entity* ent, Rect* rects, i32 rects_cnt, f64 dt);

// :char def
Rect char_get_hitbox(Entity* ent);
void char_update(Entity* ent, Entity* other, Rect* rects, i32 rects_cnt, f64 dt);
void char_render(Entity* ent, IMR* imr, v4 tint);

// :player def
Entity* player_new(SpriteManager* sm);
void player_controller(Entity* ent, Event event);
void player_update(Entity* ent, Entity* enemy, Rect* rects, i32 rects_cnt, f64 dt);

// :enemy def
Entity* enemy_new(SpriteManager* sm);
void enemy_update(Entity* ent, Entity* player, Rect* rects, i32 rects_cnt, f64 dt);

/*
 * -------------------
 *   IMPLEMENTATIONS
 * -------------------
 */

// :animator impl
Animator animator_new(AnimationEntry* entries, i32 entries_cnt, i32 starting_state) {
	return (Animator) {
		.curr_state = starting_state,
		.entries = entries,
		.entries_cnt = entries_cnt,
		.start_time = glfwGetTime(),
	};
}

void animator_delete(Animator* animator) {
	if (!animator->entries || animator->entries_cnt < 0)
		return;

	for (i32 i = 0; i < animator->entries_cnt; i++) {
		mem_free(animator->entries[i].frames.items);
	}
	mem_free(animator->entries);
}

AnimationEntry* animator_get_entry(Animator* animator, i32 state) {
	for (i32 i = 0; i < animator->entries_cnt; i++) {
		AnimationEntry* entry = &animator->entries[i];
		if (entry->id == state)
			return entry;
	}
	panic(0, "Invalid state provided: %d", state);
}

void animator_switch_frame(Animator* animator, i32 state) {
	if (state == animator->curr_state)
		return;

	animator->curr_state = state;
	animator->start_time = glfwGetTime();

	AnimationEntry* entry = animator_get_entry(animator, state);
	entry->curr_frame = 0;
}

Rect animator_get_frame(Animator* animator) {
	AnimationEntry* entry = animator_get_entry(animator, animator->curr_state);

	f64 dt = (glfwGetTime() - animator->start_time) * 1000;
	if (dt >= entry->duration) {
		animator->start_time = glfwGetTime();
	}
	dt = (glfwGetTime() - animator->start_time) * 1000;

	f64 fpt = entry->frames.count / entry->duration;
	entry->curr_frame = floor(fpt * dt);

	// NOTE: Limiting the current frame index cuz sometimes it crashes due to overflow
	if (entry->curr_frame >= entry->frames.count)
		entry->curr_frame = entry->frames.count - 1;

	panic(
		entry->curr_frame < entry->frames.count,
		"Invalid animation frame: id: %d count: %d",
		entry->curr_frame, entry->frames.count
	);

	return entry->frames.items[entry->curr_frame];
}

// :sprite impl
SpriteManager load_sprites() {
	SpriteManager sm = {0};

	for (i32 i = 0; i < SPRITES_CNT; i++) {
		SpriteSheet sprite = SPRITES[i];
		Texture tex = texture_from_file(sprite.path, false);
		texture_bind(tex);

		// Saving the texture
		sm.sprites[sprite.id] = tex;

		// Loading animations
		switch (sprite.id) {
			case E_SAMURAI: {
				Frames idle_frames = {0};
				for (i32 i = 0; i < 8; i++) {
					da_append(&idle_frames, ((Rect) {
						(f32)i / sprite.x_cnt,
						0.0f / sprite.y_cnt,
						1.0f / sprite.x_cnt,
						1.0f / sprite.y_cnt
					}));
				}
				AnimationEntry idle_entry = {
					.id = IDLE,
					.frames = idle_frames,
					.duration = 100.0f * 8
				};
			
				Frames walk_frames = {0};
				for (i32 i = 0; i < 8; i++) {
					da_append(&walk_frames, ((Rect) {
						(f32)i / sprite.x_cnt,
						1.0f / sprite.y_cnt,
						1.0f / sprite.x_cnt,
						1.0f / sprite.y_cnt
					}));
				}
				AnimationEntry walk_entry = {
					.id = WALK,
					.frames = walk_frames,
					.duration = 100.0f * 8
				};

				Frames ascent_frames = {0};
				for (i32 i = 0; i < 4; i++) {
					da_append(&ascent_frames, ((Rect) {
						(f32)i / sprite.x_cnt,
						4.0f / sprite.y_cnt,
						1.0f / sprite.x_cnt,
						1.0f / sprite.y_cnt
					}));
				}
				AnimationEntry ascent_entry = {
					.id = ASCENT,
					.frames = ascent_frames,
					.duration = 100.0f * 4
				};

				Frames descent_frames = {0};
				for (i32 i = 0; i < 4; i++) {
					da_append(&descent_frames, ((Rect) {
						(f32)i / sprite.x_cnt,
						5.0f / sprite.y_cnt,
						1.0f / sprite.x_cnt,
						1.0f / sprite.y_cnt
					}));
				}
				AnimationEntry descent_entry = {
					.id = DESCENT,
					.frames = descent_frames,
					.duration = 100.0f * 4
				};

				Frames swing_1_frames = {0};
				for (i32 i = 0; i < 4; i++) {
					da_append(&swing_1_frames, ((Rect) {
						(f32)i / sprite.x_cnt,
						2.0f / sprite.y_cnt,
						1.0f / sprite.x_cnt,
						1.0f / sprite.y_cnt
					}));
				}
				AnimationEntry swing_1_entry = {
					.id = SWING_1,
					.frames = swing_1_frames,
					.duration = 50.0f * 4
				};

				Frames swing_2_frames = {0};
				for (i32 i = 0; i < 3; i++) {
					da_append(&swing_2_frames, ((Rect) {
						(f32)i / sprite.x_cnt,
						3.0f / sprite.y_cnt,
						1.0f / sprite.x_cnt,
						1.0f / sprite.y_cnt
					}));
				}
				AnimationEntry swing_2_entry = {
					.id = SWING_2,
					.frames = swing_2_frames,
					.duration = 50.0f * 3
				};

				Frames death_frames = {0};
				for (i32 i = 0; i < 14; i++) {
					da_append(&death_frames, ((Rect) {
						(f32)i / sprite.x_cnt,
						7.0f / sprite.y_cnt,
						1.0f / sprite.x_cnt,
						1.0f / sprite.y_cnt
					}));
				}
				AnimationEntry death_entry = {
					.id = DEATH,
					.frames = death_frames,
					.duration = 100.0f * 14
				};

				AnimationEntry* entries = mem_alloc(sizeof(AnimationEntry) * 7);
				entries[0] = idle_entry;
				entries[1] = walk_entry;
				entries[2] = ascent_entry;
				entries[3] = descent_entry;
				entries[4] = swing_1_entry;
				entries[5] = swing_2_entry;
				entries[6] = death_entry;

				Animator animator = animator_new(entries, 7, IDLE);
				sm.animators[sprite.id] = animator;
			} break;

			default:
				panic(false, "Unhandled entity id");
		}
	}

	return sm;
}

void delete_sprites(SpriteManager* sm) {
	for (i32 i = 0; i < ENTITY_CNT; i++) {
		Texture tex = sm->sprites[i];
		if (tex.id) texture_delete(tex);

		Animator am = sm->animators[i];
		animator_delete(&am);
	}
}

// :entity impl
Rect entity_get_rect(Entity* ent) {
	return rect_with_offset(
		(v2) { ent->pos.x, ent->pos.y },
		ent->rect
	);
}

// :physics impl
void physics_movement(Entity* ent, f64 dt) {
	if (ent->move[UP] &&
		ent->airtime < AIRTIME_THRESHOLD)
		ent->acc.y -= JUMP_ACC;

	if (ent->move[LEFT]) {
		ent->acc.x -= SPEED;
		ent->face = LEFT;
	}

	if (ent->move[RIGHT]) {
		ent->acc.x += SPEED;
		ent->face = RIGHT;
	}
}

void physics_compute(Entity* ent, f64 dt) {
	// Add gravity
	ent->acc.y += GRAVITY_ACC;

	// Cap the vertical acceleration
	if (fabsf(ent->acc.y) > VERT_ACC_THRESHOLD) {
		if (ent->acc.y < 0)
			ent->acc.y = -VERT_ACC_THRESHOLD;
		else
			ent->acc.y = VERT_ACC_THRESHOLD;
	}

	// calculate velocity (v = u + a * t)
	ent->vel = v2_add(
		ent->vel,
		v2_mul_scalar(ent->acc, (f32) dt)
	);

	// Increasing the airtime
	ent->airtime += AIRTIME_RATE;

	// Set the entity movement states
	if (ent->vel.y > 0) {
		ent->jump_state = JS_DESCENT;
	} else if (ent->vel.y < 0) {
		ent->jump_state = JS_ASCENT;
	}
}

void physics_resolve(Entity* ent, Rect* rects, i32 rects_cnt, f64 dt) {
	// X-axis collision resolution
	{
		ent->pos.x += ent->vel.x * dt;

		// Loop through all collision bodies
		for (i32 i = 0; i < rects_cnt; i++) {
			Rect rect = rects[i];
			Rect target = entity_get_rect(ent);

			// Resolution
			if (rect_intersect(target, rect)) {
				if (ent->vel.x > 0) {
					f32 dx = target.x + target.w - rect.x;
					ent->pos.x -= dx;
				} else if (ent->vel.x < 0) {
					f32 dx = rect.x + rect.w - target.x;
					ent->pos.x += dx;
				}
			}
		}
	}

	// Y-axis collision resolution
	{
		ent->pos.y += ent->vel.y * dt;

		// Loop through all collision bodies
		for (i32 i = 0; i < rects_cnt; i++) {
			Rect rect = rects[i];
			Rect target = entity_get_rect(ent);

			// Resolution
			if (rect_intersect(target, rect)) {
				if (ent->vel.y > 0) {
					f32 dy = target.y + target.h - rect.y;
					ent->pos.y -= dy;

					// Reset airtime when on the ground
					ent->airtime = 0;
					ent->jump_state = JS_STILL;
				} else if (ent->vel.y < 0) {
					f32 dy = rect.y + rect.h - target.y;
					ent->pos.y += dy;
				}
			}
		}
	}

	// Applying frictions
	ent->acc = v2_mul_scalar(ent->acc, AIR_FRICTION);
	ent->acc.x *= GROUND_FRICTION;

	// Reset the velocity
	ent->vel = (v2) { 0, 0 };
}

// :char impl
Rect char_get_hitbox(Entity* ent) {
	Rect rect = entity_get_rect(ent);

	Rect hitbox = {
		.y = rect.y,
		.w = HIT_RANGE,
		.h = rect.h,
	};

	if (ent->face == LEFT) {
		hitbox.x = rect.x - HIT_RANGE;
	} else {
		hitbox.x = rect.x + rect.w;
	}

	return hitbox;
}

void char_update(Entity* ent, Entity* other, Rect* rects, i32 rects_cnt, f64 dt) {
	if (ent->try_atk && ent->can_atk && ent->atk_cooldown == 0.0f) {
		ent->attack = true;
	}
	
	Rect hitbox = char_get_hitbox(ent);
	Rect e_rect = entity_get_rect(other);

	// Give knockback and stun to the other entity when attacked
	if (ent->attack && rect_intersect_inclusive(e_rect, hitbox)) {
		if (ent->face == LEFT) {
			other->acc.x -= KNOCKBACK;
		} else {
			other->acc.x += KNOCKBACK;
		}
		other->hit = true;
		other->stun_timeout = STUN_TIMEOUT;
	}

	physics_movement(ent, dt);
	physics_compute(ent, dt);
	physics_resolve(ent, rects, rects_cnt, dt);

#ifdef RENDER_HITRANGE
	imr_push_quad(
		ctx.inner,
		(v3) { hitbox.x, hitbox.y, ent->pos.z },
		(v2) { hitbox.w, hitbox.h },
		rotate_y(0),
		(v4) { 1, 1, 0, 0.5 }
	);
#endif
}

void char_render(Entity* ent, IMR* imr, v4 tint) {
	// If the character is in attack animation
	// then skip the jump and walking animations
	if (!ent->can_atk)
		goto skip_state;

	// Setting up walking animation
	if (ent->move[LEFT] || ent->move[RIGHT]) {
		ent->anim_state = WALK;
	} else {
		ent->anim_state = IDLE;
	}

	// If both left and right movement is on, set it to IDLE
	if (ent->move[LEFT] && ent->move[RIGHT])
		ent->anim_state = IDLE;

	// Handling jump ascent and descent animation
	switch (ent->jump_state) {
		case JS_ASCENT:
			ent->anim_state = ASCENT;
			break;
		case JS_DESCENT:
			ent->anim_state = DESCENT;
			break;
		default: break;
	}
skip_state:

	if (ent->attack) {
		// Toggling between different swings
		if (ent->prev_atk_frame == SWING_1) {
			ent->anim_state = SWING_2;
			ent->prev_atk_frame = SWING_2;
		} else {
			ent->anim_state = SWING_1;
			ent->prev_atk_frame = SWING_1;
		}

		// Stoping further attack by setting the cooldown
		ent->attack = false;
		ent->can_atk = false;
		ent->atk_cooldown = ATK_COOLDOWN;
	}

	// Switch the animation state
	animator_switch_frame(&ent->animator, ent->anim_state);

	// If we are in attack state ie (can_atk = false)
	if (!ent->can_atk) {
		AnimationEntry* entry = animator_get_entry(&ent->animator, ent->anim_state);

		// Set (can_atk = true) when the animation for the swing is complete
		if (entry->curr_frame >= entry->frames.count - 1) {
			ent->can_atk = true;
		}
	}

	// If we can attack again ie animation is complete
	// then we start the cooldown
	// TODO: Multiply rate with (dt) maybe?
	if (ent->can_atk && ent->atk_cooldown > 0.0f) {
		ent->atk_cooldown -= ATK_COOLDOWN_RATE;
		if (ent->atk_cooldown < 0.0f) {
			ent->atk_cooldown = 0.0f;
		}
	}

	// Get the texture coords of current frame
	Rect frame = animator_get_frame(&ent->animator);

	// Handling player rotation
	m4 rot = rotate_y(0);
	switch(ent->face) {
		case LEFT:
			rot = rotate_y(PI);
			break;
		case RIGHT:
			rot = rotate_y(0);
			break;
	}

	// If hit apply the hit tint instead
	if (ent->hit) {
		tint = HIT_TINT;
	}

	// Rendering
	imr_push_quad_tex(
		imr,
		ent->pos,
		ent->size,
		frame,
		ent->texture.id,
		rot,
		tint
	);

	// Debug collider render
#ifdef RENDER_RECTS
	Rect rect = entity_get_rect(ent);
	imr_push_quad(
		imr,
		(v3) { rect.x, rect.y, 0.0f },
		(v2) { rect.w, rect.h },
		rotate_x(0),
		(v4) {1,0,0,0.5f}
	);
#endif
}

// :player impl
Entity* player_new(SpriteManager* sm) {
	Entity* ent = mem_alloc(sizeof(Entity));

	ent->pos = (v3) { 100, 0, 0 };
	ent->size = CHAR_SIZE;
	ent->rect = CHAR_RECT;
	ent->texture = sm->sprites[E_SAMURAI];
	ent->animator = sm->animators[E_SAMURAI];
	ent->face = RIGHT;
	ent->can_atk = true;

	return ent;
}

void player_controller(Entity* ent, Event event) {
	if (event.type == KEYDOWN) {
		switch (event.e.key) {
			case GLFW_KEY_W:
				ent->move[UP] = true;
				break;
			case GLFW_KEY_A:
				ent->move[LEFT] = true;
				break;
			case GLFW_KEY_D:
				ent->move[RIGHT] = true;
				break;
		}
	}
	else if (event.type == KEYUP) {
		switch (event.e.key) {
			case GLFW_KEY_W:
				ent->move[UP] = false;
				break;
			case GLFW_KEY_A:
				ent->move[LEFT] = false;
				break;
			case GLFW_KEY_D:
				ent->move[RIGHT] = false;
				break;
		}
	}
	else if (event.type == MOUSE_BUTTON_DOWN) {
		if (event.e.button == MOUSE_BUTTON_LEFT) {
			ent->try_atk = true;
		}
	}
	else if (event.type == MOUSE_BUTTON_UP) {
		if (event.e.button == MOUSE_BUTTON_LEFT) {
			ent->try_atk = false;
		}
	}
}

void player_update(Entity* ent, Entity* enemy, Rect* rects, i32 rects_cnt, f64 dt) {
	// Update the character stuff first
	char_update(ent, enemy, rects, rects_cnt, dt);

	if (ent->hit) {
		// TODO: Multiply rate with (dt) maybe?
		ent->stun_timeout -= STUN_TIMEOUT_RATE;
		if (ent->stun_timeout <= 0.0f)
			ent->hit = false;
	}
}

// :enemy impl
Entity* enemy_new(SpriteManager* sm) {
	Entity* ent = mem_alloc(sizeof(Entity));

	ent->pos = (v3) { 300, 0, 0 };
	ent->size = CHAR_SIZE;
	ent->rect = CHAR_RECT;
	ent->texture = sm->sprites[E_SAMURAI];
	ent->animator = sm->animators[E_SAMURAI];
	ent->face = LEFT;

	return ent;
}

void enemy_update(Entity* ent, Entity* player, Rect* rects, i32 rects_cnt, f64 dt) {
	// Update the character stuff first
	char_update(ent, player, rects, rects_cnt, dt);

	// If a hit is encountered add a slight stun to movement
	if (ent->hit) {
		ent->move[LEFT] = ent->move[RIGHT] = false;

		// TODO: Multiply rate with (dt) maybe?
		ent->stun_timeout -= STUN_TIMEOUT_RATE;
		if (ent->stun_timeout <= 0.0f)
			ent->hit = false;

		goto skip_movement;
	}

	// When player is on the left side
	if (player->pos.x < ent->pos.x) {
		ent->face = LEFT;
		// Move within the hitrange
		if (ent->pos.x - player->pos.x > HIT_RANGE) {
			ent->move[RIGHT] = false;
			ent->move[LEFT] = true;
		} else {
			ent->move[LEFT] = false;
		}
	}
	// When player is on the right side
	else if (player->pos.x > ent->pos.x) {
		ent->face = RIGHT;
		// Move within the hitrange
		if (player->pos.x - ent->pos.x > HIT_RANGE) {
			ent->move[LEFT] = false;
			ent->move[RIGHT] = true;
		} else {
			ent->move[RIGHT] = false;
		}
	}

	// If the player is in the sky you shall too
	if ((ent->pos.x - player->pos.x < HIT_RANGE) ||
		 (player->pos.x - ent->pos.x < HIT_RANGE)) {
		if (player->pos.y < ent->pos.y) {
			ent->move[UP] = true;
		} else {
			ent->move[UP] = false;
		}
	}
skip_movement:
}

// :main
int main() {
	Window window = window_new("Combat", WIN_WIDTH, WIN_HEIGHT);
	IMR imr = imr_new();
	FrameController fc = frame_controller_new(FPS);
	OCamera camera = ocamera_new(
		(v2) {0,0},
		1,
		(OCamera_Boundary) {
			.left = 0,
			.right = WIN_WIDTH,
			.top = 0,
			.bottom = WIN_HEIGHT,
			.near = -1.0f,
			.far = 1000.0f,
		}
	);
	SpriteManager sm = load_sprites();

	// Set renderer to context for debug rendering
	ctx.inner = &imr;

	// Characters
	Entity* player = player_new(&sm);
	Entity* enemy = enemy_new(&sm);

	Rect rects[] = {
		{ 0, 400, 300, 100 },
		{ 300, 400, 500, 100 },
		{ 700, 300, 50, 100 },
		{ 700, 100, 50, 200 },
	};
	i32 rects_cnt = sizeof(rects) / sizeof(rects[0]);

	// :loop
	while (!window.should_close) {
		frame_controller_start(&fc);

		// :event
		Event event = {0};
		while (event_poll(window, &event)) {
			player_controller(player, event);

			if (event.type == MOUSE_BUTTON_DOWN) {
				if (event.e.button == MOUSE_BUTTON_RIGHT) {
					enemy->try_atk = true;
				}
			}
			else if (event.type == MOUSE_BUTTON_UP) {
				if (event.e.button == MOUSE_BUTTON_RIGHT) {
					enemy->try_atk = false;
				}
			}
		}

		m4 mvp = ocamera_calc_mvp(&camera);
		imr_update_mvp(&imr, mvp);
		imr_clear((v4) { .5f, .5f, .5f, 1.0f });

		imr_begin(&imr);

		// :update
		{
			player_update(player, enemy, rects, rects_cnt, fc.dt);
			enemy_update(enemy, player, rects, rects_cnt, fc.dt);
		}

		// :render
		{
			char_render(player, &imr, PLAYER_TINT);
			char_render(enemy, &imr, ENEMY_TINT);

			for (i32 i = 0; i < rects_cnt; i++) {
				Rect r = rects[i];
				v3 pos = { r.x, r.y, 0 };
				v2 size = { r.w, r.h };
				imr_push_quad(
					&imr,
					pos,
					size,
					rotate_x(0),
					(v4) { 0.1, 0.1, 0.1, 1 }
		//			(v4) {pos.x / WIN_WIDTH, pos.y / WIN_HEIGHT, i / rects_cnt,1}
				);
			}
		}
		
		imr_end(&imr);

		window_update(&window);
		frame_controller_end(&fc);
		// printf("FPS: %d\n", fc.fps);
	}

	// :clean
	mem_free(player);
	mem_free(enemy);

	delete_sprites(&sm);
	imr_delete(&imr);
	window_delete(window);

	ctx_end();
	return 0;
}
