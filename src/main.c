#define BASE_IMPLEMENTATION
#include "base.h"

extern Context* ctx;

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
	.h = CHAR_SCALE * 29,
};

// Colors
const v4 PLAYER_TINT = { 0, 0.5, 1, 1 };
const v4 ENEMY_TINT  = { 1, 0, 0.1, 1 };
const v4 DEAD_TINT   = { 1, 0, 0, 1 };
const v4 HIT_OVERLAY = { 1, 1, 1, 0.8 };

// Combat constants
#define HIT_RANGE 80.0f
#define HIT_RANGE_ON_DASH 500.0f
#define HIT_DMG 10.0f;
#define SWING_COOLDOWN 10.0f
#define SWING_COOLDOWN_RATE 0.8f
#define KNOCKBACK 50000.0f
#define STUN_TIMEOUT 10.0f
#define STUN_TIMEOUT_RATE 0.5f
#define CONSEC_ATK_HOLD 2.0
#define MAX_CONSEC_ATK 3.0f

// Enemy constants
#define ENEMY_ATK_COOLDOWN 20.0f
#define ENEMY_ATK_COOLDOWN_RATE 0.1f
#define PLAYER_TOO_CLOSE 200.0f
#define IN_PLAYER_HITZONE HIT_RANGE + 20.0f
#define ENEMY_DASH_PROBABILITY 30

// Physics constants
#define GRAVITY_ACC 2000.0f
#define VERT_ACC_THRESHOLD 50000.0f
#define AIR_FRICTION 0.95f
#define GROUND_FRICTION 0.5f
#define AIRTIME_THRESHOLD 50.0f
#define AIRTIME_RATE 15.0f

// Movement constants
#define SPEED 10000.0f
#define JUMP_ACC 30000.0f
#define DASH_ACC 500000.0f
#define DASH_COOLDOWN 100.0f
#define DASH_COOLDOWN_RATE 0.8f
#define DASH_GHOST_ALPHA 0.7f
#define DASH_GHOST_ALPHA_RATE 0.005f;

// UI stuff
#define PAUSE_BUTTON_WIDTH 50
#define PAUSE_BUTTON_HEIGHT 200

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

static b32 LineIntersectsRect(v2 p0, v2 p1, Rect rect, v2* outHitPoint) {
	float tmin = 0.0f;
	float tmax = 1.0f;
	v2 d = { p1.x - p0.x, p1.y - p0.y };
	
	float minX = rect.x;
	float maxX = rect.x + rect.w;
	float minY = rect.y;
	float maxY = rect.y + rect.h;
	
	for (int i = 0; i < 2; i++) {
		float origin = (i == 0) ? p0.x : p0.y;
		float dir    = (i == 0) ? d.x  : d.y;
		float min    = (i == 0) ? minX : minY;
		float max    = (i == 0) ? maxX : maxY;
		
		if (dir == 0.0f) {
			if (origin < min || origin > max)
				return false; // No hit
		} else {
			float ood = 1.0f / dir;
			float t1 = (min - origin) * ood;
			float t2 = (max - origin) * ood;
			
			if (t1 > t2) {
				float tmp = t1; t1 = t2; t2 = tmp;
			}
			
			if (t1 > tmin) tmin = t1;
			if (t2 < tmax) tmax = t2;
			
			if (tmin > tmax)
				return false; // No overlap
		}
	}
	
	if (tmin < 0.0f || tmin > 1.0f)
		return false; // Intersection not on segment
	
	if (outHitPoint)
		*outHitPoint = v2_add(p0, v2_mul_scalar(d, tmin));
	
	return true;
}

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
	b32 no_repeat;
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

	// combat
	b32 attack;
	b32 try_atk;
	f32 atk_cooldown;

	f32 swing_cooldown;
	b32 is_swing_complete;
	AnimationID prev_atk_frame;

	f64 last_atk_time;
	i32 consec_atk;
	b32 do_consec_atk;

	// dash
	b32 dash;
	b32 try_dash;
	f32 dash_cooldown;
	v3 dash_start_pos;
	v3 dash_end_pos;
	Rect frame_during_dash;
	Dir face_during_dash;
	f32 dash_ghost_alpha;

	// damage
	b32 hit;
	f32 stun_timeout;

	// health
	f32 health;
	b32 dead;

	// render
	Texture texture;

	// animation
	AnimationID anim_state;
	Animator animator;
	Dir face;
	JumpState jump_state;
	Rect curr_frame;

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
void char_handle_atk(Entity* ent, Entity* other);
void char_handle_hit(Entity* ent);
void char_handle_dash(Entity* ent, f64 dt);
void char_render(Entity* ent, IMR* imr, v4 tint);

