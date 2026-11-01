/* SDL3 port of Alien Arcade.
   The matching decompilation in src/ is the specification. It is not
   compiled into this program, and nothing here is allowed to change it.

   Matching map for this shell:
   WinMain / MainWndProc (0002:0000 / 0002:0179)     main event loop
   CreateMainWindow (0003:0150)                      window / renderer setup
   LayoutMain / PaintMain (000E:0000 / 000E:0214)    via select_paint_frame
   RunStartDialog (0022:091F)                        intro_*
   RunPlay / PlayWndProc (000D:03AE / 000D:0443)     play_* / confirm 3|4
   RunResultDialog (0015:0109)                       pause overlay
   ConfirmChildCmd / ConfirmDlgProc
     (0010:0323 / 0010:0396)                         leave_* (template 300)
   RunNextDialog / NextDlgProc
     (0012:0372 / 0012:0390)                         ending 3 / board_until
   RecordScore (0016:00F2)                           ending 1..2 / scores_*
   SelectVolume (0017:02D5)                          Ctrl+0..9 in live play
   Accel tables 1–3 + MsgHookProc (0005:04A8)        key routing below
   CreateMainWindow opens a borderless popup the size of the desktop and
   refuses a screen smaller than 512 by 342. String 8 asks for 640 by 480;
   the port uses a fixed 640×480 logical window instead. */

#include <stdio.h>
#include <time.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "intro.h"
#include "play.h"
#include "scores.h"
#include "select.h"
#include "sound.h"
#include "text.h"

#define GAME_W 640
#define GAME_H 480

/* dir ends in a separator. Returns a copy of "<dir>build/res", or NULL. */
static char *resources_under(const char *dir)
{
    char probe[1024];
    char found[1024];
    char cursor[1024];
    int depth;

    if (!dir || !dir[0])
        return NULL;
    SDL_strlcpy(cursor, dir, sizeof(cursor));
    for (depth = 0; depth < 8; depth++) {
        size_t n;
        char *slash;

        SDL_snprintf(probe, sizeof(probe), "%sbuild/res/AARCADE.RC", cursor);
        if (SDL_GetPathInfo(probe, NULL)) {
            SDL_snprintf(found, sizeof(found), "%sbuild/res", cursor);
            return SDL_strdup(found);
        }
        n = SDL_strlen(cursor);
        if (n <= 1)
            break;
        cursor[n - 1] = '\0';
        slash = SDL_strrchr(cursor, '/');
        if (!slash)
            break;
        slash[1] = '\0';
    }
    return NULL;
}

/* Pictures and sounds extracted by tools/extract_res.py. */
static char *find_resources(void)
{
    char *cwd = SDL_GetCurrentDirectory();
    char *found = resources_under(cwd);
    const char *base;

    SDL_free(cwd);
    if (found)
        return found;
    base = SDL_GetBasePath();
    return resources_under(base);
}

