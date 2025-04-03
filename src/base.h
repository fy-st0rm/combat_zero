#ifndef __BASE_H__
#define __BASE_H__

// :includes
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "external/glew/include/GL/glew.h"
#include "external/glfw/include/GLFW/glfw3.h"
#include "external/stb/stb_image.h"

// :log
#if defined(__clang__) || defined(__gcc__)
	#define STATIC_ASSERT _Static_assert
#else
	#define STATIC_ASSERT static_assert
#endif

#define panic(x, ...) \
	do {\
		if (!(x)) {\
			fprintf(stderr, "\033[31m[ASSERTION]: %s:%d:\033[0m ", __FILE__, __LINE__);\
			fprintf(stderr, " " __VA_ARGS__);\
			fprintf(stderr, "\n");\
			exit(1);\
		}\
	} while(0)

#define LOG_NORMAL 0
#define LOG_ERROR  91
#define LOG_SUCESS 92
#define LOG_WARN   93
#define LOG_INFO   95

#define log_typed(type, ...)         \
	({                                 \
		printf("\033[%dm", type);        \
		printf(__VA_ARGS__);             \
		printf("\033[%dm", LOG_NORMAL);  \
	})

#define log(...)                          \
	({                                      \
		log_typed(LOG_NORMAL, "[LOG]:    ");  \
		log_typed(LOG_NORMAL, __VA_ARGS__);   \
	})

#define log_sucess(...)                   \
	({                                      \
		log_typed(LOG_SUCESS, "[SUCESS]: ");  \
		log_typed(LOG_SUCESS, __VA_ARGS__);   \
	})

#define log_warn(...)                     \
	({                                      \
		log_typed(LOG_WARN,   "[WARN]:   ");  \
		log_typed(LOG_WARN, __VA_ARGS__);     \
	})

#define log_error(...)                    \
	({                                      \
		log_typed(LOG_ERROR,  "[ERROR]:  ");  \
		log_typed(LOG_ERROR, __VA_ARGS__);    \
	})

#define log_info(...)                     \
	({                                      \
		log_typed(LOG_INFO,   "[INFO]:   ");  \
		log_typed(LOG_INFO, __VA_ARGS__);     \
	})

#define GLCall(x)\
	(\
		clear_gl_error(), \
		x\
	);\
	panic(gl_error_log(#x, __FILE__, __LINE__), "Opengl failed.\n");\

static void clear_gl_error() {
	while(glGetError());
}

static int gl_error_log(const char* function, const char* file, int line) {
	GLenum error;
	while ((error = glGetError())) {
		log_error("[Error code]: %d\n", error);
		log_error("[Error message]: %s\n", gluErrorString(error));
		log_error("[Opengl error]: %s %s: %d\n", function ,file, line);
		return 0;
	}
	return 1;
}

// :types
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;
typedef float f32;
typedef double f64;
typedef enum {
	false, true
} b32;

STATIC_ASSERT(sizeof(u8) == 1, "Expected u8 to be 1 byte.");
STATIC_ASSERT(sizeof(u16) == 2, "Expected u16 to be 2 bytes.");
STATIC_ASSERT(sizeof(u32) == 4, "Expected u32 to be 4 bytes.");
STATIC_ASSERT(sizeof(u64) == 8, "Expected u64 to be 8 bytes.");
STATIC_ASSERT(sizeof(i8) == 1, "Expected i8 to be 1 byte.");
STATIC_ASSERT(sizeof(i16) == 2, "Expected i16 to be 2 bytes.");
STATIC_ASSERT(sizeof(i32) == 4, "Expected i32 to be 4 bytes.");
STATIC_ASSERT(sizeof(i64) == 8, "Expected i64 to be 8 bytes.");
STATIC_ASSERT(sizeof(f32) == 4, "Expected f32 to be 4 bytes.");
STATIC_ASSERT(sizeof(f64) == 8, "Expected f64 to be 8 bytes.");
STATIC_ASSERT(sizeof(b32) == 4, "Expected b32 to be 4 bytes.");

// :alloc def
#define TRACE_ALLOCATOR_MEMORY_CAP 1024
#define TRACE_ALLOCATOR_GROW_RATE 0.5

typedef struct {
	void* ptr;
	size_t size;
	const char* file;
	i32 line;
} Traceable_Memory_Block;
#define print_traceable_memory_block(block) ({\
	printf(\
		"%p at %s:%d of %zu bytes\n",\
		(block)->ptr,  (block)->file,\
		(block)->line, (block)->size\
	);\
});

typedef struct {
	Traceable_Memory_Block* blocks;
	u32 blocks_cnt;
	u32 blocks_cap;
} Trace_Allocator;

Trace_Allocator* trace_allocator_new();
void trace_allocator_delete(Trace_Allocator* allocator);
#define trace_allocator_alloc(allocator, size) __trace_allocator_alloc(allocator, size, __FILE__, __LINE__)
void* __trace_allocator_alloc(Trace_Allocator* allocator, size_t size, const char* file, i32 line);
#define trace_allocator_realloc(allocator, ptr, new_cap) __trace_allocator_realloc(allocator, ptr, new_cap, __FILE__, __LINE__)
void* __trace_allocator_realloc(Trace_Allocator* allocator, void* ptr, size_t new_cap, const char* file, i32 line);
void trace_allocator_free(Trace_Allocator* allocator, void* ptr);
void trace_allocator_alert(Trace_Allocator* allocator);

#define mem_alloc(size) __mem_alloc(size, __FILE__, __LINE__)
void* __mem_alloc(i64 size, const char* file, i32 line);
#define mem_realloc(ptr, new_cap) __mem_realloc(ptr, new_cap, __FILE__, __LINE__);
void* __mem_realloc(void* ptr, size_t new_cap, const char* file, i32 line);
void mem_free(void* ptr);

// :vec def
typedef struct { f32 x, y;    } v2;
typedef struct { f32 x, y, z; } v3;
typedef struct {
	union { f32 x, r; };
	union { f32 y, g; };
	union { f32 z, b; };
	union { f32 w, a; };
} v4;

void print_v2(v2 v);
void print_v3(v3 v);
void print_v4(v4 v);

b32 v2_eq(v2 a, v2 b);
b32 v3_eq(v3 a, v3 b);
b32 v4_eq(v4 a, v4 b);

v2 v2_add(v2 a, v2 b);
v3 v3_add(v3 a, v3 b);
v4 v4_add(v4 a, v4 b);

v2 v2_sub(v2 a, v2 b);
v3 v3_sub(v3 a, v3 b);
v4 v4_sub(v4 a, v4 b);

v2 v2_mul(v2 a, v2 b);
v3 v3_mul(v3 a, v3 b);
v4 v4_mul(v4 a, v4 b);

v2 v2_mul_scalar(v2 v, f32 scalar);
v3 v3_mul_scalar(v3 v, f32 scalar);
v4 v4_mul_scalar(v4 v, f32 scalar);

v2 v2_div(v2 a, v2 b);
v3 v3_div(v3 a, v3 b);
v4 v4_div(v4 a, v4 b);

f32 v2_mag(v2 v);
f32 v3_mag(v3 v);
f32 v4_mag(v4 v);

v2 v2_dir(v2 v);
v3 v3_dir(v3 v);

v2 v2_normalize(v2 v);
v3 v3_normalize(v3 v);
v4 v4_normalize(v4 v);