// :player def
Entity* player_new(SpriteManager* sm);
void player_controller(Entity* ent, Event event);
void player_update(Entity* ent, Entity* enemy, Rect* rects, i32 rects_cnt, f64 dt);

// :enemy def
Entity* enemy_new(SpriteManager* sm);
void enemy_update(Entity* ent, Entity* player, Rect* rects, i32 rects_cnt, f64 dt);

// :ui def
void render_progress_bar(IMR* imr, v3 pos, v2 size, f32 val, f32 max, v4 color);


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

	// Responsible for looping the animation
	if (dt >= entry->duration && !entry->no_repeat) {
		animator->start_time = glfwGetTime();
		dt = (glfwGetTime() - animator->start_time) * 1000;
	}

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
					.duration = 100.0f * 14,
					.no_repeat = true, // Death animation should not repeat
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
		v2 p0, p1;
		Rect target = entity_get_rect(ent);

		// Point infront of the entity before moving
		if (ent->face == RIGHT) {
			p0 = (v2) {
				target.x + target.w,
				target.y + target.h / 2,
			};
		} else {
			p0 = (v2) {
				target.x,
				target.y + target.h / 2
			};
		}

		ent->pos.x += ent->vel.x * dt;

		target = entity_get_rect(ent);

		// Point infront of the entity after moving
		if (ent->face == RIGHT) {
			p1 = (v2) {
				target.x + target.w,
				target.y + target.h / 2,
			};
		} else {
			p1 = (v2) {
				target.x,
				target.y + target.h / 2
			};
		}

		// Loop through all collision bodies
		for (i32 i = 0; i < rects_cnt; i++) {
			Rect rect = rects[i];

			v2 hit = {0};

			// Checking if there is a rect inbetween of those points
			if (LineIntersectsRect(p0, p1, rect, &hit)) {

				// Resolving if there exists a collision
				if (ent->face == RIGHT) {
					if (hit.x < target.x + target.w) {
						ent->pos.x = hit.x - (ent->rect.x + ent->rect.w);
					}
				} else {
					if (hit.x > target.x) {
						ent->pos.x = hit.x - ent->rect.x;
					}
				}

				// Since the X collision is resolved for that rect
				// No further resolution is needed
				continue;
			}

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

	f32 hit_range = HIT_RANGE;

	// Increasing the hit range during dashing
	if (ent->dash) {
		hit_range += HIT_RANGE_ON_DASH;
	}

	Rect hitbox = {
		.y = rect.y,
		.w = hit_range,
		.h = rect.h,
	};

	// Setting up the hitbox start position according to the dash and face direction
	if (ent->face == LEFT) {
		if (ent->dash) {
			hitbox.x = rect.x - hit_range / 3.0f;
		} else {
			hitbox.x = rect.x - hit_range;
		}
	} else {
		if (ent->dash) {
			hitbox.x = rect.x + rect.w - hit_range / 1.5f;
		} else {
			hitbox.x = rect.x + rect.w;
		}
	}

	return hitbox;
}

