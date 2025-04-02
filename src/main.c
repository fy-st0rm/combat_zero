#define BASE_IMPLEMENTATION
#include "base.h"

// :const
#define WIN_WIDTH  800
#define WIN_HEIGHT 600
#define FPS 60

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
typedef struct {
	v3 pos;
	v2 size;
	Rect rect;

	// physics members
	v2 vel;
	v2 acc;
	f32 airtime;
} Entity;

// :physics def
void physics_compute(Entity* ent, f64 dt);
void physics_resolve(Entity* ent, Rect* rects, i32 rects_cnt, f64 dt);

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
					.duration = 100.0f * 4
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
					.duration = 100.0f * 3
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

// :physics impl
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
}

void physics_resolve(Entity* ent, Rect* rects, i32 rects_cnt, f64 dt) {
	// X-axis collision resolution
	{
		ent->pos.x += ent->vel.x * dt;

		// Loop through all collision bodies
		for (i32 i = 0; i < rects_cnt; i++) {
			Rect rect = rects[i];
			Rect target = rect_with_offset(
				(v2) { ent->pos.x, ent->pos.y },
				ent->rect
			);

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
			Rect target = rect_with_offset(
				(v2) { ent->pos.x, ent->pos.y },
				ent->rect
			);

			// Resolution
			if (rect_intersect(target, rect)) {
				if (ent->vel.y > 0) {
					f32 dy = target.y + target.h - rect.y;
					ent->pos.y -= dy;

					// Reset airtime when on the ground
					ent->airtime = 0;
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
			.near = 0.0f,
			.far = 1000.0f,
		}
	);
	SpriteManager sm = load_sprites();

	Animator am = sm.animators[E_SAMURAI];
	animator_switch_frame(&am, IDLE);

	Entity player = {
		.pos = (v3) { 100, 0, 0 },
		.size = (v2) { 64, 64 },
		.rect = (Rect) { 0, 0, 64, 64},
		.acc = (v2) { 0, 0 },
		.vel = (v2) { 0, 0 },
	};

	b32 left, right, up, down;

	Rect rects[] = {
		{ 0, 400, 300, 100 },
		{ 300, 400, 500, 100 },
		{ 700, 300, 50, 100 },
		{ 700, 100, 50, 200 },
	};
	i32 rects_cnt = sizeof(rects) / sizeof(rects[0]);

	while (!window.should_close) {
		frame_controller_start(&fc);

		Event event;
		while (event_poll(window, &event)) {
			if (event.type == KEYDOWN) {
				switch (event.e.key) {
					case GLFW_KEY_W:
						up = true;
						break;
					case GLFW_KEY_A:
						left = true;
						break;
					case GLFW_KEY_S:
						down = true;
						break;
					case GLFW_KEY_D:
						right = true;
						break;
				}
			}
			else if (event.type == KEYUP) {
				switch (event.e.key) {
					case GLFW_KEY_W:
						up = false;
						break;
					case GLFW_KEY_A:
						left = false;
						break;
					case GLFW_KEY_S:
						down = false;
						break;
					case GLFW_KEY_D:
						right = false;
						break;
				}
			}
		}

		if (up && player.airtime < AIRTIME_THRESHOLD) player.acc.y -= JUMP_ACC;
		if (left)  player.acc.x -= SPEED;
		if (right) player.acc.x += SPEED;

		physics_compute(&player, fc.dt);
		physics_resolve(&player, rects, rects_cnt, fc.dt);

		m4 mvp = ocamera_calc_mvp(&camera);
		imr_update_mvp(&imr, mvp);
		imr_begin(&imr);
		{
			imr_clear((v4) { .5f, .5f, .5f, 1.0f });

			Rect frame = animator_get_frame(&am);
			// imr_push_quad_tex(
			// 	&imr,
			// 	player.pos,
			// 	player.size,
			// 	frame,
			// 	sm.sprites[E_SAMURAI].id,
			// 	rotate_x(0),
			// 	(v4) {1,1,1,1}
			// );
			imr_push_quad(
				&imr,
				player.pos,
				player.size,
				rotate_x(0),
				(v4) {1,1,0,1}
			);

			for (i32 i = 0; i < rects_cnt; i++) {
				Rect r = rects[i];
				v3 pos = { r.x, r.y, 0 };
				v2 size = { r.w, r.h };
				imr_push_quad(
					&imr,
					pos,
					size,
					rotate_x(0),
					(v4) {pos.x / WIN_WIDTH, pos.y / WIN_HEIGHT, i / rects_cnt,1}
				);
			}
		}
		imr_end(&imr);

		window_update(&window);
		frame_controller_end(&fc);
		// printf("FPS: %d\n", fc.fps);
	}

	delete_sprites(&sm);
	imr_delete(&imr);
	window_delete(window);

	ctx_end();
	return 0;
}
