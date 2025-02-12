#include <array>
#include <raylib.h>
#include <raymath.h>

using i32 = signed int;
using i64 = signed long int;

using u32 = unsigned int;
using u64 = unsigned long int;

using f32 = float;
using f64 = double;

static constexpr i32 WIDTH = 1200;
static constexpr i32 HEIGHT = 800;

static constexpr i32 BRUSH_SEGMENTS = 200;

static constexpr i32 CANVAS_WIDTH = 8192;
static constexpr i32 CANVAS_HEIGHT = 8192;

static constexpr f32 MIN_ZOOM = 0.4f;
static constexpr f32 MAX_ZOOM = 10.0f;

static constexpr f32 MIN_BRUSH_RADIUS = 1.0f;
static constexpr f32 MAX_BRUSH_RADIUS = 300.0f;

static constexpr f32 MAX_SCREEN_STEP_SIZE = 2.0f;

static constexpr Color BACKGROUND_COLOR = {0x18, 0x18, 0x18, 0xFF};

static constexpr KeyboardKey EXIT_KEY = KEY_ESCAPE;
static constexpr KeyboardKey PANNING_KEY = KEY_SPACE;
static constexpr KeyboardKey COLOR_SELECTOR_KEY = KEY_C;
static constexpr KeyboardKey BRUSH_RADIUS_SELECTOR_KEY = KEY_B;

static constexpr Color colors[] = {
  GRAY,
  DARKGRAY,
  YELLOW,
  GOLD,
  ORANGE,
  PINK,
  RED,
  MAROON,
  GREEN,
  LIME,
  DARKGREEN,
  SKYBLUE,
  BLUE,
  DARKBLUE,
  PURPLE,
  VIOLET,
  DARKPURPLE,
  BEIGE,
  BROWN,
  DARKBROWN,
  WHITE,
  BLACK,
  MAGENTA,
  RAYWHITE
};

static constexpr u64 COLORS_COUNT = (sizeof(colors) / sizeof(Color));

static constexpr f32 COLORS_PADDING = 5.0f;
static constexpr Vector2 COLOR_PREVIEW_SIZE = {27.4f, 40.0f};
static constexpr Vector2 COLOR_SELECTOR_WINDOW_SIZE = {200.0f, 185.5f};
static constexpr Vector2 COLOR_SELECTOR_WINDOW_CURSOR_PADDING = {50.0f, -150.0f};

static constexpr Vector2 COLOR_POSITIONS[] = {
  {5.0, 5.0},
  {37.4, 5.0},
  {69.8, 5.0},
  {102.2, 5.0},
  {134.6, 5.0},
  {167.0, 5.0},
  {5.0, 50.0},
  {37.4, 50.0},
  {69.8, 50.0},
  {102.2, 50.0},
  {134.6, 50.0},
  {167.0, 50.0},
  {5.0, 95.0},
  {37.4, 95.0},
  {69.8, 95.0},
  {102.2, 95.0},
  {134.6, 95.0},
  {167.0, 95.0},
  {5.0, 140.0},
  {37.4, 140.0},
  {69.8, 140.0},
  {102.2, 140.0},
  {134.6, 140.0},
  {167.0, 140.0},
};

static constexpr std::array<Color, 62> color_map = []() constexpr {
  std::array<Color, 62> color_map;
  color_map[39] = LIGHTGRAY;
  color_map[34] = GRAY;
  color_map[18] = DARKGRAY;
  color_map[61] = YELLOW;
  color_map[14] = GOLD;
  color_map[26] = ORANGE;
  color_map[4]  = PINK;
  color_map[3]  = RED;
  color_map[6]  = MAROON;
  color_map[25] = GREEN;
  color_map[29] = LIME;
  color_map[19] = DARKGREEN;
  color_map[37] = SKYBLUE;
  color_map[9]  = BLUE;
  color_map[23] = DARKBLUE;
  color_map[31] = PURPLE;
  color_map[36] = VIOLET;
  color_map[20] = DARKPURPLE;
  color_map[10] = BEIGE;
  color_map[35] = BROWN;
  color_map[24] = DARKBROWN;
  color_map[15] = WHITE;
  color_map[40] = BLACK;
  color_map[30] = BLANK;
  color_map[27] = MAGENTA;
  color_map[13] = RAYWHITE;
  return color_map;
}();

static constexpr f32 F32_UNINITIALIZED = -1;

static bool color_selector_mode = false;
static f32 color_selector_mode_end_time = F32_UNINITIALIZED;
static Vector2 color_selector_entered_position = Vector2 {F32_UNINITIALIZED};

static Vector2 mouse_pos = {-1, -1};
static Vector2 dmouse_pos = {-1, -1};

static Color brush_color = {0, 0xFA, 0xB, 0xFF};

static f32 brush_radius = 3.0f;
static f32 outline_outer_radius = brush_radius + 0.5f;

static RenderTexture2D target;

static Camera2D camera = {
    .offset = {(f32)        WIDTH / 2.0f, (f32)        HEIGHT / 2.0f},
    .target = {(f32) CANVAS_WIDTH / 2.0f, (f32) CANVAS_HEIGHT / 2.0f},
    .rotation = 0.0f,
    .zoom = 1.0f
};

static inline void stop_color_selector_mode(void)
{
	color_selector_entered_position = {F32_UNINITIALIZED};
  color_selector_mode = false;
}

static inline Rectangle get_tile_rect(Vector2 rpos, u64 color_idx)
{
  const auto pos = rpos + COLOR_POSITIONS[color_idx];
  return {
    .x = pos.x,
    .y = pos.y,
    .width = COLOR_PREVIEW_SIZE.x,
    .height = COLOR_PREVIEW_SIZE.y
  };
}