void char_handle_atk(Entity* ent, Entity* other) {
	// If the entity hasnt attacked for too long then reset the consecutive attack counter
	if (fabs(ent->last_atk_time - glfwGetTime()) >= CONSEC_ATK_HOLD) {
		ent->consec_atk = 0;
	}

	// Do attack
	if (
		ent->try_atk &&                       // When entity tried to attack
		ent->atk_cooldown == 0.0f &&          // If the attack cooldown is 0
		ent->swing_cooldown == 0.0f &&        // If the swing cooldown is 0
		ent->consec_atk < MAX_CONSEC_ATK      // If the entity hasnt exhausted cosecutive attacks
	) {
		ent->attack = true;
		ent->consec_atk++;

		// Record the attack time
		ent->last_atk_time = glfwGetTime();
	}
	
	Rect hitbox = char_get_hitbox(ent);
	Rect o_rect = entity_get_rect(other);

	// Give knockback and stun to the other entity when attacked
	if (
		ent->attack  &&                               // When able to attack
		!other->dead &&                               // And when the other guy is alive
//	TODO: Maybe introduce someday?
//	!ent->hit    &&                               // And self shouldnt be hit at the moment
		rect_intersect_inclusive(o_rect, hitbox)      // The hitbox and rect of the other should collide
	) {
		if (ent->face == LEFT) {
			other->acc.x -= KNOCKBACK;
		} else {
			other->acc.x += KNOCKBACK;
		}
		other->hit = true;
		other->stun_timeout = STUN_TIMEOUT;

		// Give damage to the other entity
		other->health -= HIT_DMG;
	}

#ifdef RENDER_HITRANGE
	imr_push_quad(
		ctx->inner,
		(v3) { hitbox.x, hitbox.y, ent->pos.z },
		(v2) { hitbox.w, hitbox.h },
		rotate_y(0),
		(v4) { 1, 1, 0, 0.5 }
	);
#endif
}

void char_handle_hit(Entity* ent) {
	if (!ent->hit) return;

	// If a hit is encountered add a slight stun to movement

	// TODO: This stuns the character when got hit. Maybe introduce this when needed?
	// Reset states while being in stun state
	// ent->move[LEFT] = ent->move[RIGHT] = ent->move[UP] = false;
	// ent->try_atk = false;

	// TODO: Multiply rate with (dt) maybe?
	ent->stun_timeout -= STUN_TIMEOUT_RATE;
	if (ent->stun_timeout <= 0.0f)
		ent->hit = false;
}

void char_handle_dash(Entity* ent, f64 dt) {
	// TODO: What if I got hit during dash?
	// Handle dashing

	ent->dash = ent->try_dash && (ent->dash_cooldown == 0.0f);
	if (ent->dash) {
		switch (ent->face) {
			case LEFT:
				ent->acc.x -= DASH_ACC;
				break;
			case RIGHT:
				ent->acc.x += DASH_ACC;
				break;
		}

		// Record the current state of the player
		v3 start_pos = ent->pos;
		v3 end_pos = ent->pos;
		v2 acc = ent->acc;

		// Do a simple acceleration simulation to find the end position after dashing
		while (v2_mag(acc) > 0.0f) {
			v2 vel = v2_add(
				ent->vel,
				v2_mul_scalar(acc, (f32) dt)
			);

			// Calculating end position
			end_pos.x += vel.x * dt;

			// Apply friction
			acc = v2_mul_scalar(acc, AIR_FRICTION);
			acc.x *= GROUND_FRICTION;
		}

		// Save the dash informations
		ent->dash_start_pos = start_pos;
		ent->dash_end_pos = end_pos;
		ent->frame_during_dash = ent->curr_frame;
		ent->face_during_dash = ent->face;

		// Set the dash cooldown
		ent->dash_cooldown = DASH_COOLDOWN;

		// Setting up the alpha for dashing
		ent->dash_ghost_alpha = DASH_GHOST_ALPHA;
	}

	ent->try_dash = false;
	ent->dash_cooldown -= DASH_COOLDOWN_RATE;
	if (ent->dash_cooldown <= 0.0f) {
		ent->dash_cooldown = 0.0f;
	}
}

