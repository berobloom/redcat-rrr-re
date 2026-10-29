/* reken_sim: the arithmetic part without a screen.

   usage: reken_sim gen <type 0..3> <level 0..5> <seed> <count>
              one line per seed: text|answer|seed after
          reken_sim prize <fuel> <repair> <boosts> <rockets> <seed> <count>
              one line per seed: prize|seed after
          reken_sim play [groep] [seed]
              Handig rekenen on stdin/stdout
          reken_sim shot <data dir> <out.ppm> [ms] [keys] [sum answer prize]
              the exercise screen after ms (default 1000), keys typed
              (digits, b = TERUG, o = OK); the first sum can be forced

   gen and prize are what port/tools/check_reken.py compares with the
   original code. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "archive.h"
#include "reken.h"
#include "reken_screen.h"

static const char *prize_name[7] = {
    "5 boosts", "5 mini-raketten", "volledige reparatie", "30 reparatie",
    "5 benzine", "10 benzine", "15 benzine",
};

static int play(int groep, uint32_t seed)
{
    rc_session s;
    rc_reken r;
    char line[64];

    rc_session_default(&s);
    s.groep = groep;
    rc_reken_start(&r, &s, seed);
    while (rc_module1_next(&r)) {
        printf("[benzine %d  reparatie %d  boosts %d  raketten %d]  prijs: %s\n",
               r.fuel, r.repair, r.boosts, r.rockets, prize_name[r.prize]);
        printf("%s", r.text);
        fflush(stdout);
        if (!fgets(line, sizeof line, stdin))
            return 0;
        printf(rc_reken_answer(&r, atoi(line)) ? "goed!\n" : "fout, het is %d\n",
               r.answer);
    }
    rc_reken_finish(&r, &s);
    printf("%d van de %d goed; benzine %d, reparatie %d, boosts %d, raketten %d\n",
           r.correct, RC_SUMS_PER_ROUND, s.fuel, s.repair, s.boosts, s.rockets);
    return 0;
}

static int shot(int argc, char **argv)
{
    static rc_exercise x;
    rc_archive reken, menu;
    rc_session s;
    const char *keys = argc > 5 ? argv[5] : "";
    int ms = argc > 4 ? atoi(argv[4]) : 1000, t, i;
    FILE *f;

    if (rc_archive_open(&reken, argv[2], "REKEN") != 0 ||
        rc_archive_open(&menu, argv[2], "MENU") != 0 ||
        rc_exercise_load(&x, &reken, &menu) != 0) {
        fprintf(stderr, "REKEN/MENU not loaded from %s\n", argv[2]);
        return 1;
    }
    rc_session_default(&s);
    rc_exercise_start(&x, &s, 1);
    if (argc > 8) {
        snprintf(x.r.text, sizeof x.r.text, "%s", argv[6]);
        x.r.answer = atoi(argv[7]);
        x.field = (int)strlen(argv[7]);
        x.r.prize = atoi(argv[8]);
    }
    for (t = 0; t < ms; t += 10) {
        rc_exercise_tick(&x, 10);
        if (t == 300)
            for (i = 0; keys[i]; i++)
                rc_exercise_key(&x, keys[i] == 'b' ? RC_KEY_BACK :
                                keys[i] == 'o' ? RC_KEY_OK : keys[i]);
    }
    rc_exercise_draw(&x);
    f = fopen(argv[3], "wb");
    if (!f)
        return 1;
    fprintf(f, "P6\n640 480\n255\n");
    for (i = 0; i < 640 * 480; i++) {
        rc_rgb c = x.pal[x.fb[i]];
        fputc(c.r, f); fputc(c.g, f); fputc(c.b, f);
    }
    fclose(f);
    return 0;
}

int main(int argc, char **argv)
{
    rc_reken r;
    int i, n;

    memset(&r, 0, sizeof r);
    if (argc >= 6 && strcmp(argv[1], "gen") == 0) {
        int type = atoi(argv[2]);
        uint32_t seed = strtoul(argv[4], NULL, 0);

        r.level = atoi(argv[3]);
        n = atoi(argv[5]);
        for (i = 0; i < n; i++) {
            r.seed = seed + i;
            r.text[0] = 0;
            r.answer = 0;
            rc_sum_make(&r, type);
            printf("%s|%d|%u\n", r.text, r.answer, r.seed);
        }
        return 0;
    }
    if (argc >= 8 && strcmp(argv[1], "prize") == 0) {
        uint32_t seed = strtoul(argv[6], NULL, 0);

        n = atoi(argv[7]);
        for (i = 0; i < n; i++) {
            r.fuel = atoi(argv[2]);
            r.repair = atoi(argv[3]);
            r.boosts = atoi(argv[4]);
            r.rockets = atoi(argv[5]);
            r.prize = 0;
            r.seed = seed + i;
            rc_prize_choose(&r);
            printf("%d|%u\n", r.prize, r.seed);
        }
        return 0;
    }
    if (argc >= 4 && strcmp(argv[1], "shot") == 0)
        return shot(argc, argv);
    if (argc >= 2 && strcmp(argv[1], "play") == 0)
        return play(argc > 2 ? atoi(argv[2]) : 6,
                    argc > 3 ? strtoul(argv[3], NULL, 0) : (uint32_t)time(NULL));
    fprintf(stderr, "usage: %s gen <type> <level> <seed> <count>\n"
            "       %s prize <fuel> <repair> <boosts> <rockets> <seed> <count>\n"
            "       %s play [groep] [seed]\n", argv[0], argv[0], argv[0]);
    return 2;
}