/* PORT_SHOT=path writes one frame as a PPM and exits. For checking the picture. */
static int write_shot(SDL_Renderer *renderer, const char *path)
{
    SDL_Surface *shot = SDL_RenderReadPixels(renderer, NULL);
    SDL_Surface *rgba;
    FILE *fp;
    int x;
    int y;

    if (!shot)
        return 0;
    rgba = SDL_ConvertSurface(shot, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(shot);
    if (!rgba)
        return 0;
    fp = fopen(path, "wb");
    if (!fp) {
        SDL_DestroySurface(rgba);
        return 0;
    }
    fprintf(fp, "P6\n%d %d\n255\n", rgba->w, rgba->h);
    for (y = 0; y < rgba->h; y++) {
        const uint8_t *row = (const uint8_t *)rgba->pixels + (size_t)y * (size_t)rgba->pitch;
        for (x = 0; x < rgba->w; x++)
            fwrite(row + x * 4, 1, 3, fp);
    }
    fclose(fp);
    SDL_DestroySurface(rgba);
    return 1;
}

/* orig: MainWndProc IDC_NEXT/IDC_PLAY tail (0002:0179);
   attract path = ++g_nRound + LoadRoundScores (0016:0025) */
static void dismiss_board(Play **rounds, int *round, int *ending, int *attract_board,
                          int *attract, Intro **opening, SDL_Renderer *renderer,
                          const char *resources, Scores *scores)
{
    *ending = 0;
    if (*attract_board) {
        *attract_board = 0;
        scores_clear_ranks(scores);
        if (++*attract > 6)
            *attract = 1;
        if (resources)
            *opening = intro_new(renderer, resources);
        return;
    }
    if (*round >= 1 && *round <= 6 && rounds[*round - 1]) {
        play_free(rounds[*round - 1]);
        rounds[*round - 1] = NULL;
    }
    *round = 0;
}

/* orig: PlayBorderProc timer (000D:0000) / PlayDlgProc -1,1,2 (000D:0105)
   → RunPlay → IDC_NEXT (0002:0179) */
static void select_idle(Scores *scores, int attract, int *ending, int *attract_board,
                        time_t *board_until, Intro **opening, SDL_Renderer *renderer,
                        const char *resources)
{
    if (scores) {
        scores_show(scores, attract);
        *ending = 3;
        *attract_board = 1;
        *board_until = time(NULL) + 15;
    } else if (resources)
        *opening = intro_new(renderer, resources);
}

/* orig: ConfirmChildCmd (0010:0323) / ConfirmDlgProc (0010:0396),
   template 300, strings 138/139 */
static void leave_boxes(SDL_FRect *frame, SDL_FRect *yes, SDL_FRect *no)
{
    int ox;
    int oy;

    select_place(130 * 2, 70 * 2, &ox, &oy);

    frame->x = (float)ox;
    frame->y = (float)oy;
    frame->w = 130 * 2;
    frame->h = 70 * 2;
    yes->x = (float)(ox + 17 * 2);
    yes->y = (float)(oy + 39 * 2);
    yes->w = 37 * 2;
    yes->h = 17 * 2;
    no->x = (float)(ox + 74 * 2);
    no->y = (float)(oy + 39 * 2);
    no->w = 37 * 2;
    no->h = 17 * 2;
}

static int leave_hit(float x, float y)
{
    SDL_FRect frame, yes, no;

    leave_boxes(&frame, &yes, &no);
    if (x >= yes.x && y >= yes.y && x < yes.x + yes.w && y < yes.y + yes.h)
        return 1;
    if (x >= no.x && y >= no.y && x < no.x + no.w && y < no.y + no.h)
        return 2;
    return 0;
}

static void leave_draw(SDL_Renderer *renderer, int quit)
{
    SDL_FRect frame, yes, no;
    float tw;
    const char *word = quit ? "quit" : "abort";
    char text[96];

    leave_boxes(&frame, &yes, &no);
    SDL_SetRenderDrawColor(renderer, 192, 192, 192, 255);
    SDL_RenderFillRect(renderer, &frame);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &frame);
    SDL_snprintf(text, sizeof(text),
                 "Are you sure you want to %s? The game in progress will be lost.", word);
    text_block(renderer, frame.x + 9 * 2, frame.y + 7 * 2, 111.0f * 2.0f, text, 9, 1, 0, 0, 0);
    SDL_SetRenderDrawColor(renderer, 192, 192, 192, 255);
    SDL_RenderFillRect(renderer, &yes);
    SDL_RenderFillRect(renderer, &no);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &yes);
    SDL_RenderRect(renderer, &no);
    tw = (float)text_width("Yes", 9, 1);
    text_draw(renderer, yes.x + (yes.w - tw) / 2.0f, yes.y + (yes.h - (float)text_line(9, 1)) / 2.0f,
              "Yes", 9, 1, 0, 0, 0);
    tw = (float)text_width("No", 9, 1);
    text_draw(renderer, no.x + (no.w - tw) / 2.0f, no.y + (no.h - (float)text_line(9, 1)) / 2.0f,
              "No", 9, 1, 0, 0, 0);
}

/* orig: ResumeTimeLeft (001D:0430) after ConfirmChildCmd returns No */
static void leave_no(Play *play)
{
    if (!play)
        return;
    if (play_paused(play))
        play_resume(play);
    else
        play_catchup(play);
}