v2 v2_cross(v2 a, v2 b);
v3 v3_cross(v3 a, v3 b);
v4 v4_cross(v4 a, v4 b);

// :mat def
typedef struct {
	f32 m[4][4];
} m4;

void print_m4(m4 m);
void m4_clear(m4* m);
m4 m4_mul(m4 m1, m4 m2);
v3 m4_mul_v3(m4 m, v3 v);
m4 m4_identity();
m4 m4_zero();
m4 m4_inverse(m4 in);
m4 m4_translate(m4 m, v3 v);
m4 m4_transpose(m4 m);
m4 m4_scale(f32 s);
m4 ortho_projection(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far);
m4 persp_projection(f32 aspect_ratio, f32 fov, f32 near, f32 far);
m4 rotate_x(f32 theta);
m4 rotate_y(f32 theta);
m4 rotate_z(f32 theta);

// :rect def
typedef struct {
	f32 x, y, w, h;
} Rect;

#define print_rect(r) print_v4(*((v4*)&r))
b32 rect_intersect(Rect r1, Rect r2);
b32 point_in_rect(v2 p, Rect r);
Rect rect_with_offset(v2 pos, Rect offset_rect);

// :math def
#define PI 3.14159
#define to_radians(x) ((x) * PI / 180)
#define to_degrees(x) ((x) * 180 / PI)
#define rand_init(seed) srand(seed)
#define rand_range(l, u) rand() % (u - l + 1) + l

static b32 f32_eq(f32 a, f32 b) {
	return fabs(a - b) < 0.01f;
}

static v2 pixel_to_gl_coords(v2 pos, u32 WIN_WIDTH, u32 WIN_HEIGHT) {
	return (v2) {
		(2 * pos.x) / WIN_WIDTH - 1,
		1 - (2 * pos.y) / WIN_HEIGHT
	};
}

static v2 dp_to_dgl_coords(v2 dp, u32 WIN_WIDTH, u32 WIN_HEIGHT) {
	return (v2) {
		(2 * dp.x) / WIN_WIDTH,
		- (2 * dp.y) / WIN_HEIGHT
	};
}

static v2 gl_to_pixel_coords(v2 pos, u32 WIN_WIDTH, u32 WIN_HEIGHT) {
	return (v2) {
		(pos.x + 1) * WIN_WIDTH / 2,
		(1 - pos.y) * WIN_HEIGHT / 2
	};
}

static v2 dgl_to_dp_coords(v2 dp, u32 WIN_WIDTH, u32 WIN_HEIGHT) {
	return (v2) {
		(dp.x) * WIN_WIDTH / 2,
		(-dp.y) * WIN_HEIGHT / 2
	};
}

// :window def
typedef struct {
	GLFWwindow* glfw_window;
	u32 width, height;
	b32 should_close;
} Window;

Window window_new(const char* title, u32 width, u32 height);
void window_delete(Window window);
void window_update(Window* window);

// :frame controller def
// NOTE: The times are in seconds
typedef struct {
	f64 start_time;
	f64 start_tick;
	f64 unit_frame;
	f64 dt;
	i32 frame;
	i32 fps;
} FrameController;

FrameController frame_controller_new(i32 fps);
void frame_controller_start(FrameController* fc);
void frame_controller_end(FrameController* fc);

// :event def
typedef enum {
	KEYDOWN,
	KEYUP,

	MOUSE_BUTTON_DOWN,
	MOUSE_BUTTON_UP,
	MOUSE_MOTION,
} EventType;

typedef enum {
	MOUSE_BUTTON_LEFT,
	MOUSE_BUTTON_RIGHT
} MouseButton;

typedef struct {
	EventType type;

	union {
		u32 key;
		MouseButton button;
		v2 mouse_pos;
	} e;
} Event;

b32 event_poll(Window window, Event* event);
v2 event_mouse_pos(Window window);
void event_set_mouse_pos(Window window, v2 pos);

// :event queue def
#define MAX_EVENTS 1024
typedef struct {
	Event events[MAX_EVENTS];
	i32 front, back;
} EventQueue;

void event_enqueue(EventQueue* queue, Event e);
Event event_dequeue(EventQueue* queue);

// :ocamera def
typedef struct {
	f32 left, right, top, bottom, near, far;
} OCamera_Boundary;

typedef struct {
	v2 pos;
	f32 zoom;
	m4 mvp;
	OCamera_Boundary boundary;

	// For camera follow
	b32 active_x;
	b32 active_y;
} OCamera;

OCamera ocamera_new(v2 pos, f32 zoom, OCamera_Boundary boundary);
void ocamera_change_zoom(OCamera* cam, f32 dz);
void ocamera_change_pos(OCamera* cam, v2 dp);
void ocamera_follow(OCamera* cam, Rect to_follow_rect, v2 offset, f32 delay, v2 surf_size);
m4 ocamera_calc_mvp(OCamera* cam);

// :pcamera def
typedef struct {
	f32 aspect_ratio, fov, near, far;
} PCamera_Info;

typedef struct {
	v3 dir, up, right;
	v3 pos;
	m4 look_at, mvp;
	f32 pitch, yaw;

	// Mouse
	v2 mp;
	f32 sensitivity;
	b32 first;
	b32 mouse_enable;

	PCamera_Info info;
} PCamera;

PCamera pcamera_new(v3 pos, v3 dir, f32 sensitivity, PCamera_Info info);
void pcamera_change_pos(PCamera* cam, v3 dp);
void pcamera_handle_mouse(PCamera* cam, Window window);
m4 pcamera_calc_mvp(PCamera* cam);

// :texture def
typedef struct {
	u32 id, width, height;
} Texture;

// TODO: Implement control over texture filters
// Filters are hard coded for now
Texture texture_from_file(const char* filepath, b32 flip);
Texture texture_from_data(u32 width, u32 height, u32* data);
void texture_clear(Texture texture);
void texture_bind(Texture texture);
void texture_unbind(Texture texture);
void texture_delete(Texture texture);

// :shader def
typedef u32 Shader;
typedef enum {
	VERTEX_SHADER = GL_VERTEX_SHADER,
	FRAGMENT_SHADER = GL_FRAGMENT_SHADER
} ShaderType;

Shader shader_new(const char* v_src, const char* f_src);
void shader_delete(Shader id);
u32 shader_compile(ShaderType type, const char* shader_src);

// :fbo def
typedef struct {
	u32 id;
	Texture color_texture;
} FBO;

FBO fbo_new(u32 width, u32 height);
void fbo_delete(FBO* fbo);
void fbo_bind(FBO* fbo);
void fbo_unbind();

// :imr def
typedef struct {
	v3 pos;
	v4 color;
	v2 tex_coord;
	f32 tex_id;
} Vertex;

typedef struct {
	v3 a, b, c;
} Triangle;

#define TEXTURE_SAMPLE_AMT 32
#define VERTEX_SIZE   10
#define MAX_VERT_CNT  10000
#define MAX_BUFF_CAP  MAX_VERT_CNT  * VERTEX_SIZE
#define MAX_VBO_SIZE  MAX_BUFF_CAP  * sizeof(f32)

STATIC_ASSERT(VERTEX_SIZE == sizeof(Vertex) / sizeof(f32), "Size of vertex missmatched");

