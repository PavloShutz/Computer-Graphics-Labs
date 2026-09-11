/*
 * Headless smoke runner for the labs.
 *
 * The labs are written against SDL's main-callback API, so none of them has a
 * main() of its own: SDL_main.h normally generates one that calls SDL_AppInit,
 * then SDL_AppIterate forever, then SDL_AppQuit once the user closes the
 * window. That is fine for a person at a keyboard and useless in CI.
 *
 * This file is linked against a lab in place of that generated entry point (the
 * lab is compiled with SDL_MAIN_NOIMPL, see add_lab() in ../CMakeLists.txt) and
 * drives the very same callbacks for a bounded number of frames. Paired with
 * SDL_VIDEODRIVER=dummy it answers the question CI actually needs answered:
 * does this lab still start up, render frames, handle events and shut down?
 *
 * It is a smoke test, not a correctness test. It does not know what a circle or
 * a rotated triangle should look like; it proves the program runs.
 */
#define SDL_MAIN_USE_CALLBACKS 1  /* pulls in the SDL_App* declarations */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

/* SDL_main.h renames main() so its own entry point can call ours. Here we are
   the entry point, so put the name back. */
#undef main

#define DEFAULT_FRAMES 120

/* Mouse movement is the only input any lab reacts to so far (AffineTransformation
   rotates its triangle by motion.xrel), and a dummy video driver never produces
   any. Synthesising motion keeps SDL_AppEvent on the tested path instead of
   leaving it covered only by the quit event below. */
static void inject_mouse_motion(int frame)
{
  SDL_Event event;

  SDL_zero(event);
  event.motion.type = SDL_EVENT_MOUSE_MOTION;
  event.motion.timestamp = SDL_GetTicksNS();
  event.motion.x = (float)(frame % 900);
  event.motion.y = (float)(frame % 900);
  event.motion.xrel = (frame % 2) ? 7.0f : -5.0f;
  event.motion.yrel = 0.0f;

  SDL_PushEvent(&event);
}

/* Returns false if a callback reported failure. */
static bool pump_events(void *appstate, SDL_AppResult *result)
{
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    *result = SDL_AppEvent(appstate, &event);
    if (*result != SDL_APP_CONTINUE) {
      return *result != SDL_APP_FAILURE;
    }
  }
  return true;
}

int main(int argc, char *argv[])
{
  int frames = DEFAULT_FRAMES;
  bool inject_input = true;
  void *appstate = NULL;
  SDL_AppResult result;
  int frame;

  for (int i = 1; i < argc; ++i) {
    if (SDL_strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
      frames = SDL_atoi(argv[++i]);
    } else if (SDL_strcmp(argv[i], "--no-input") == 0) {
      inject_input = false;
    } else {
      SDL_Log("usage: %s [--frames N] [--no-input]", argv[0]);
      return 2;
    }
  }

  if (frames <= 0) {
    SDL_Log("--frames must be positive, got %d", frames);
    return 2;
  }

  result = SDL_AppInit(&appstate, argc, argv);
  if (result == SDL_APP_FAILURE) {
    SDL_Log("SDL_AppInit failed: %s", SDL_GetError());
    SDL_AppQuit(appstate, result);
    return 1;
  }

  for (frame = 0; frame < frames && result == SDL_APP_CONTINUE; ++frame) {
    if (inject_input) {
      inject_mouse_motion(frame);
    }
    SDL_PumpEvents();
    if (!pump_events(appstate, &result)) {
      SDL_Log("SDL_AppEvent failed on frame %d: %s", frame, SDL_GetError());
      SDL_AppQuit(appstate, result);
      return 1;
    }
    if (result != SDL_APP_CONTINUE) {
      break;
    }

    result = SDL_AppIterate(appstate);
    if (result == SDL_APP_FAILURE) {
      SDL_Log("SDL_AppIterate failed on frame %d: %s", frame, SDL_GetError());
      SDL_AppQuit(appstate, result);
      return 1;
    }
  }

  /* A lab that ran its frames should still quit when asked to. Every lab
     answers SDL_EVENT_QUIT with SDL_APP_SUCCESS, so anything else is a
     regression in its event handling. */
  if (result == SDL_APP_CONTINUE) {
    SDL_Event quit_event;

    SDL_zero(quit_event);
    quit_event.quit.type = SDL_EVENT_QUIT;
    quit_event.quit.timestamp = SDL_GetTicksNS();

    result = SDL_AppEvent(appstate, &quit_event);
    if (result != SDL_APP_SUCCESS) {
      SDL_Log("lab did not exit on SDL_EVENT_QUIT (returned %d)", (int)result);
      SDL_AppQuit(appstate, result);
      return 1;
    }
  }

  /* Read the driver name before tearing SDL down; afterwards there isn't one. */
  SDL_Log("ok: %d frame(s), video driver '%s'", frame, SDL_GetCurrentVideoDriver());

  SDL_AppQuit(appstate, result);
  SDL_Quit();
  return 0;
}
