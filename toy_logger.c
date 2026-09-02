#include "toy_logger.h"

#ifdef TOY_VISUALIZE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdalign.h>
#include <inttypes.h>

static FILE *log_file = NULL;
static int event_seq = 0;

typedef union {
    struct {
        size_t size;
        size_t prev_size;
    };
    max_align_t _align;
} toy_header_t;

#define TOY_HEADER_SIZE (sizeof(toy_header_t))

void toy_logger_init(const char *filename) {
    if (log_file != NULL) return;

    const char *path = filename;
    if (path == NULL) {
        path = getenv("TOY_LOG_FILE");
        if (path == NULL || strlen(path) == 0) {
            path = "allocator_events.jsonl";
        }
    }

    log_file = fopen(path, "w");
    if (!log_file) {
        fprintf(stderr, "[toy_logger] Warning: Could not open %s for writing\n", path);
        return;
    }
    atexit(toy_logger_close);
}

void toy_logger_close(void) {
    if (log_file) {
        fflush(log_file);
        fclose(log_file);
        log_file = NULL;
    }
}

void toy_log_event(const char *event_type,
                   const char *call_str,
                   const char *description,
                   void *target_header,
                   size_t target_size,
                   int target_is_allocated,
                   void *sec_header,
                   size_t sec_size,
                   int sec_is_allocated,
                   const char *highlight_role,
                   void *heap_start,
                   void *movingptr,
                   void *heap_end) {
    (void)sec_is_allocated;
    if (log_file == NULL) {
        toy_logger_init(NULL);
        if (log_file == NULL) return;
    }

    event_seq++;

    uintptr_t u_start = (uintptr_t)heap_start;
    uintptr_t u_moving = (uintptr_t)movingptr;
    uintptr_t u_end = (uintptr_t)heap_end;
    size_t used_bytes = (u_moving >= u_start) ? (u_moving - u_start) : 0;
    size_t total_capacity = (u_end >= u_start) ? (u_end - u_start) : (1024 * 1024);

    fprintf(log_file, "{");
    fprintf(log_file, "\"seq\":%d,", event_seq);
    fprintf(log_file, "\"event\":\"%s\",", event_type ? event_type : "UNKNOWN");
    fprintf(log_file, "\"call\":\"%s\",", call_str ? call_str : "");
    fprintf(log_file, "\"desc\":\"%s\",", description ? description : "");
    fprintf(log_file, "\"target_addr\":\"%p\",", target_header);
    fprintf(log_file, "\"target_size\":%zu,", target_size);
    fprintf(log_file, "\"target_allocated\":%d,", target_is_allocated);
    fprintf(log_file, "\"sec_addr\":\"%p\",", sec_header);
    fprintf(log_file, "\"sec_size\":%zu,", sec_size);
    fprintf(log_file, "\"highlight_role\":\"%s\",", highlight_role ? highlight_role : "none");
    fprintf(log_file, "\"heap_start\":\"%p\",", heap_start);
    fprintf(log_file, "\"movingptr\":\"%p\",", movingptr);
    fprintf(log_file, "\"heap_end\":\"%p\",", heap_end);
    fprintf(log_file, "\"used_bytes\":%zu,", used_bytes);
    fprintf(log_file, "\"heap_capacity\":%zu,", total_capacity);

    // Walk current heap blocks
    fprintf(log_file, "\"blocks\":[");
    if (heap_start != NULL && movingptr != NULL && movingptr > heap_start) {
        toy_header_t *curr = (toy_header_t *)heap_start;
        int block_idx = 0;

        while ((void *)curr < movingptr) {
            size_t real_size = curr->size & ~1;
            int is_alloc = (int)(curr->size & 1);
            size_t prev_sz = curr->prev_size & ~1;
            int prev_alloc = (int)(curr->prev_size & 1);
            size_t total_block_size = real_size + TOY_HEADER_SIZE;
            size_t offset = (uintptr_t)curr - u_start;

            const char *status = is_alloc ? "allocated" : "free";
            const char *highlight = "normal";

            if ((void *)curr == target_header) {
                if (strcmp(event_type, "SPLIT") == 0) {
                    highlight = "just_split";
                    status = "allocated";
                } else if (strcmp(event_type, "COALESCE_FORWARD") == 0 ||
                           strcmp(event_type, "COALESCE_BACKWARD") == 0) {
                    highlight = "just_coalesced";
                    status = "free";
                } else if (strcmp(event_type, "FREE") == 0) {
                    highlight = "just_freed";
                    status = "free";
                } else if (strcmp(event_type, "ALLOC") == 0) {
                    highlight = "just_allocated";
                    status = "allocated";
                }
            } else if ((void *)curr == sec_header) {
                if (strcmp(event_type, "SPLIT") == 0) {
                    highlight = "just_split_leftover";
                    status = "free";
                }
            }

            if (block_idx > 0) fprintf(log_file, ",");
            fprintf(log_file, "{");
            fprintf(log_file, "\"id\":%d,", block_idx);
            fprintf(log_file, "\"addr\":\"%p\",", (void *)curr);
            fprintf(log_file, "\"payload_addr\":\"%p\",", (void *)((char *)curr + TOY_HEADER_SIZE));
            fprintf(log_file, "\"offset\":%zu,", offset);
            fprintf(log_file, "\"payload_size\":%zu,", real_size);
            fprintf(log_file, "\"total_size\":%zu,", total_block_size);
            fprintf(log_file, "\"allocated\":%d,", is_alloc);
            fprintf(log_file, "\"prev_size\":%zu,", prev_sz);
            fprintf(log_file, "\"prev_allocated\":%d,", prev_alloc);
            fprintf(log_file, "\"status\":\"%s\",", status);
            fprintf(log_file, "\"highlight\":\"%s\"", highlight);
            fprintf(log_file, "}");

            block_idx++;
            toy_header_t *next = (toy_header_t *)((char *)curr + total_block_size);
            if ((void *)next <= (void *)curr) {
                // Safeguard against corrupted loop
                break;
            }
            curr = next;
        }
    }
    fprintf(log_file, "]}");
    fprintf(log_file, "\n");
    fflush(log_file);
}

#endif // TOY_VISUALIZE
