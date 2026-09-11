#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define WINDOW_WIDTH  900
#define WINDOW_HEIGHT 900

/* We will use this renderer to draw into this window every frame. */
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

static SDL_FPoint base_points[] = {
  {50.0f,  450.0f},
  {150.0f, 440.0f},
  {250.0f, 350.0f},
  {300.0f, 100.0f}, // sharp peak
  {350.0f, 350.0f},
  {450.0f, 440.0f},
  {550.0f, 450.0f}
};
#define NUM_BASE_POINTS ((int)SDL_arraysize(base_points))

static SDL_FPoint points[WINDOW_WIDTH];
#define MAX_POINTS ((int)SDL_arraysize(points))
static int npoints;

static SDL_FPoint interpolate(float x);

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
  SDL_SetAppMetadata("Lagrange Interpolation", "1.0", "com.example.lagrange-interpolation");

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer("Lagrange Interpolation", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  /* keep the 900x900 drawing area intact when the window is resized. */
  SDL_SetRenderLogicalPresentation(renderer, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

  /* The Lagrange basis divides by (x_i - x_j), so the nodes must have distinct
     x values; the sampling loop below additionally assumes they are sorted. */
  for (int i = 1; i < NUM_BASE_POINTS; ++i) {
    if (base_points[i].x <= base_points[i - 1].x) {
      SDL_Log("base_points must be sorted by x with no repeated x values");
      return SDL_APP_FAILURE;
    }
  }

  for (int x = (int)base_points[0].x;
       x <= (int)base_points[NUM_BASE_POINTS - 1].x && npoints < MAX_POINTS;
       ++x)
    points[npoints++] = interpolate((float)x);

  return SDL_APP_CONTINUE;  /* carry on with the program! */
} 

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
  if (event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
  }
  return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void* appstate)
{
  /* clear the window to the draw color. */
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(renderer);

  /* draw basic points */
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
  for (int i = 0; i < NUM_BASE_POINTS; ++i)
  { /* draw red rects around base points for visual */
    SDL_FRect r = { base_points[i].x - 4, base_points[i].y - 4, 9, 9 };
    SDL_RenderRect(renderer, &r);
  }
  SDL_RenderPoints(renderer, base_points, NUM_BASE_POINTS);

  /* draw calculated points */
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
  if (npoints >= 2)
    SDL_RenderLines(renderer, points, npoints);

  /* put the newly-cleared rendering on the screen. */
  SDL_RenderPresent(renderer);

  return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
  /* SDL will clean up the window/renderer for us. */
}

/* apply lagrange interpolation to x using base_points */
static SDL_FPoint interpolate(float x)
{
  float y = 0.f, prod = 1.f;
  for (int i = 0; i < NUM_BASE_POINTS; ++i)
  {
    for (int j = 0; j < NUM_BASE_POINTS; ++j)
      if (j != i) /* skip same terms to avoid division by zero */
        prod *= (x - base_points[j].x) / (base_points[i].x - base_points[j].x);

    prod *= base_points[i].y;
    y += prod;
    prod = 1;
  }

  SDL_FPoint p = { x, y };
  return p;
}