typedef struct {
	u32 vao, vbo;
	Shader shader;
	Shader def_shader;
	f32 buffer[MAX_BUFF_CAP];
	u32 buff_idx;
	Texture white;
} IMR;

IMR imr_new();
void imr_delete(IMR* imr);
void imr_clear(v4 color);
void imr_begin(IMR* imr);
void imr_end(IMR* imr);
void imr_switch_shader(IMR* imr, Shader shader);
void imr_reapply_samplers(IMR* imr);
void imr_switch_shader_to_default(IMR* imr);
void imr_update_mvp(IMR* imr, m4 mvp);
void imr_push_vertex(IMR* imr, Vertex v);
void imr_push_quad(IMR* imr, v3 pos, v2 size, m4 rot, v4 color);
void imr_push_quad_tex(IMR* imr, v3 pos, v2 size, Rect tex_rect, f32 tex_id, m4 rot, v4 color);
void imr_push_triangle(IMR* imr, v3 p1, v3 p2, v3 p3, m4 rot, v4 color);
void imr_push_triangle_tex(IMR* imr, v3 p1, v3 p2, v3 p3, Triangle tex_coord, f32 tex_id, m4 rot, v4 color);

// :context def
typedef struct {
	Trace_Allocator* t_alloc;
	EventQueue e_queue;
	void* inner;
} Context;

void ctx_end();

// :impl
#ifdef BASE_IMPLEMENTATION

// :context impl
Context ctx = {
	.t_alloc = NULL,
	.e_queue = (EventQueue) {
		.front = -1,
		.back  = -1,
	},
	.inner = NULL,
};

void ctx_end() {
	if (ctx.t_alloc) trace_allocator_delete(ctx.t_alloc);
}

// :alloc impl
Trace_Allocator* trace_allocator_new() {
	Trace_Allocator* allocator = (Trace_Allocator*) calloc(1, sizeof(Trace_Allocator));

	allocator->blocks = (Traceable_Memory_Block*) calloc(
		TRACE_ALLOCATOR_MEMORY_CAP,
		sizeof(Traceable_Memory_Block)
	);
	allocator->blocks_cnt = 0;
	allocator->blocks_cap = TRACE_ALLOCATOR_MEMORY_CAP;

	return allocator;
}

void trace_allocator_delete(Trace_Allocator* allocator) {
	trace_allocator_alert(allocator);
	free(allocator->blocks);
	free(allocator);
}

void handle_blocks_overflow(Trace_Allocator* allocator) {
	if (allocator->blocks_cnt < allocator->blocks_cap) return;

	Traceable_Memory_Block* tmp_blocks = (Traceable_Memory_Block*) calloc(
		TRACE_ALLOCATOR_MEMORY_CAP,
		sizeof(Traceable_Memory_Block)
	);
	memcpy(
		tmp_blocks, allocator->blocks,
		sizeof(Traceable_Memory_Block) * allocator->blocks_cnt
	);

	free(allocator->blocks);

	allocator->blocks_cap += TRACE_ALLOCATOR_GROW_RATE * TRACE_ALLOCATOR_MEMORY_CAP;
	allocator->blocks = (Traceable_Memory_Block*) calloc(
		allocator->blocks_cap,
		sizeof(Traceable_Memory_Block)
	);
	memcpy(
		allocator->blocks, tmp_blocks,
		sizeof(Traceable_Memory_Block) * allocator->blocks_cnt
	);

	free(tmp_blocks);
}

b32 remove_block(Trace_Allocator* allocator, void* ptr) {
	if (!ptr) return false;

	for (u32 i = 0; allocator->blocks_cnt; i++) {
		if (allocator->blocks[i].ptr == ptr) {
			memmove(
				&allocator->blocks[i], &allocator->blocks[i+1],
				(allocator->blocks_cnt - i - 1) * sizeof(Traceable_Memory_Block)
			);
			allocator->blocks_cnt--;
			return true;
		}
	}
	return false;
}

void* __trace_allocator_alloc(Trace_Allocator* allocator, size_t size, const char* file, i32 line) {
	handle_blocks_overflow(allocator);

	void* ptr = (void*) malloc(size);
	Traceable_Memory_Block mem = {
		ptr,
		size,
		file,
		line
	};
	memset(ptr, 0, size);

	allocator->blocks[allocator->blocks_cnt++] = mem;
	return ptr;
}

void* __trace_allocator_realloc(Trace_Allocator* allocator, void* ptr, size_t new_cap, const char* file, i32 line) {
	// Do a normal alloc if the mem block doesnt exists
	if (!remove_block(allocator, ptr))
		return __trace_allocator_alloc(allocator, new_cap, file, line);

	ptr = (void*) realloc(ptr, new_cap);
	Traceable_Memory_Block mem = {
		ptr,
		new_cap,
		file,
		line
	};

	allocator->blocks[allocator->blocks_cnt++] = mem;
	return ptr;
}

void trace_allocator_free(Trace_Allocator* allocator, void* ptr) {
	if (remove_block(allocator, ptr))
		free(ptr);
}

void trace_allocator_alert(Trace_Allocator* allocator) {
	if (!allocator->blocks_cnt) return;

	printf("\n---------Unfreed memories---------\n");
	size_t total = 0;
	for (u32 i = 0; i < allocator->blocks_cnt; i++) {
		print_traceable_memory_block(&allocator->blocks[i]);
		total += allocator->blocks[i].size;
	}
	printf("\nTotal unfreed memories count = %d\n", allocator->blocks_cnt);
	printf("Total unfreed memories = %d bytes\n", total);
	printf("---------Unfreed memories---------\n\n");
}

void* __mem_alloc(i64 size, const char* file, i32 line) {
	if (!ctx.t_alloc) {
		ctx.t_alloc = trace_allocator_new();
	}
	return __trace_allocator_alloc(ctx.t_alloc, size, file, line);
}

void* __mem_realloc(void* ptr, size_t new_cap, const char* file, i32 line){
	if (!ctx.t_alloc) {
		ctx.t_alloc = trace_allocator_new();
	}
	return __trace_allocator_realloc(ctx.t_alloc, ptr, new_cap, file, line);
}

void mem_free(void* ptr) {
	trace_allocator_free(ctx.t_alloc, ptr);
}

// :vec impl
void print_v2(v2 v) {
	printf("(%f, %f)\n", v.x, v.y);
}

void print_v3(v3 v) {
	printf("(%f, %f, %f)\n", v.x, v.y, v.z);
}

void print_v4(v4 v) {
	printf("(%f, %f, %f, %f)\n", v.x, v.y, v.z, v.w);
}

b32 v2_eq(v2 a, v2 b) {
	return f32_eq(a.x, b.x) &&
         f32_eq(a.y, b.y);
}

b32 v3_eq(v3 a, v3 b) {
	return f32_eq(a.x, b.x) &&
         f32_eq(a.y, b.y) &&
         f32_eq(a.z, b.z);
}

b32 v4_eq(v4 a, v4 b) {
	return f32_eq(a.x, b.x) &&
         f32_eq(a.y, b.y) &&
         f32_eq(a.z, b.z) &&
         f32_eq(a.w, b.w);
}

v2 v2_add(v2 a, v2 b) {
	return (v2) {
		.x = a.x + b.x,
		.y = a.y + b.y
	};
}

v3 v3_add(v3 a, v3 b) {
	return (v3) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z
	};
}

v4 v4_add(v4 a, v4 b) {
	return (v4) {
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
		.w = a.w + b.w
	};
}

