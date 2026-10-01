#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <stdio.h>

/* Wired USB-C and the HyperSpeed dongle. SDL's PS5 HID driver knows both. */
#define RAIJU_VENDOR 0x1532
#define RAIJU_WIRED 0x1024
#define RAIJU_DONGLE 0x1026

#define MAX_JOYSTICKS 8
#define WAIT_MS 2000
/* One cell per PS5 HID step: percent is min(level * 10 + 5, 100). */
#define BAR_CELLS 10

static const char *power_state_name(SDL_PowerState state)
{
    switch (state) {
    case SDL_POWERSTATE_ERROR:
        return "error";
    case SDL_POWERSTATE_UNKNOWN:
        return "unknown";
    case SDL_POWERSTATE_ON_BATTERY:
        return "on-battery";
    case SDL_POWERSTATE_NO_BATTERY:
        return "no-battery";
    case SDL_POWERSTATE_CHARGING:
        return "charging";
    case SDL_POWERSTATE_CHARGED:
        return "charged";
    default:
        return "unknown";
    }
}

static int is_raiju(Uint16 vendor, Uint16 product)
{
    return vendor == RAIJU_VENDOR && (product == RAIJU_WIRED || product == RAIJU_DONGLE);
}

/* 5% -> one block, 85% -> nine, 95% and 100% -> full. */
static void print_charge_bar(int percent)
{
    int filled;
    int i;

    if (percent < 0) {
        filled = 0;
    } else if (percent >= 100) {
        filled = BAR_CELLS;
    } else {
        filled = (percent + 5) / 10;
    }

    fputs("[", stdout);
    for (i = 0; i < BAR_CELLS; i++) {
        fputs(i < filled ? "█" : " ", stdout);
    }
    fputs("] ", stdout);
}

int main(int argc, char **argv)
{
    SDL_Joystick *open[MAX_JOYSTICKS];
    SDL_JoystickID open_id[MAX_JOYSTICKS];
    int nopen = 0;
    int got_percent = 0;
    Uint64 deadline;
    int i;

    (void)argc;
    (void)argv;

    /* The kernel node has no battery page. The percent comes from SDL's
       PS5 HID driver, which reads the charge nibble in the input report. */
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_ENHANCED_REPORTS, "1");

    if (!SDL_Init(SDL_INIT_JOYSTICK)) {
        fprintf(stderr, "raiju-level: SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    /* The dongle ignores its first input report, then adds the joystick.
       Battery is filled in on a later report, and only once the joystick
       is open. */
    deadline = SDL_GetTicks() + WAIT_MS;
    do {
        int count = 0;
        SDL_JoystickID *ids;

        SDL_UpdateJoysticks();
        ids = SDL_GetJoysticks(&count);
        for (i = 0; i < count; i++) {
            int j;
            int already = 0;
            SDL_Joystick *joy;

            for (j = 0; j < nopen; j++) {
                if (open_id[j] == ids[i]) {
                    already = 1;
                    break;
                }
            }
            if (already || nopen == MAX_JOYSTICKS) {
                continue;
            }
            joy = SDL_OpenJoystick(ids[i]);
            if (!joy) {
                fprintf(stderr, "raiju-level: open %s: %s\n",
                        SDL_GetJoystickNameForID(ids[i]), SDL_GetError());
                continue;
            }
            open[nopen] = joy;
            open_id[nopen] = ids[i];
            nopen++;
        }
        SDL_free(ids);

        for (i = 0; i < nopen; i++) {
            int percent = -1;

            SDL_GetJoystickPowerInfo(open[i], &percent);
            if (percent >= 0 &&
                (is_raiju(SDL_GetJoystickVendor(open[i]), SDL_GetJoystickProduct(open[i])) ||
                 nopen == 1)) {
                got_percent = 1;
            }
        }
        if (got_percent) {
            break;
        }
        SDL_Delay(20);
    } while (SDL_GetTicks() < deadline);

    if (nopen == 0) {
        fprintf(stderr, "raiju-level: no joystick\n");
        SDL_Quit();
        return 1;
    }

    got_percent = 0;
    for (i = 0; i < nopen; i++) {
        int percent = -1;
        SDL_PowerState state = SDL_GetJoystickPowerInfo(open[i], &percent);
        const char *name = SDL_GetJoystickName(open[i]);
        Uint16 vendor = SDL_GetJoystickVendor(open[i]);
        Uint16 product = SDL_GetJoystickProduct(open[i]);

        if (!name) {
            name = "unknown";
        }
        printf("%s (%04x:%04x): ", name, vendor, product);
        if (percent >= 0) {
            got_percent = 1;
            print_charge_bar(percent);
            printf("%d%% %s\n", percent, power_state_name(state));
        } else {
            printf("%s\n", power_state_name(state));
        }
        SDL_CloseJoystick(open[i]);
    }

    SDL_Quit();
    return got_percent ? 0 : 1;
}
