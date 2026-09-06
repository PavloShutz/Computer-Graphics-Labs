#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

#define WINDOW_WIDTH  900
#define WINDOW_HEIGHT 900

#define RADIUS 210

static SDL_FPoint points[4 * (2 * RADIUS + 1)];
static SDL_FPoint center = { WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 };

void initCircle(void);

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
  /* Create the window */
  if (!SDL_CreateWindowAndRenderer("Circle Interpolation", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OCCLUDED, &window, &renderer))
  {
    SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  initCircle();

  return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
  if (event->type == SDL_EVENT_QUIT)
    return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
  return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void* appstate)
{
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);  /* black, full alpha */
  SDL_RenderClear(renderer);  /* start with a blank canvas */
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);  /* white, full alpha */
  SDL_RenderPoints(renderer, points, SDL_arraysize(points));  /* draw all the points! */
  SDL_RenderPoint(renderer, center.x, center.y);

  SDL_RenderPresent(renderer);  /* put it all on the screen! */

  return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
}

/* initialize points of a circle whose center coincides with the center of the window */
void initCircle(void)
{
  int u = 0;
  points[0].x = center.x;
  points[0].y = center.y - RADIUS;

  /* 1st quadrant */
  for (int i = 1; i < SDL_arraysize(points) / 4; ++i)
  {
    if (u >= 0)
      u = u - 2 * (int)(center.y - points[i - 1].y) + 1;
    else
      u = u + 2 * (int)(points[i - 1].x - center.x) + 1;

    if (u >= 0)
    {
      points[i].x = points[i - 1].x;
      points[i].y = points[i - 1].y + 1;
    }
    else
    {
      points[i].y = points[i - 1].y;
      points[i].x = points[i - 1].x + 1;
    }
  }

  /* 4th quadrant */
  for (int i = SDL_arraysize(points) / 4; i < SDL_arraysize(points) / 2; ++i)
  {
    points[i].x = points[i - SDL_arraysize(points) / 4].x;
    points[i].y = 2 * center.y - points[i - SDL_arraysize(points) / 4].y;
  }

  /* 3rd quadrant */
  for (int i = SDL_arraysize(points) / 2; i < 3 * SDL_arraysize(points) / 4; ++i)
  {
    points[i].x = 2 * center.x - points[i - SDL_arraysize(points) / 2].x;
    points[i].y = 2 * center.y - points[i - SDL_arraysize(points) / 2].y;
  }

  /* 2nd quadrant */
  for (int i = 3 * SDL_arraysize(points) / 4; i < SDL_arraysize(points); ++i)
  {
    points[i].x = 2 * center.x - points[i - 3 * SDL_arraysize(points) / 4].x;
    points[i].y = points[i - 3 * SDL_arraysize(points) / 4].y;
  }
}