v2 v2_sub(v2 a, v2 b) {
	return (v2) {
		.x = a.x - b.x,
		.y = a.y - b.y
	};
}

v3 v3_sub(v3 a, v3 b) {
	return (v3) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z
	};
}

v4 v4_sub(v4 a, v4 b) {
	return (v4) {
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
		.w = a.w - b.w
	};
}

v2 v2_mul(v2 a, v2 b) {
	return (v2) {
		.x = a.x * b.x,
		.y = a.y * b.y
	};
}

v3 v3_mul(v3 a, v3 b) {
	return (v3) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z
	};
}

v4 v4_mul(v4 a, v4 b) {
	return (v4) {
		.x = a.x * b.x,
		.y = a.y * b.y,
		.z = a.z * b.z,
		.w = a.w * b.w
	};
}

v2 v2_mul_scalar(v2 v, f32 scalar) {
	return (v2) {
		.x = v.x * scalar,
		.y = v.y * scalar
	};
}

v3 v3_mul_scalar(v3 v, f32 scalar) {
	return (v3) {
		.x = v.x * scalar,
		.y = v.y * scalar,
		.z = v.z * scalar
	};
}

v4 v4_mul_scalar(v4 v, f32 scalar) {
	return (v4) {
		.x = v.x * scalar,
		.y = v.y * scalar,
		.z = v.z * scalar,
		.w = v.w * scalar
	};
}

v2 v2_div(v2 a, v2 b) {
	return (v2) {
		.x = a.x / b.x,
		.y = a.y / b.y
	};
}

v3 v3_div(v3 a, v3 b) {
	return (v3) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z
	};
}

v4 v4_div(v4 a, v4 b) {
	return (v4) {
		.x = a.x / b.x,
		.y = a.y / b.y,
		.z = a.z / b.z,
		.w = a.w / b.w
	};
}

f32 v2_mag(v2 v) {
	return sqrt(v.x * v.x + v.y * v.y);
}

f32 v3_mag(v3 v) {
	return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

f32 v4_mag(v4 v) {
	return sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

v2 v2_dir(v2 v) {
	v2 d = { 0, 0 };
	if (v.x < 0) d.x = -1;
	if (v.x > 0) d.x = 1;
	if (v.x == 0) d.x = 0;

	if (v.y < 0) d.y = -1;
	if (v.y > 0) d.y = 1;
	if (v.y == 0) d.y = 0;
	return d;
}

v3 v3_dir(v3 v) {
	v3 d = { 0, 0 };
	if (v.x < 0) d.x = -1;
	if (v.x > 0) d.x = 1;
	if (v.x == 0) d.x = 0;

	if (v.y < 0) d.y = -1;
	if (v.y > 0) d.y = 1;
	if (v.y == 0) d.y = 0;

	if (v.z < 0) d.z = -1;
	if (v.z > 0) d.z = 1;
	if (v.z == 0) d.z = 0;
	return d;
}

v2 v2_normalize(v2 v) {
	f32 r = v2_mag(v);
	return (v2) {
		.x = v.x / r,
		.y = v.y / r
	};
}

v3 v3_normalize(v3 v) {
	f32 r = v3_mag(v);
	return (v3) {
		.x = v.x / r,
		.y = v.y / r,
		.z = v.z / r
	};
}
v4 v4_normalize(v4 v) {
	f32 r = v4_mag(v);
	return (v4) {
		.x = v.x / r,
		.y = v.y / r,
		.z = v.z / r,
		.w = v.w / r
	};
}

v2 v2_cross(v2 a, v2 b) {
	panic(false, "v2_cross is not implemented yet.\n");
}

v3 v3_cross(v3 a, v3 b) {
	return (v3) {
		.x = a.y * b.z - a.z * b.y,
		.y = - (a.x * b.z - a.z * b.x),
		.z = a.x * b.y - a.y * b.x
	};
}

v4 v4_cross(v4 a, v4 b) {
	panic(false, "v3_cross is not implemented yet.\n");
}

// :mat impl
void print_m4(m4 m) {
	for (i32 i = 0; i < 4; i++) {
		for (i32 j = 0; j < 4; j++) {
			printf("%f\t", m.m[i][j]);
		}
		printf("\n");
	}
}

void m4_clear(m4* m) {
	memset(m, 0, sizeof(m4));
}

m4 m4_mul(m4 m1, m4 m2) {
	m4 out;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			out.m[i][j] = 0;
			for (int k = 0; k < 4; k++) {
				out.m[i][j] += m1.m[i][k] * m2.m[k][j];
			}
		}
	}
	return out;
}

v3 m4_mul_v3(m4 m, v3 v) {
	v3 out;
	out.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
	out.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
	out.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
	f32 w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];
	if (w) {
		out.x /= w;
		out.y /= w;
		out.z /= w;
	}
	return out;
}

m4 m4_identity() {
	return (m4) {
		.m = {
			{ 1.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 1.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 1.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 1.0f }
		}
	};
}

m4 m4_zero() {
	return (m4) {
		.m = {
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f }
		}
	};
}

m4 m4_inverse(m4 in) {
	m4 out;
	m4_clear(&out);
	out.m[0][0] = in.m[0][0]; 
	out.m[0][1] = in.m[1][0]; 
	out.m[0][2] = in.m[2][0]; 
	out.m[0][3] = 0.0f;
	out.m[1][0] = in.m[0][1]; 
	out.m[1][1] = in.m[1][1];
	out.m[1][2] = in.m[2][1];
	out.m[1][3] = 0.0f;
	out.m[2][0] = in.m[0][2];
	out.m[2][1] = in.m[1][2];
	out.m[2][2] = in.m[2][2];
	out.m[2][3] = 0.0f;
	out.m[3][0] = -(in.m[3][0] * out.m[0][0] + in.m[3][1] * out.m[1][0] + in.m[3][2] * out.m[2][0]);
	out.m[3][1] = -(in.m[3][0] * out.m[0][1] + in.m[3][1] * out.m[1][1] + in.m[3][2] * out.m[2][1]);
	out.m[3][2] = -(in.m[3][0] * out.m[0][2] + in.m[3][1] * out.m[1][2] + in.m[3][2] * out.m[2][2]);
	out.m[3][3] = 1.0f;
	return out;
}

m4 m4_translate(m4 m, v3 v) {
	return (m4) {
		.m = {
			{      1.0f, m.m[0][1], m.m[0][2],  v.x },
			{ m.m[1][0],      1.0f, m.m[1][2],  v.y },
			{ m.m[2][0], m.m[2][1],      1.0f,  v.z },
			{ m.m[3][0], m.m[3][1], m.m[3][2], 1.0f },
		}
	};
}

m4 m4_transpose(m4 m) {
	return (m4) {
		.m = {
			{ m.m[0][0], m.m[1][0], m.m[2][0], m.m[3][0] },
			{ m.m[0][1], m.m[1][1], m.m[2][1], m.m[3][1] },
			{ m.m[0][2], m.m[1][2], m.m[2][2], m.m[3][2] },
			{ m.m[0][3], m.m[1][3], m.m[2][3], m.m[3][3] }
		}
	};
}

m4 m4_scale(f32 s) {
	return (m4) {
		.m = {
			{    s, 0.0f, 0.0f, 0.0f },
			{ 0.0f,    s, 0.0f, 0.0f },
			{ 0.0f, 0.0f,    s, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 1.0f },
		}
	};
}