static inline i32 check_color_selector_collisions(Vector2 mouse_pos)
{
  const auto rpos = color_selector_entered_position + COLOR_SELECTOR_WINDOW_CURSOR_PADDING;
  for (u64 color_idx = 0; color_idx < COLORS_COUNT; color_idx++) {
    const auto tile_rect = get_tile_rect(rpos, color_idx);
    if (CheckCollisionPointRec(mouse_pos, tile_rect)) {
      return (i32) color_idx;
    }
  }

  return -1;
}

static inline void handle_color_selector_mode(void)
{
  const auto rpos = color_selector_entered_position + COLOR_SELECTOR_WINDOW_CURSOR_PADDING;
  DrawRectangleV(rpos, COLOR_SELECTOR_WINDOW_SIZE, WHITE);

  for (u64 color_idx = 0; color_idx < COLORS_COUNT; color_idx++) {
    const auto tile_rect = get_tile_rect(rpos, color_idx);
    const auto draw_pos = rpos + COLOR_POSITIONS[color_idx];
    DrawRectangleV(draw_pos, COLOR_PREVIEW_SIZE, colors[color_idx]);
    DrawRectangleLinesEx(tile_rect, 1.0f, BLACK);
  }
}

static inline void handle_brush_radius_selector_mode(void)
{
  DrawRingLines(mouse_pos,
                brush_radius,
                outline_outer_radius,
                0.0f,
                365.0f,
                BRUSH_SEGMENTS,
                WHITE);
}

static inline void draw(void)
{
  if (color_selector_mode_end_time != F32_UNINITIALIZED) {
    if (GetTime() - color_selector_mode_end_time > 0.25) {
      color_selector_mode_end_time = F32_UNINITIALIZED;
    } else {
      return;
    }
  }

  if (dmouse_pos.x == -1) {
    dmouse_pos = mouse_pos;
  }

  const auto world_pos = GetScreenToWorld2D(mouse_pos, camera);
  const auto dworld_pos = GetScreenToWorld2D(dmouse_pos, camera);

  BeginTextureMode(target);
  DrawLineEx(dworld_pos, world_pos, brush_radius, brush_color);
  EndTextureMode();
}

static inline void draw_whole_texture(Texture texture)
{
  DrawTextureRec(texture,
                 {0, 0, (f32) texture.width, -(f32) texture.height},
                 Vector2Zeros, WHITE);
}

static inline void handle_keyboard_input(void)
{
  if (IsKeyDown(PANNING_KEY)) {
    const auto d = GetMouseDelta();
    camera.target.x -= d.x / camera.zoom;
    camera.target.y -= d.y / camera.zoom;
  }

  if (IsKeyPressed(COLOR_SELECTOR_KEY)) {
    if (!color_selector_mode) {
      color_selector_mode = true;
      color_selector_entered_position = mouse_pos;
    } else {
      stop_color_selector_mode();
    }
  }

  if (IsKeyPressed(EXIT_KEY)) {
    if (color_selector_mode) {
      stop_color_selector_mode();
    }
  }
}

static inline void handle_mouse_wheel(void)
{
  const auto wmove = GetMouseWheelMove();
  if (wmove != 0) {
    if (IsKeyDown(BRUSH_RADIUS_SELECTOR_KEY)) {
      brush_radius += wmove * 1.0f * GetTime();
      brush_radius = Clamp(brush_radius, MIN_BRUSH_RADIUS, MAX_BRUSH_RADIUS);
      outline_outer_radius = brush_radius + 0.5f;
    } else {
      camera.zoom += wmove * 0.1f;
      camera.zoom = Clamp(camera.zoom, MIN_ZOOM, MAX_ZOOM);
    }
  }
}

static inline void handle_mouse_input(void)
{
  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    if (color_selector_mode) {
      const auto tile_idx = check_color_selector_collisions(mouse_pos);
      if (tile_idx >= 0) {
        color_selector_mode_end_time = GetTime();
        brush_color = colors[tile_idx];
      }
      stop_color_selector_mode();
    } else {
      draw();
    }
    dmouse_pos = mouse_pos;
  } else {
    dmouse_pos = {-1, -1};
  }
}

int main(void)
{
  SetTargetFPS(60);
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
  InitWindow(WIDTH, HEIGHT, "paint");
  SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
  HideCursor();

  target = LoadRenderTexture(CANVAS_WIDTH, CANVAS_HEIGHT);

  BeginTextureMode(target);
  ClearBackground(BACKGROUND_COLOR);
  EndTextureMode();

  while (!WindowShouldClose()) {
    mouse_pos = GetMousePosition();

    if (color_selector_mode) {
      ShowCursor();
      SetMouseCursor(MOUSE_CURSOR_ARROW);
    } else {
      HideCursor();
    }

    handle_keyboard_input();
    handle_mouse_wheel();
    handle_mouse_input();

    const auto drawing = IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    BeginDrawing();
    {
      ClearBackground(LIGHTGRAY);

      BeginMode2D(camera);
        draw_whole_texture(target.texture);
      EndMode2D();

      if (!drawing) {
        if (color_selector_mode) {
          handle_color_selector_mode();
        }

        if (IsKeyDown(BRUSH_RADIUS_SELECTOR_KEY)) {
          handle_brush_radius_selector_mode();
        } else {
          DrawCircleV(mouse_pos, brush_radius, brush_color);
        }
      }
    }
    EndDrawing();
  }

  UnloadRenderTexture(target);
  CloseWindow();

  return 0;
}