void char_render(Entity* ent, IMR* imr, v4 tint) {
	// If the character is in attack animation
	// then skip the jump and walking animations
	if (!ent->is_swing_complete)
		goto skip_movement_state;

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
skip_movement_state:

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
		ent->is_swing_complete = false;
		ent->swing_cooldown = SWING_COOLDOWN;
	}

	// If the swing is complete then we start the swing cooldown for next swing
	// TODO: Multiply rate with (dt) maybe?
	if (ent->is_swing_complete && ent->swing_cooldown > 0.0f) {
		ent->swing_cooldown -= SWING_COOLDOWN_RATE;
		if (ent->swing_cooldown < 0.0f) {
			ent->swing_cooldown = 0.0f;
		}
	}

	// Making entity dead when the health drops below 0
	if (ent->health <= 0.0f) {
		ent->anim_state = DEATH;
		ent->dead = true;
		tint = DEAD_TINT;
	}

	// Switch the animation state
	animator_switch_frame(&ent->animator, ent->anim_state);

	// If we are in attack state ie (is_swing_complete = false) and entity is alive
	if (!ent->is_swing_complete && !ent->dead) {
		AnimationEntry* entry = animator_get_entry(&ent->animator, ent->anim_state);

		// Set (is_swing_complete = true) when the animation for the swing is complete
		if (entry->curr_frame >= entry->frames.count - 1) {
			ent->is_swing_complete = true;
		}
	}

	// Get the texture coords of current frame
	ent->curr_frame = animator_get_frame(&ent->animator);

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

	// If hit apply the hit overlay
	v4 overlay = { 0 };
	if (ent->hit) {
		overlay = HIT_OVERLAY;
	}

	// Rendering dash effect
	f32 step;
	v2 size;
	m4 dash_rot = rotate_y(0);

	// Calculating the size and step of the dash
	if (ent->face_during_dash == RIGHT) {
		size = (v2) {
			ent->dash_end_pos.x - ent->dash_start_pos.x,
			ent->size.y
		};

		step = ent->rect.w + 5;
		dash_rot = rotate_y(0);
	} else {
		size = (v2) {
			ent->dash_start_pos.x - ent->dash_end_pos.x,
			ent->size.y
		};

		step = - (ent->rect.w + 5);
		dash_rot = rotate_y(PI);
	}
	
	// Rendering the ghost sprite
	for (
		f32 x = ent->dash_start_pos.x;
		(step > 0) ? (x < ent->dash_end_pos.x) : (x > ent->dash_end_pos.x);
		x += step
	) {
		v3 pos = {
			x,
			ent->dash_start_pos.y,
			0
		};

		imr_push_quad_tex(
			imr,
			pos,
			ent->size,
			ent->frame_during_dash,
			ent->texture.id,
			dash_rot,
			(v4) { tint.r, tint.g, tint.b, ent->dash_ghost_alpha }
		);

		// Decreasing the alpha for every render
		// This is done inside of the forloop to have variadic alpha for each dash ghost
		ent->dash_ghost_alpha -= DASH_GHOST_ALPHA_RATE;
	}

	// Rendering character sprite
	imr_push_quad_tex_overlay(
		imr,
		ent->pos,
		ent->size,
		ent->curr_frame,
		ent->texture.id,
		rot,
		tint,
		overlay
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

	ent->pos = (v3) { 100, 600 - CHAR_RECT.h, 0 };
	ent->size = CHAR_SIZE;
	ent->rect = CHAR_RECT;
	ent->texture = sm->sprites[E_SAMURAI];
	ent->animator = sm->animators[E_SAMURAI];
	ent->face = RIGHT;
	ent->health = 100.0f;
	ent->dash_ghost_alpha = DASH_GHOST_ALPHA;

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
			case GLFW_KEY_SPACE:
				ent->try_dash = true;
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
	if (ent->dead) goto skip_movement;

	char_handle_atk(ent, enemy);
	char_handle_dash(ent, dt);

	// Movement handling
	physics_movement(ent, dt);

skip_movement:
	char_handle_hit(ent);

	// Physics updating
	physics_compute(ent, dt);
	physics_resolve(ent, rects, rects_cnt, dt);
}