m4 ortho_projection(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far) {
	f32 x_range = right - left;
	f32 y_range = top - bottom;
	f32 z_range = far - near;

	return (m4) {
		.m = {
			{               2 / x_range,                         0,                       0, 0 },
			{                         0,               2 / y_range,                       0, 0 },
			{                         0,                         0,            -2 / z_range, 0 },
			{ -(right + left) / x_range, -(top + bottom) / y_range, -(far + near) / z_range, 1 }
		}
	};
}

m4 persp_projection(f32 aspect_ratio, f32 fov, f32 near, f32 far) {
	f32 t = tanf(to_radians(fov / 2));
	f32 z_range = near - far;
	f32 A = (-far - near) / z_range;
	f32 B = (2 * far * near) / z_range;

	return (m4) {
		.m = {
			{ 1 / (aspect_ratio * t),     0, 0, 0 },
			{                      0, 1 / t, 0, 0 },
			{                      0,     0, A, B },
			{                      0,     0, 1, 0 }
		}
	};
}

m4 rotate_x(f32 theta) {
	return (m4) {
		.m = {
			{ 1.0f,         0.0f,        0.0f, 0.0f },
			{ 0.0f,  cosf(theta), sinf(theta), 0.0f },
			{ 0.0f, -sinf(theta), cosf(theta), 0.0f },
			{ 0.0f,         0.0f,        0.0f, 1.0f }
		}
	};
}


m4 rotate_y(f32 theta) {
	return (m4) {
		.m = {
			{  cosf(theta),  0.0f, sinf(theta), 0.0f },
			{         0.0f,  1.0f,        0.0f, 0.0f },
			{ -sinf(theta),  0.0f, cosf(theta), 0.0f },
			{         0.0f,  0.0f,        0.0f, 1.0f }
		}
	};
}

m4 rotate_z(f32 theta) {
	return (m4) {
		.m = {
			{  cosf(theta), sinf(theta), 0.0f, 0.0f },
			{ -sinf(theta), cosf(theta), 0.0f, 0.0f },
			{         0.0f,        0.0f, 1.0f, 0.0f },
			{         0.0f,        0.0f, 0.0f, 1.0f }
		}
	};
}

// :rect impl
b32 rect_intersect(Rect r1, Rect r2) {
	return (
		((r1.x < r2.x && r2.x < r1.x + r1.w) ||
		 (r2.x < r1.x && r1.x < r2.x + r2.w) ||
		 (r1.x < r2.x && r2.x + r2.w < r1.x + r1.w) ||
		 (r2.x < r1.x && r1.x + r1.w < r2.x + r2.w)) &&
		((r1.y < r2.y && r2.y < r1.y + r1.h) ||
		 (r2.y < r1.y && r1.y < r2.y + r2.h) ||
		 (r1.y < r2.y && r2.y + r2.h < r1.y + r1.h) ||
		 (r2.y < r1.y && r1.y + r1.h < r2.y + r2.h))
	);
}

b32 point_in_rect(v2 p, Rect r) {
	return (
		(r.x < p.x && p.x < r.x + r.w) &&
		(r.y < p.y && p.y < r.y + r.h)
	);
}

Rect rect_with_offset(v2 pos, Rect offset_rect) {
	return (Rect) {
		pos.x + offset_rect.x,
		pos.y + offset_rect.y,
		offset_rect.w,
		offset_rect.h
	};
}

// :window impl
Window window_new(const char* title, u32 width, u32 height) {
	panic(glfwInit(), "Failed to initialize glfw\n");

	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* glfw_window = glfwCreateWindow(
		width, height,
		title, NULL, NULL
	);
	panic(glfw_window, "Failed to create glfw window\n");

	glfwMakeContextCurrent(glfw_window);
	panic(glewInit() == GLEW_OK, "Failed to initialize glew\n");

	b32 should_close = glfwWindowShouldClose(glfw_window);
	return (Window) {
		.glfw_window = glfw_window,
		.width = width,
		.height = height,
		.should_close = should_close
	};
}

void window_delete(Window window) {
	glfwDestroyWindow(window.glfw_window);
}

void window_update(Window* window) {
	window->should_close = glfwWindowShouldClose(window->glfw_window);
	glfwSwapBuffers(window->glfw_window);
	glfwPollEvents();
}

// :frame controller impl
FrameController frame_controller_new(i32 fps) {
	return (FrameController) {
		.start_time = glfwGetTime(),
		.start_tick = glfwGetTime(),
		.unit_frame = 1.0f / fps,
		.dt = 0.0f,
		.frame = 0,
		.fps = 0
	};
}

void frame_controller_start(FrameController* fc) {
	fc->start_tick = glfwGetTime();
}

void frame_controller_end(FrameController* fc) {
	fc->frame++;

	fc->dt = glfwGetTime() - fc->start_tick;
	if (fc->unit_frame > fc->dt) {
		sleep(fc->unit_frame - fc->dt);
	}
	fc->dt = glfwGetTime() - fc->start_tick;

	if (glfwGetTime() - fc->start_time >= 1.0f) {
		fc->fps = fc->frame;
		fc->start_time = glfwGetTime();
		fc->frame = 0;
	}
}

// :event impl
void key_callback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods) {
	Event event = { 0 };
	event.e.key = key;
	switch (action) {
		case GLFW_PRESS:
		case GLFW_REPEAT:
			event.type = KEYDOWN;
			break;
		case GLFW_RELEASE:
			event.type = KEYUP;
			break;
	}

	event_enqueue(&ctx.e_queue, event);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	Event event = {0};

	switch (button) {
		case GLFW_MOUSE_BUTTON_LEFT:
			event.e.button = MOUSE_BUTTON_LEFT;
			break;
		case GLFW_MOUSE_BUTTON_RIGHT:
			event.e.button = MOUSE_BUTTON_RIGHT;
			break;
		default:
			panic(0, "Unhandled mouse button: %d\n", button);
	}

	switch (action) {
		case GLFW_PRESS:
			event.type = MOUSE_BUTTON_DOWN;
			break;
		case GLFW_RELEASE:
			event.type = MOUSE_BUTTON_UP;
			break;
		default:
			panic(0, "Unhandled mouse action: %d\n", action);
	}

	event_enqueue(&ctx.e_queue, event);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	Event event = { 0 };
	event.e.mouse_pos = (v2) { xpos, ypos };
	event.type = MOUSE_MOTION;
	event_enqueue(&ctx.e_queue, event);
}

b32 event_poll(Window window, Event* event) {
	glfwSetKeyCallback(window.glfw_window, key_callback);
	glfwSetMouseButtonCallback(window.glfw_window, mouse_button_callback);
	glfwSetCursorPosCallback(window.glfw_window, cursor_position_callback);

	if (ctx.e_queue.front != ctx.e_queue.back) {
		*event = event_dequeue(&ctx.e_queue);  // Dequeue the next event
		return true;  // Event found
	}
	return false;
}

v2 event_mouse_pos(Window window) {
	f64 xpos, ypos;
	glfwGetCursorPos(window.glfw_window, &xpos, &ypos);
	return (v2) { xpos, ypos };
}

void event_set_mouse_pos(Window window, v2 pos) {
	glfwSetCursorPos(window.glfw_window, pos.x, pos.y);
}

