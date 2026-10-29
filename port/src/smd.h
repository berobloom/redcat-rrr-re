/* The pupil files %8.8d.SMD: the results of each exercise, read by the
   menu's Rapport and written by REKEN at the end of a module ("somdata.c",
   REKEN 0x19cfb / 0x19f07, RRRMENU 0x12afd / 0x12d09). */
#ifndef RC_SMD_H
#define RC_SMD_H

#include <stdint.h>

#define RC_SMD_NAME 255         /* the pupil's name, the file's first bytes */
#define RC_SMD_STRIDE 0x5b      /* a record as read back */
#define RC_SMD_WRITTEN 27       /* a record as appended */

typedef struct {
    int32_t module;             /* 0..3 */
    int32_t asked, correct;
    char date[11];              /* "dd/mm/yyyy", the month counted from 0 */
    int32_t groep;
} rc_smd_record;

typedef struct {
    int number;                 /* the file's: 1 for 00000001.SMD */
    int count;
    rc_smd_record *rec;
} rc_smd;

/* The file whose name matches (as stricmp), else an empty one numbered
   after the files there.  0 on success (also when none matched). */
int rc_smd_load(rc_smd *p, const char *dir, const char *name);
void rc_smd_free(rc_smd *p);
/* One record more, written at once: the file as REKEN writes it. */
int rc_smd_append(const char *dir, const char *name, int module, int asked,
                  int correct, int groep, const char *date);
/* "%2.2d/%2.2d/%4d" of today, as 0x19cb6 makes it. */
void rc_smd_date(char out[11]);

/* Rapport's figures, in percent: the module's last record (0x12e8d, -1
   when none), the first n of its records together (0x12e12: all for n <= 0;
   -1 when the file has none), the number of its records (0x12fb4). */
int rc_smd_last(const rc_smd *p, int module);
int rc_smd_total(const rc_smd *p, int module, int n);
int rc_smd_count(const rc_smd *p, int module);

#endif