// :enemy impl
Entity* enemy_new(SpriteManager* sm) {
	Entity* ent = mem_alloc(sizeof(Entity));

	ent->pos = (v3) { 800, 600 - CHAR_RECT.h, 0 };
	ent->size = CHAR_SIZE;
	ent->rect = CHAR_RECT;
	ent->texture = sm->sprites[E_SAMURAI];
	ent->animator = sm->animators[E_SAMURAI];
	ent->face = LEFT;
	ent->health = 100.0f;
	ent->dash_ghost_alpha = DASH_GHOST_ALPHA;

	return ent;
}

void enemy_update(Entity* ent, Entity* player, Rect* rects, i32 rects_cnt, f64 dt) {
	// If dead dont update
	if (ent->dead) goto skip_movement;

	// If the enemy is doing consecutive attack
	// Do not chase the player
	if (ent->do_consec_atk) {

		// Stopping every movement when doing consecutive attack
		ent->try_atk = true;
		ent->move[UP] = ent->move[LEFT] = ent->move[RIGHT] = false;
		if (ent->consec_atk >= MAX_CONSEC_ATK) {
			ent->do_consec_atk = false;

			// Enemy just completed its consecutive attack
			// So giving it some attack cooldown
			ent->atk_cooldown = ENEMY_ATK_COOLDOWN;
		}

		// Skip the running and chasing part
		goto skip_chasing;
	} else {

		// Do not attack if the consec attack is not enabled
		ent->try_atk = false;
	}

	// Only facing when we arent doing consecutive attacks
	// Make enemy face the player
	if (player->pos.x < ent->pos.x) {
		ent->face = LEFT;
	}
	else if (player->pos.x > ent->pos.x) {
		ent->face = RIGHT;
	}

	// During cooldown enemy cannot attack
	// So logic that handles enemy doing whatever the fk it does when it cannot attack
	// GOES HERE
	if (ent->atk_cooldown > 0.0f) {
		f32 player_enemy_dist = fabsf(player->pos.x - ent->pos.x);

		// If player is way closer to the enemy then dash away
		if (player_enemy_dist < IN_PLAYER_HITZONE) {
			i32 chance = rand_range(0, 100);

			// Only dash for certain probability
			if (chance < ENEMY_DASH_PROBABILITY) {
				// This helps the enemy to dash where there is more space
				if ((ent->pos.x - 0) > (WIN_WIDTH - ent->pos.x)) {
					// Dash to left
					ent->face = LEFT;
					ent->try_dash = true;
				} else {
					// Dash to right
					ent->face = RIGHT;
					ent->try_dash = true;
				}
			}
		}

		// If player is too close then just run the opposite direction
		if (player_enemy_dist < PLAYER_TOO_CLOSE) {
			if (player->pos.x < ent->pos.x) {
				ent->move[LEFT] = false;
				ent->move[RIGHT] = true;
			} else {
				ent->move[LEFT] = true;
				ent->move[RIGHT] = false;
			}
		} else {
			// If the player isnt in the range just stop mate
			ent->move[LEFT] = ent->move[RIGHT] = false;
		}

		// Droping the cooldown so that enemy can attack again
		ent->atk_cooldown -= ENEMY_ATK_COOLDOWN_RATE;
		if (ent->atk_cooldown <= 0.0f)
			ent->atk_cooldown = 0.0f;

		// Do not chase when you cannot hit
		goto skip_chasing;
	}

	// Enemy Chasing

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

skip_chasing:

	// If player is alive and inside of hitbox ATTACK
	Rect hitbox = char_get_hitbox(ent);
	Rect p_rect = entity_get_rect(player);
	if (rect_intersect_inclusive(hitbox, p_rect) && !player->dead) {
		if (!ent->do_consec_atk && ent->consec_atk == 0 && ent->atk_cooldown == 0.0f) {
			ent->do_consec_atk = true;
		}
	}

	char_handle_atk(ent, player);
	char_handle_dash(ent, dt);

	// Movement handling
	physics_movement(ent, dt);

skip_movement:
	char_handle_hit(ent);

	physics_compute(ent, dt);
	physics_resolve(ent, rects, rects_cnt, dt);
}