// :event queue impl
void event_enqueue(EventQueue* queue, Event e) {
	int next = (queue->back + 1) % MAX_EVENTS;
	panic(next != queue->front, "EventQueue is full\n");
	queue->events[queue->back] = e;
	queue->back = next;
}

Event event_dequeue(EventQueue* queue) {
	panic(queue->front != queue->back, "EventQueue is empty\n");
	Event value = queue->events[queue->front];
	queue->front = (queue->front + 1) % MAX_EVENTS;
	return value;
}

// :ocamera impl
OCamera ocamera_new(v2 pos, f32 zoom, OCamera_Boundary boundary) {
	return (OCamera) {
		.pos = pos,
		.zoom = zoom,
		.mvp = m4_zero(),
		.boundary = boundary,
		.active_x = false,
		.active_y = false
	};
}

void ocamera_change_zoom(OCamera* cam, f32 dz) {
	f32 temp = cam->zoom;
	temp += dz;
	if (temp <= 0.0f)
		return;
	cam->zoom = temp;
}

void ocamera_change_pos(OCamera* cam, v2 dp) {
	cam->pos = v2_add(cam->pos, dp);
}

void ocamera_follow(OCamera* cam, Rect to_follow_rect, v2 offset, f32 delay, v2 surf_size) {
	v2 gl_offset = dp_to_dgl_coords(offset, surf_size.x, surf_size.y);

	v2 to_follow = {
		to_follow_rect.x + to_follow_rect.w / 2,
		to_follow_rect.y + to_follow_rect.h / 2
	};
	v2 gl_to_follow = pixel_to_gl_coords(
		to_follow, surf_size.x, surf_size.y
	);

	cam->pos.x += (gl_to_follow.x - cam->pos.x - gl_offset.x) / delay;
	cam->pos.y += (gl_to_follow.y - cam->pos.y + gl_offset.y) / delay;
}

m4 ocamera_calc_mvp(OCamera* cam) {
	m4 proj = ortho_projection(
		cam->boundary.left,
		cam->boundary.right,
		cam->boundary.top,
		cam->boundary.bottom,
		cam->boundary.near,
		cam->boundary.far
	);
	m4 transform = m4_translate(
		m4_identity(),
		(v3) {
			cam->pos.x,
			cam->pos.y,
			0
		}
	);
	m4 transpose = m4_transpose(transform);
	m4 view_mat = m4_inverse(transpose);
	m4 model = m4_scale(cam->zoom);
	m4 vp = m4_mul(proj, view_mat);
	cam->mvp = m4_transpose(m4_mul(model, vp));
	return cam->mvp;
}

// :pcamera impl
PCamera pcamera_new(v3 pos, v3 dir, f32 sensitivity, PCamera_Info info) {
	dir = v3_normalize(dir);
	v3 up = { 0.0f, 1.0f, 0.0f };
	v3 right = v3_cross(dir, up);
	return (PCamera) {
		.pos = pos,
		.dir = dir,
		.up = up,
		.right = right,
		.pitch = 0.0f,
		.yaw = -90.0f,
		.mp = (v2) { 0, 0 },
		.sensitivity = sensitivity,
		.first = true,
		.mouse_enable = false,
		.info = info
	};
}

void pcamera_change_pos(PCamera* cam, v3 dp) {
	cam->pos = v3_add(cam->pos, dp);
}

void pcamera_handle_mouse(PCamera* cam, Window window) {
	if (!cam->mouse_enable) {
		glfwSetInputMode(
			window.glfw_window,
			GLFW_CURSOR,
			GLFW_CURSOR_NORMAL
		);
		return;
	}

	glfwSetInputMode(
		window.glfw_window,
		GLFW_CURSOR,
		GLFW_CURSOR_DISABLED
	);

	if (cam->first) {
		event_set_mouse_pos(
			window, (v2) {
				window.width / 2.0f,
				window.height / 2.0f
			}
		);
		cam->mp = event_mouse_pos(window);
		cam->first = false;
	}

	v2 p = event_mouse_pos(window);
	v2 dp = v2_sub(cam->mp, p);
	v2 sp = v2_mul_scalar(dp, cam->sensitivity);

	cam->yaw -= sp.x;
	cam->pitch += sp.y;

	if (cam->pitch > 89.0f) {
		cam->pitch = 89.0f;
	}
	else if (cam->pitch < -89.0f) {
		cam->pitch = -89.0f;
	}

	v3 front;
	front.x = cos(to_radians(cam->yaw)) * cos(to_radians(cam->pitch));
	front.y = sin(to_radians(cam->pitch));
	front.z = sin(to_radians(cam->yaw)) * cos(to_radians(cam->pitch));

	cam->dir = v3_normalize(front);
	cam->right = v3_normalize(
		v3_cross(cam->dir, (v3) { 0, 1, 0 })
	);
	cam->up = v3_normalize(
		v3_cross(cam->right, cam->dir)
	);

	cam->mp = p;
}

m4 pcamera_calc_mvp(PCamera* cam) {
	m4 proj = persp_projection(
		cam->info.aspect_ratio,
		cam->info.fov,
		cam->info.near,
		cam->info.far
	);

	m4 camera_mat = {
		.m = {
			{ cam->right.x, cam->right.y, cam->right.z, 0.0f },
			{ cam->up.x   , cam->up.y   , cam->up.z   , 0.0f },
			{ cam->dir.x  , cam->dir.y  , cam->dir.z  , 0.0f },
			{         0.0f,         0.0f,         0.0f, 1.0f }
		}
	};

	m4 camera_trans = m4_translate(
		m4_identity(),
		(v3) { -cam->pos.x, -cam->pos.y, -cam->pos.z }
	);

	cam->look_at = m4_mul(camera_mat, camera_trans);
	cam->mvp = m4_mul(proj, cam->look_at);
	return cam->mvp;
}

// :texture impl
Texture texture_from_file(const char* filepath, b32 flip) {
	stbi_set_flip_vertically_on_load(flip);

	i32 w, h, c;
	u8* data = stbi_load(filepath, &w, &h, &c, 0);
	panic(data, "Failed to load file: %s\n", filepath);

	// Binding the texture
	u32 id;
	GLCall(glGenTextures(1, &id));
	GLCall(glBindTexture(GL_TEXTURE_2D, id));

	// Setting up some basic modes to display texture
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

	// Sending the pixel data to opengl
	GLenum internal_format, format;
	if (c == 1) {
		internal_format = GL_RED;
		format = GL_RED;
	} else if (c == 3) {
		internal_format = GL_RGB;
		format = GL_RGB;
	} else if (c == 4) {
		internal_format = GL_RGBA;
		format = GL_RGBA;
	}

	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, internal_format, w, h, 0, format, GL_UNSIGNED_BYTE, data));
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));

	if (data) {
		stbi_image_free(data);
	}

	return (Texture) {
		id, w, h
	};
}

Texture texture_from_data(u32 width, u32 height, u32* data) {
	u32 id;
	GLCall(glGenTextures(1, &id));
	GLCall(glBindTexture(GL_TEXTURE_2D, id));
	
	// Setting up some basic modes to display texture
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	
	// Sending the pixel data to opengl
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data));
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));

	return (Texture) {
		id, width, height
	};
}

void texture_clear(Texture texture) {
	u32 t = 0;
	GLCall(glClearTexImage(texture.id, 0, GL_RGBA, GL_UNSIGNED_BYTE, &t));
}