/* orig: ResultItemProc WM_CHAR (0015:0000) — bare modifiers do not resume */
static int pause_resume_key(SDL_Keycode key)
{
    if (key == SDLK_LCTRL || key == SDLK_RCTRL || key == SDLK_LSHIFT ||
        key == SDLK_RSHIFT || key == SDLK_LALT || key == SDLK_RALT ||
        key == SDLK_LGUI || key == SDLK_RGUI || key == SDLK_CAPSLOCK ||
        key == SDLK_NUMLOCKCLEAR || key == SDLK_SCROLLLOCK)
        return 0;
    if (key >= SDLK_F1 && key <= SDLK_F24)
        return 0;
    if (key == SDLK_UP || key == SDLK_DOWN || key == SDLK_LEFT || key == SDLK_RIGHT ||
        key == SDLK_HOME || key == SDLK_END || key == SDLK_PAGEUP || key == SDLK_PAGEDOWN ||
        key == SDLK_INSERT || key == SDLK_DELETE || key == SDLK_PRINTSCREEN ||
        key == SDLK_PAUSE || key == SDLK_MENU || key == SDLK_APPLICATION)
        return 0;
    return 1;
}

int main(int argc, char **argv)
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    Select *screen = NULL;
    Intro *opening = NULL;
    Scores *scores = NULL;
    Play *rounds[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
    /* 0 is play or the menu, 1 the score box, 2 the name, 3 the lists. */
    int ending = 0;
    int confirm = 0;
    /* g_nRound. The idle loop shows this game's scores, then advances. */
    int attract = 1;
    int attract_board = 0;
    /* Mouse-up dismisses only if the press also began on the board; otherwise
       releasing OK on Game Over / name would close the lists immediately. */
    int board_press = 0;
    /* orig: UserWndProc TrackUserButton on pause user item (000A:0EBE) */
    int pause_press = 0;
    time_t board_until = 0;
    char *resources;
    const char *shot;
    const char *start;
    const char *ticks;
    const char *fire_every;
    int round = 0;
    int burst = 0;
    int fire_n = 0;
    bool running = true;

    (void)argc;
    (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("Alien Arcade", GAME_W, GAME_H, SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("SDL_CreateWindow: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderLogicalPresentation(renderer, GAME_W, GAME_H,
                                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    text_open(renderer);
    SDL_SetRenderVSync(renderer, 1);

    resources = find_resources();
    if (resources) {
        screen = select_load(renderer, resources);
        sound_open(resources);
        scores = scores_open(renderer, resources);
    }
    else
        SDL_Log("no build/res yet; run tools/extract_res.py for pictures and sounds");

    shot = SDL_getenv("PORT_SHOT");
    start = SDL_getenv("PORT_ROUND");
    ticks = SDL_getenv("PORT_TICKS");
    fire_every = SDL_getenv("PORT_FIRE");
    if (start) {
        int n = SDL_atoi(start);
        if (n >= 1 && n <= 6) {
            round = n;
            attract = n;
        }
    }
    if (ticks)
        burst = SDL_atoi(ticks);
    if (fire_every)
        fire_n = SDL_atoi(fire_every);
    if (resources && round == 0)
        opening = intro_new(renderer, resources);

    while (running) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            /* orig: SC_CLOSE → command 101 (0002:0179) while a game is up */
            if (event.type == SDL_EVENT_QUIT) {
                if (ending == 0 && round != 0 && rounds[round - 1] && confirm == 0)
                    confirm = 4;
                else if (confirm == 0)
                    running = false;
            } else if (event.type == SDL_EVENT_TEXT_INPUT && ending == 2)
                scores_type(scores, event.text.text);
            else if (event.type == SDL_EVENT_KEY_DOWN && ending == 2 &&
                     event.key.key == SDLK_BACKSPACE)
                scores_backspace(scores);
            else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
                /* Accel tables 1–3 (AARCADE.RC) + MsgHookProc (0005:04A8):
                   table 1 = live play (incl. Ctrl+0..9 volume); table 2 = Q only
                   in other dialogs; table 3 = pause (Q / Esc / A). */
                SDL_Keymod mod = event.key.mod;
                int chord = (mod & (SDL_KMOD_CTRL | SDL_KMOD_ALT)) != 0;
                SDL_Keycode key = event.key.key;

                if (opening) {
                    int n = intro_key(opening, key, mod);

                    if (n == 2 || n == 101)
                        running = false;
                    else if (n == 1) {
                        intro_free(opening);
                        opening = NULL;
                    }
                } else if (ending == 0 && round == 0 && screen) {
                    int n = select_key(screen, key, mod);

                    if (n == 101)
                        running = false;
                    else if (n == -1)
                        select_idle(scores, attract, &ending, &attract_board, &board_until,
                                    &opening, renderer, resources);
                    else if (n >= 1 && n <= 6) {
                        round = n;
                        attract = n;
                    }
                } else if (confirm == 3 || confirm == 4) {
                    int yes = key == SDLK_Y || key == SDLK_RETURN || key == SDLK_KP_ENTER;
                    int no = key == SDLK_N || key == SDLK_ESCAPE;

                    if (yes && confirm == 4)
                        running = false;
                    else if (yes) {
                        if (scores) {
                            scores_show(scores, round);
                            ending = 3;
                            attract_board = 0;
                            board_press = 0;
                            board_until = time(NULL) + 15;
                        } else
                            dismiss_board(rounds, &round, &ending, &attract_board, &attract,
                                          &opening, renderer, resources, scores);
                    } else if (no && round >= 1 && rounds[round - 1])
                        leave_no(rounds[round - 1]);
                    if (yes || no)
                        confirm = 0;
                } else if (confirm) {
                    /* Templates 280/290: No is the default — Enter means No. */
                    int yes = key == SDLK_Y;
                    int no = key == SDLK_N || key == SDLK_ESCAPE ||
                             key == SDLK_RETURN || key == SDLK_KP_ENTER;

                    if (yes || no) {
                        if (yes) {
                            if (confirm == 2)
                                scores_erase_all(scores);
                            else
                                scores_reset_round(scores);
                        }
                        confirm = 0;
                        board_until = time(NULL) + 15;
                    }
                } else if (ending == 0 && round != 0 && rounds[round - 1] &&
                           (key == SDLK_ESCAPE || (chord && (key == SDLK_A || key == SDLK_Q)))) {
                    /* Step/Frame ignore Ctrl-Q (101); help ignores Esc. */
                    if (play_modal(rounds[round - 1])) {
                        if (key == SDLK_ESCAPE)
                            play_key(rounds[round - 1], key);
                    } else
                        confirm = (chord && key == SDLK_Q) ? 4 : 3;
                } else if (chord && key == SDLK_Q && ending != 1 && ending != 2) {
                    /* RoundDone / NameDlgProc ignore table-2 Q. */
                    running = false;
                } else if (ending == 3 && key == SDLK_R && !(mod & SDL_KMOD_CTRL)) {
                    confirm = (mod & SDL_KMOD_SHIFT) ? 2 : 1;
                } else if (ending == 3 &&
                           (key == SDLK_RETURN || key == SDLK_KP_ENTER ||
                            key == SDLK_ESCAPE)) {
                    /* NextDlgProc: only dialog commands dismiss the board. */
                    board_press = 0;
                    dismiss_board(rounds, &round, &ending, &attract_board, &attract,
                                  &opening, renderer, resources, scores);
                } else if (ending == 1 &&
                           (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER ||
                            event.key.key == SDLK_SPACE)) {
                    if (scores_needs_name(scores)) {
                        ending = 2;
                        SDL_StartTextInput(window);
                        sound_play(0x2331, 1);
                    } else {
                        ending = 3;
                        board_press = 0;
                        board_until = time(NULL) + 15;
                    }
                } else if (ending == 2 &&
                           (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER)) {
                    SDL_StopTextInput(window);
                    if (!scores_name_blank(scores))
                        scores_commit(scores);
                    ending = 3;
                    board_press = 0;
                    board_until = time(NULL) + 15;
                    if (scores_daily_place(scores))
                        sound_play(0x2331, 1);
                } else if (ending == 0 && round != 0 && rounds[round - 1]) {
                    Play *play = rounds[round - 1];

                    /* orig: SelectVolume (0017:02D5) — accel table 1, Ctrl only */
                    if ((mod & SDL_KMOD_CTRL) && key >= SDLK_0 && key <= SDLK_9 &&
                        !play_paused(play) && !play_modal(play)) {
                        sound_volume(key - SDLK_0);
                    } else if (play_paused(play)) {
                        /* orig: PlayWndProc 102 (000D:0443) → RunResultDialog */
                        if (pause_resume_key(key))
                            play_resume(play);
                    } else if (event.key.key == SDLK_P && chord)
                        play_pause(play);
                    else if (event.key.key == SDLK_SPACE) {
                        if (!play_pause(play))
                            play_key(play, event.key.key);
                    } else
                        play_key(play, event.key.key);
                }
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                       event.button.button == SDL_BUTTON_LEFT) {
                float x = 0;
                float y = 0;

                SDL_RenderCoordinatesFromWindow(renderer, event.button.x, event.button.y, &x, &y);
                if (opening)
                    intro_press(opening, x, y);
                else if (confirm == 3 || confirm == 4) {
                    int hit = leave_hit(x, y);

                    if (hit == 1 && confirm == 4)
                        running = false;
                    else if (hit == 1) {
                        if (scores) {
                            scores_show(scores, round);
                            ending = 3;
                            attract_board = 0;
                            board_press = 0;
                            board_until = time(NULL) + 15;
                        } else
                            dismiss_board(rounds, &round, &ending, &attract_board, &attract,
                                          &opening, renderer, resources, scores);
                    } else if (hit == 2 && round >= 1 && rounds[round - 1])
                        leave_no(rounds[round - 1]);
                    if (hit != 0)
                        confirm = 0;
                } else if (confirm) {
                    int hit = scores_hit_confirm(x, y, confirm == 2);

                    if (hit == 3) {
                        if (confirm == 2)
                            scores_erase_all(scores);
                        else
                            scores_reset_round(scores);
                        confirm = 0;
                        board_until = time(NULL) + 15;
                    } else if (hit == 1) {
                        confirm = 0;
                        board_until = time(NULL) + 15;
                    }
                } else if (ending == 3 && confirm == 0 && scores_hit_board(x, y)) {
                    board_press = 1;
                } else if (ending == 1 && scores_hit_over(scores, x, y)) {
                    if (scores_needs_name(scores)) {
                        ending = 2;
                        SDL_StartTextInput(window);
                        sound_play(0x2331, 1);
                    } else {
                        ending = 3;
                        board_press = 0;
                        board_until = time(NULL) + 15;
                    }
                } else if (ending == 2 && scores_hit_name_ok(scores, x, y)) {
                    SDL_StopTextInput(window);
                    if (!scores_name_blank(scores))
                        scores_commit(scores);
                    ending = 3;
                    board_press = 0;
                    board_until = time(NULL) + 15;
                    if (scores_daily_place(scores))
                        sound_play(0x2331, 1);
                } else if (ending == 0 && round == 0 && screen)
                    select_press(screen, x, y);
                else if (ending == 0 && round != 0 && rounds[round - 1] &&
                         play_paused(rounds[round - 1]))
                    pause_press = 1;
                else if (ending == 0 && round != 0 && rounds[round - 1]) {
                    play_pointer(rounds[round - 1], x, y);
                    play_press(rounds[round - 1]);
                }
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
                       event.button.button == SDL_BUTTON_LEFT) {
                float x = 0;
                float y = 0;

                SDL_RenderCoordinatesFromWindow(renderer, event.button.x, event.button.y, &x, &y);
                if (opening) {
                    if (intro_release(opening, x, y) == 1) {
                        intro_free(opening);
                        opening = NULL;
                    }
                } else if (ending == 3 && confirm == 0 && board_press &&
                           scores_hit_board(x, y)) {
                    /* UserWndProc: press+release inside a user item → dismiss. */
                    board_press = 0;
                    dismiss_board(rounds, &round, &ending, &attract_board, &attract,
                                  &opening, renderer, resources, scores);
                } else if (pause_press && ending == 0 && round != 0 && rounds[round - 1] &&
                           play_paused(rounds[round - 1])) {
                    pause_press = 0;
                    play_resume(rounds[round - 1]);
                } else {
                    board_press = 0;
                    pause_press = 0;
                    if (ending == 0 && round == 0 && screen) {
                        int n = select_release(screen, x, y);

                        if (n == -1)
                            select_idle(scores, attract, &ending, &attract_board, &board_until,
                                        &opening, renderer, resources);
                        else if (n >= 1 && n <= 6) {
                            round = n;
                            attract = n;
                        }
                    } else if (ending == 0 && round != 0 && rounds[round - 1] &&
                               !play_paused(rounds[round - 1])) {
                        play_pointer(rounds[round - 1], x, y);
                        play_release(rounds[round - 1]);
                    }
                }
            } else if (event.type == SDL_EVENT_MOUSE_MOTION && ending == 0 && confirm < 3 &&
                       round != 0 && rounds[round - 1]) {
                float x = 0;
                float y = 0;

                SDL_RenderCoordinatesFromWindow(renderer, event.motion.x, event.motion.y, &x, &y);
                play_pointer(rounds[round - 1], x, y);
            }
        }

        if (opening && intro_tick(opening)) {
            intro_free(opening);
            opening = NULL;
        }
        if (!opening && ending == 0 && round == 0 && screen && resources &&
            select_tick(screen))
            select_idle(scores, attract, &ending, &attract_board, &board_until, &opening,
                        renderer, resources);
        if (ending == 3 && confirm == 0 && time(NULL) >= board_until) {
            board_press = 0;
            dismiss_board(rounds, &round, &ending, &attract_board, &attract,
                          &opening, renderer, resources, scores);
        }
        if (round != 0 && rounds[round - 1] == NULL && resources && ending == 0) {
            rounds[round - 1] = play_load(renderer, resources, round);
            if (!rounds[round - 1])
                round = 0;
        }
        if (ending == 0 && confirm < 3 && round != 0 && rounds[round - 1] && !shot) {
            float mx = 0;
            float my = 0;
            float x = 0;
            float y = 0;

            SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mx, &my);

            SDL_RenderCoordinatesFromWindow(renderer, mx, my, &x, &y);
            play_pointer(rounds[round - 1], x, y);
            if (!play_paused(rounds[round - 1]))
                play_button(rounds[round - 1],
                            (buttons & (SDL_BUTTON_MASK(SDL_BUTTON_LEFT) |
                                        SDL_BUTTON_MASK(SDL_BUTTON_RIGHT))) != 0);
            if (play_tick(rounds[round - 1])) {
                if (scores) {
                    scores_present(scores, play_round(rounds[round - 1]),
                                   play_score(rounds[round - 1]));
                    ending = 1;
                } else {
                    play_free(rounds[round - 1]);
                    rounds[round - 1] = NULL;
                    round = 0;
                }
            }
        } else if (ending == 0 && round != 0 && rounds[round - 1] && burst > 0) {
            int i;

            for (i = 0; i < burst; i++) {
                if (fire_n > 0 && i % fire_n == 0)
                    play_click(rounds[round - 1]);
                if (play_step(rounds[round - 1]))
                    break;
            }
            burst = 0;
        }

        sound_tick();
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);
        if (opening)
            intro_draw(renderer, opening);
        else if (round != 0 && rounds[round - 1] && (ending == 0 || ending == 1))
            play_draw(renderer, rounds[round - 1]);
        else if (round != 0 && rounds[round - 1] && ending >= 2)
            /* Name/board: play window is gone; keep the cabinet frame only. */
            play_draw_border(renderer, rounds[round - 1]);
        else if (screen && (ending == 0 || attract_board))
            select_draw(renderer, screen);
        if (ending == 1)
            scores_draw_over(renderer, scores);
        else if (ending == 2)
            scores_draw_name(renderer, scores);
        else if (ending == 3) {
            scores_draw_board(renderer, scores);
            if (confirm)
                scores_draw_confirm(renderer, confirm == 2);
        }
        if (confirm == 3 || confirm == 4)
            leave_draw(renderer, confirm == 4);
        if (shot) {
            if (!write_shot(renderer, shot))
                SDL_Log("could not write %s", shot);
            break;
        }
        SDL_RenderPresent(renderer);
    }

    {
        int i;
        for (i = 0; i < 6; i++)
            play_free(rounds[i]);
    }
    intro_free(opening);
    select_free(screen);
    scores_close(scores);
    sound_close();
    text_close();
    SDL_free(resources);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