// :ui impl
void render_progress_bar(IMR* imr, v3 pos, v2 size, f32 val, f32 max, v4 color) {
	f32 length = val / max * size.x;
	imr_push_quad(
		imr,
		pos,
		(v2) { length, size.y },
		rotate_x(0),
		color
	);
}

// :main
int main() {
	rand_init(time(NULL));

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

	b32 pause = false;

	// Set renderer to context for debug rendering
	ctx->inner = &imr;

	// Characters
	Entity* player = player_new(&sm);
	Entity* enemy = enemy_new(&sm);

	Rect rects[] = {
		{ 0, 700, WIN_WIDTH, 100 },
		{ 0, 0, 50, WIN_HEIGHT },
		{ WIN_WIDTH - 50, 0, 50, WIN_HEIGHT },
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
			else if (event.type == KEYDOWN) {
				if (event.e.key == GLFW_KEY_ESCAPE) {
					pause = !pause;
				}
			}
		}

		m4 mvp = ocamera_calc_mvp(&camera);
		imr_update_mvp(&imr, mvp);
		imr_clear((v4) { .5f, .5f, .5f, 1.0f });

		imr_begin(&imr);

		// :update
		if (!pause) {
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
				);
			}

			// Rendering ui stuff
			render_progress_bar(&imr, (v3) { 10, 10, 0 }, (v2) { 200, 20 }, player->health, 100.0f, PLAYER_TINT);
			render_progress_bar(&imr, (v3) { 10, 40, 0 }, (v2) { 200, 10 }, DASH_COOLDOWN - player->dash_cooldown, DASH_COOLDOWN, PLAYER_TINT);
			render_progress_bar(&imr, (v3) { 10, 60, 0 }, (v2) { 200, 10 }, MAX_CONSEC_ATK - player->consec_atk, MAX_CONSEC_ATK, PLAYER_TINT);

			render_progress_bar(&imr, (v3) { WIN_WIDTH - 210, 10, 0 }, (v2) { 200, 20 }, enemy->health, 100.0f, ENEMY_TINT);
		}

		// :pause
		if (pause) {
			imr_push_quad(
				&imr,
				(v3) {0},
				(v2) { WIN_WIDTH, WIN_HEIGHT },
				rotate_x(0),
				(v4) { 0, 0, 0, 0.7 }
			);

			imr_push_quad(
				&imr,
				(v3) { WIN_WIDTH / 2 - PAUSE_BUTTON_WIDTH / 2 - 50, WIN_HEIGHT / 2 - PAUSE_BUTTON_HEIGHT / 2, 0 },
				(v2) { PAUSE_BUTTON_WIDTH, PAUSE_BUTTON_HEIGHT },
				rotate_x(0),
				(v4) { 1, 1, 1, 1 }
			);

			imr_push_quad(
				&imr,
				(v3) { WIN_WIDTH / 2 - PAUSE_BUTTON_WIDTH / 2 + 50, WIN_HEIGHT / 2 - PAUSE_BUTTON_HEIGHT / 2, 0 },
				(v2) { PAUSE_BUTTON_WIDTH, PAUSE_BUTTON_HEIGHT },
				rotate_x(0),
				(v4) { 1, 1, 1, 1 }
			);
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
	return 0;
}