void texture_bind(Texture texture) {
	GLCall(glBindTextureUnit(texture.id, texture.id));
}

void texture_unbind(Texture texture) {
	GLCall(glBindTextureUnit(texture.id, 0));
}

void texture_delete(Texture texture) {
	GLCall(glDeleteTextures(1, &texture.id));
}

// :shader impl
Shader shader_new(const char* v_src, const char* f_src) {
	u32 program = glCreateProgram();

	u32 vs = shader_compile(GL_VERTEX_SHADER,   v_src);
	u32 fs = shader_compile(GL_FRAGMENT_SHADER, f_src);

	// Attaching shader
	GLCall(glAttachShader(program, vs));
	GLCall(glAttachShader(program, fs));
	GLCall(glLinkProgram(program));
	GLCall(glValidateProgram(program));

	GLCall(glDeleteShader(vs));
	GLCall(glDeleteShader(fs));

	return program;
}

void shader_delete(Shader id) {
	GLCall(glDeleteProgram(id));
}

u32 shader_compile(ShaderType type, const char* shader_src) {
	u32 id = glCreateShader(type);
	GLCall(glShaderSource(id, 1, &shader_src, NULL));
	GLCall(glCompileShader(id));

	// Checking error in shader
	i32 result;
	GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &result));
	if (result == GL_FALSE) {
		i32 length;
		GLCall(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length));

		// TODO: This memory is not freed.
		char message[length];
		GLCall(glGetShaderInfoLog(id, length, &length, message));

		panic(false, "Failed to compile [%s shader]\n%s\n",
			(type == GL_VERTEX_SHADER ? "Vertex" : "Fragment"), message
		);
	}
	return id;
}

// :fbo impl
FBO fbo_new(u32 width, u32 height) {
	u32 id;

	// Generate and bind framebuffer
	GLCall(glGenFramebuffers(1, &id));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, id));

	// Color texture
	Texture color_texture = texture_from_data(width, height, NULL);
	GLCall(glFramebufferTexture2D(
		GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, color_texture.id, 0
	));

	// Setting up draw buffers
	GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 };
	GLCall(glDrawBuffers(1, attachments));

	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));

	panic(
		glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
		"Framebuffer is not complete!\n"
	);

	return (FBO) {
		.id = id,
		.color_texture = color_texture,
	};
}

void fbo_delete(FBO* fbo) {
	texture_delete(fbo->color_texture);
	GLCall(glDeleteFramebuffers(1, &fbo->id));
}

void fbo_bind(FBO* fbo) {
	texture_bind(fbo->color_texture);
	texture_clear(fbo->color_texture);
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, fbo->id));
}

void fbo_unbind() {
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

// :imr impl
const char* __internal_v_src =
	"#version 440 core\n"
	"layout (location = 0) in vec3 position;\n"
	"layout (location = 1) in vec4 color;\n"
	"layout (location = 2) in vec2 tex_coord;\n"
	"layout (location = 3) in float tex_id;\n"
	"uniform mat4 mvp;\n"
	"out vec4 o_color;\n"
	"out vec2 o_tex_coord;\n"
	"out float o_tex_id;\n"
	"void main() {\n"
	"o_color = color;\n"
	"o_tex_coord = tex_coord;\n"
	"o_tex_id = tex_id;\n"
	"gl_Position = mvp * vec4(position, 1.0f);\n"
	"}\n";

const char* __internal_f_src =
	"#version 440 core\n"
	"layout (location = 0) out vec4 color;\n"
	"uniform sampler2D textures[32];\n"
	"in vec4 o_color;\n"
	"in vec2 o_tex_coord;\n"
	"in float o_tex_id;\n"
	"void main() {\n"
	"int index = int(o_tex_id);\n"
	"color = texture(textures[index], o_tex_coord) * o_color;\n"
	"}\n";

IMR imr_new() {
	u32 vao, vbo;

	GLCall(glEnable(GL_BLEND));
	GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

	// Buffers
	GLCall(glGenVertexArrays(1, &vao));
	GLCall(glBindVertexArray(vao));

	GLCall(glGenBuffers(1, &vbo));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, vbo));
	GLCall(glBufferData(GL_ARRAY_BUFFER, MAX_VBO_SIZE, NULL, GL_DYNAMIC_DRAW));

	// VAO format
	STATIC_ASSERT(
		10 == sizeof(Vertex) / sizeof(f32),
		"Vertex has been updated. Update VAO format."
	);

	GLCall(glEnableVertexAttribArray(0));
	GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*) offsetof(Vertex, pos)));
	GLCall(glEnableVertexAttribArray(1));
	GLCall(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*) offsetof(Vertex, color)));
	GLCall(glEnableVertexAttribArray(2));
	GLCall(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*) offsetof(Vertex, tex_coord)));
	GLCall(glEnableVertexAttribArray(3));
	GLCall(glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*) offsetof(Vertex, tex_id)));

	// Generating white texture
	u32 data = 0xffffffff;
	Texture white = texture_from_data(1, 1, &data);
	texture_bind(white);

	// Shader
	Shader shader = shader_new(__internal_v_src, __internal_f_src);
	GLCall(glUseProgram(shader));

	// Providing texture samples
	u32 samplers[TEXTURE_SAMPLE_AMT];
	for (u32 i = 0; i < TEXTURE_SAMPLE_AMT; i++)
		samplers[i] = i;
	
	// Providing samplers to the shader
	int loc = GLCall(glGetUniformLocation(shader, "textures"));
	panic(loc != -1, "Cannot find uniform: textures\n");
	GLCall(glUniform1iv(loc, TEXTURE_SAMPLE_AMT, samplers));

	return (IMR) {
		.vao = vao,
		.vbo = vbo,
		.shader = shader,
		.def_shader = shader,
		.buff_idx = 0,
		.white = white
	};
}

void imr_delete(IMR* imr) {
	GLCall(glDeleteVertexArrays(1, &imr->vao));
	GLCall(glDeleteBuffers(1, &imr->vbo));
	texture_delete(imr->white);
	shader_delete(imr->shader);
	shader_delete(imr->def_shader);
}

void imr_clear(v4 color) {
	GLCall(glClearColor(color.r, color.g, color.b, color.a));
	GLCall(glClear(GL_COLOR_BUFFER_BIT));
}

void imr_begin(IMR* imr) {
	imr->buff_idx = 0;
	texture_bind(imr->white);
	GLCall(glUseProgram(imr->shader));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, imr->vbo));
}

void imr_end(IMR* imr) {
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, imr->vbo));
	GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(imr->buffer), imr->buffer));

	GLCall(glBindVertexArray(imr->vao));
	GLCall(glDrawArrays(GL_TRIANGLES, 0, imr->buff_idx / VERTEX_SIZE));
}

void imr_switch_shader(IMR* imr, Shader shader) {
	imr->shader = shader;
}

void imr_switch_shader_to_default(IMR* imr) {
	imr->shader = imr->def_shader;
	imr_reapply_samplers(imr);
}

void imr_reapply_samplers(IMR* imr) {
	GLCall(glUseProgram(imr->shader));

	// Providing texture samples
	u32 samplers[TEXTURE_SAMPLE_AMT];
	for (u32 i = 0; i < TEXTURE_SAMPLE_AMT; i++)
		samplers[i] = i;
	
	// Providing samplers to the shader
	int loc = GLCall(glGetUniformLocation(imr->shader, "textures"));
	panic(loc != -1, "Cannot find uniform: textures\n");
	GLCall(glUniform1iv(loc, TEXTURE_SAMPLE_AMT, samplers));
}

