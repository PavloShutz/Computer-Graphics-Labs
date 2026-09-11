#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <math.h>

#define WINDOW_WIDTH  900
#define WINDOW_HEIGHT 900

/* how far the triangle turns for each pixel of horizontal mouse movement */
#define DEGREES_PER_PIXEL 1.0f

/* We will use this renderer to draw into this window every frame. */
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

static SDL_FPoint points[3] = {
  {450.f, 390.f},
  {398.f, 480.f},
  {502.f, 480.f}
}; /* triangle points */
static SDL_FPoint points_transformed[3];

static float alpha = 0.f;

static void rotate_triangle(void);

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
  SDL_SetAppMetadata("Affine Transformation", "1.0", "com.example.affine-tranformation");

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer("Affine Transformation", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  /* keep the 900x900 drawing area intact when the window is resized. */
  SDL_SetRenderLogicalPresentation(renderer, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

  rotate_triangle();  /* so the transformed triangle is valid before the first mouse move */

  return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
  if (event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
  }
  if (event->type == SDL_EVENT_MOUSE_MOTION) {
    /* turn by how far the mouse moved, not by how many events arrived. */
    alpha -= event->motion.xrel * DEGREES_PER_PIXEL;
    alpha = fmodf(alpha, 360.0f);

    rotate_triangle();
  }

  return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void* appstate)
{
  /* clear the window to the draw color. */
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(renderer);

  /* draw start points */
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
  SDL_RenderLine(renderer, points[0].x, points[0].y, points[1].x, points[1].y);
  SDL_RenderLine(renderer, points[1].x, points[1].y, points[2].x, points[2].y);
  SDL_RenderLine(renderer, points[2].x, points[2].y, points[0].x, points[0].y);

  /* draw transformed points*/
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderLine(renderer, points_transformed[0].x, points_transformed[0].y, points_transformed[1].x, points_transformed[1].y);
  SDL_RenderLine(renderer, points_transformed[1].x, points_transformed[1].y, points_transformed[2].x, points_transformed[2].y);
  SDL_RenderLine(renderer, points_transformed[2].x, points_transformed[2].y, points_transformed[0].x, points_transformed[0].y);

  /* put the newly-cleared rendering on the screen. */
  SDL_RenderPresent(renderer);

  return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
  /* SDL will clean up the window/renderer for us. */
}

/* Rotate the triangle by alpha degrees about its own centroid. A rotation
   matrix always turns about the origin, so the pivot has to be moved there
   first and put back afterwards: translate(+p) * rotate(alpha) * translate(-p). */
static void rotate_triangle(void)
{
  const float alpha_rad = alpha * (SDL_PI_F / 180.0f);
  const float c = cosf(alpha_rad);
  const float s = sinf(alpha_rad);

  SDL_FPoint pivot = { 0.f, 0.f };
  for (int i = 0; i < 3; ++i) {
    pivot.x += points[i].x;
    pivot.y += points[i].y;
  }
  pivot.x /= 3.f;
  pivot.y /= 3.f;

  for (int i = 0; i < 3; ++i) {
    const float dx = points[i].x - pivot.x;
    const float dy = points[i].y - pivot.y;
    points_transformed[i].x = pivot.x + dx * c + dy * s;
    points_transformed[i].y = pivot.y + dy * c - dx * s;
  }
}
