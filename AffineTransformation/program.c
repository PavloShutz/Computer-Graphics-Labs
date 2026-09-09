#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <math.h>

#define WINDOW_WIDTH  900
#define WINDOW_HEIGHT 900

/* We will use this renderer to draw into this window every frame. */
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

static SDL_FPoint points[3] = {
  {450.f, 390.f},
  {398.f, 480.f},
  {502.f, 480.f}
}; /* triangle points */
static SDL_FPoint points_transformed[3];

static SDL_FPoint mouse_delta;

static float alpha = 0.f;

static void rotate_triangle();

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
  SDL_SetRenderLogicalPresentation(renderer, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_LOGICAL_PRESENTATION_DISABLED);

  return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
  if (event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
  }
  if (event->type == SDL_EVENT_MOUSE_MOTION) {
    SDL_GetRelativeMouseState(&mouse_delta.x, &mouse_delta.y);

    if (mouse_delta.x > 0)
      --alpha;
    else
      ++alpha;
    
    rotate_triangle();

    SDL_LogInfo(0, "(%f, %f)", mouse_delta.x, mouse_delta.y);
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

static void rotate_triangle()
{
  for (int i = 0; i < 3; ++i) {
    float alpha_rad = alpha * (SDL_PI_F / 180.0f);
    points_transformed[i].x = points[i].x * cosf(alpha_rad) + points[i].y * sinf(alpha_rad);
    points_transformed[i].y = points[i].y * cosf(alpha_rad) - points[i].x * sinf(alpha_rad);
  }
}