void imr_update_mvp(IMR* imr, m4 mvp) {
	i32 loc = GLCall(glGetUniformLocation(imr->shader, "mvp"));
	GLCall(glUniformMatrix4fv(loc, 1, GL_TRUE, &mvp.m[0][0]));
}

void imr_push_vertex(IMR* imr, Vertex v) {
	STATIC_ASSERT(
		10 == sizeof(Vertex) / sizeof(f32),
		"Vertex has been updated. Update this method."
	);

	imr->buffer[imr->buff_idx++] = v.pos.x;
	imr->buffer[imr->buff_idx++] = v.pos.y;
	imr->buffer[imr->buff_idx++] = v.pos.z;
	imr->buffer[imr->buff_idx++] = v.color.r;
	imr->buffer[imr->buff_idx++] = v.color.g;
	imr->buffer[imr->buff_idx++] = v.color.b;
	imr->buffer[imr->buff_idx++] = v.color.a;
	imr->buffer[imr->buff_idx++] = v.tex_coord.x;
	imr->buffer[imr->buff_idx++] = v.tex_coord.y;
	imr->buffer[imr->buff_idx++] = v.tex_id;
}

void imr_push_quad(IMR* imr, v3 pos, v2 size, m4 rot, v4 color) {
	Rect tex_rect = {
		0, 0, 1, 1
	};
	imr_push_quad_tex(imr, pos, size, tex_rect, imr->white.id, rot, color);
}

void imr_push_quad_tex(IMR* imr, v3 pos, v2 size, Rect tex_rect, f32 tex_id, m4 rot, v4 color) {
	if (((imr->buff_idx + 6 * VERTEX_SIZE) / VERTEX_SIZE) >= MAX_VERT_CNT) {
		imr_end(imr);
		imr_begin(imr);
	}

	Vertex p1, p2, p3, p4, p5, p6;

	// Rotating over origin
	p1.pos = m4_mul_v3(rot, (v3) { -size.x / 2, -size.y / 2, 0.0f });
	p2.pos = m4_mul_v3(rot, (v3) {  size.x / 2, -size.y / 2, 0.0f });
	p3.pos = m4_mul_v3(rot, (v3) {  size.x / 2,  size.y / 2, 0.0f });

	p4.pos = m4_mul_v3(rot, (v3) {  size.x / 2,  size.y / 2, 0.0f });
	p5.pos = m4_mul_v3(rot, (v3) { -size.x / 2,  size.y / 2, 0.0f });
	p6.pos = m4_mul_v3(rot, (v3) { -size.x / 2, -size.y / 2, 0.0f });

	// Shifting to the desired position
	p1.pos = v3_add(p1.pos, (v3) { pos.x + size.x / 2, pos.y + size.y / 2, pos.z });
	p2.pos = v3_add(p2.pos, (v3) { pos.x + size.x / 2, pos.y + size.y / 2, pos.z });
	p3.pos = v3_add(p3.pos, (v3) { pos.x + size.x / 2, pos.y + size.y / 2, pos.z });

	p4.pos = v3_add(p4.pos, (v3) { pos.x + size.x / 2, pos.y + size.y / 2, pos.z });
	p5.pos = v3_add(p5.pos, (v3) { pos.x + size.x / 2, pos.y + size.y / 2, pos.z });
	p6.pos = v3_add(p6.pos, (v3) { pos.x + size.x / 2, pos.y + size.y / 2, pos.z });

	// Making the texure coordinates
	p1.tex_coord = (v2) { tex_rect.x, tex_rect.y };
	p2.tex_coord = (v2) { tex_rect.x + tex_rect.w, tex_rect.y };
	p3.tex_coord = (v2) { tex_rect.x + tex_rect.w, tex_rect.y + tex_rect.h };
	p4.tex_coord = (v2) { tex_rect.x + tex_rect.w, tex_rect.y + tex_rect.h };
	p5.tex_coord = (v2) { tex_rect.x, tex_rect.y + tex_rect.h };
	p6.tex_coord = (v2) { tex_rect.x, tex_rect.y };

	p1.color = p2.color = p3.color = p4.color = p5.color = p6.color = color;
	p1.tex_id = p2.tex_id = p3.tex_id = p4.tex_id = p5.tex_id = p6.tex_id = tex_id;

	imr_push_vertex(imr, p1);
	imr_push_vertex(imr, p2);
	imr_push_vertex(imr, p3);
	imr_push_vertex(imr, p4);
	imr_push_vertex(imr, p5);
	imr_push_vertex(imr, p6);
}

void imr_push_triangle(IMR* imr, v3 p1, v3 p2, v3 p3, m4 rot, v4 color) {
	Triangle tex_coord = {
		(v3) { 0, 0, 0 },
		(v3) { 1, 0, 0 },
		(v3) { 1, 1, 0 }
	};
	imr_push_triangle_tex(imr, p1, p2, p3, tex_coord, imr->white.id, rot, color);
}

void imr_push_triangle_tex(IMR* imr, v3 p1, v3 p2, v3 p3, Triangle tex_coord, f32 tex_id, m4 rot, v4 color) {
	if (((imr->buff_idx + 3 * VERTEX_SIZE) / VERTEX_SIZE) >= MAX_VERT_CNT) {
		imr_end(imr);
		imr_begin(imr);
	}

	v3 centroid = {
		(p1.x + p2.x + p3.x) / 3.0f,
		(p1.y + p2.y + p3.y) / 3.0f,
		(p1.z + p2.z + p3.z) / 3.0f,
	};

	Vertex a1, a2, a3;

	// Rotating over origin
	a1.pos = m4_mul_v3(rot, (v3) { p1.x - centroid.x, p1.y - centroid.y, p1.z - centroid.z });
	a2.pos = m4_mul_v3(rot, (v3) { p2.x - centroid.x, p2.y - centroid.y, p2.z - centroid.z });
	a3.pos = m4_mul_v3(rot, (v3) { p3.x - centroid.x, p3.y - centroid.y, p3.z - centroid.z });

	// Shifting to the desired position
	a1.pos = v3_add(a1.pos, (v3) { centroid.x, centroid.y, centroid.z });
	a2.pos = v3_add(a2.pos, (v3) { centroid.x, centroid.y, centroid.z });
	a3.pos = v3_add(a3.pos, (v3) { centroid.x, centroid.y, centroid.z });

	// Texture coordinates
	a1.tex_coord = (v2) { tex_coord.a.x, tex_coord.a.y };
	a2.tex_coord = (v2) { tex_coord.b.x, tex_coord.b.y };
	a3.tex_coord = (v2) { tex_coord.c.x, tex_coord.c.y };

	a1.color = a2.color = a3.color = color;
	a1.tex_id = a2.tex_id = a3.tex_id = tex_id;

	imr_push_vertex(imr, a1);
	imr_push_vertex(imr, a2);
	imr_push_vertex(imr, a3);
}

// :external impl
#define STB_IMAGE_IMPLEMENTATION
#include "external/stb/stb_image.h"

#endif // BASE_IMPLEMENTATION

#endif // __BASE_H__
