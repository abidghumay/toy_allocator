#ifndef TOY_LOGGER_H
#define TOY_LOGGER_H

#include <stddef.h>
#include <stdint.h>

#ifdef TOY_VISUALIZE

void toy_logger_init(const char *filename);
void toy_logger_close(void);
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
                   void *heap_end);

#define LOG_INIT(start, moving, end) \
    toy_log_event("INIT", "allocator_init()", "Heap initialized (1MB)", NULL, 0, 0, NULL, 0, 0, "none", start, moving, end)

#define LOG_ALLOC(hdr, size, call_str, desc, start, moving, end) \
    toy_log_event("ALLOC", call_str, desc, hdr, size, 1, NULL, 0, 0, "allocated", start, moving, end)

#define LOG_SPLIT(orig_hdr, leftover_hdr, orig_n, leftover_size, call_str, desc, start, moving, end) \
    toy_log_event("SPLIT", call_str, desc, orig_hdr, orig_n, 1, leftover_hdr, leftover_size, 0, "split", start, moving, end)

#define LOG_FREE(hdr, size, call_str, desc, start, moving, end) \
    toy_log_event("FREE", call_str, desc, hdr, size, 0, NULL, 0, 0, "freed", start, moving, end)

#define LOG_COALESCE_FWD(hdr, merged_with_hdr, old_size, new_size, desc, start, moving, end) \
    toy_log_event("COALESCE_FORWARD", "coalesce_forward", desc, hdr, new_size, 0, merged_with_hdr, old_size, 0, "coalesced_fwd", start, moving, end)

#define LOG_COALESCE_BWD(hdr, merged_into_hdr, old_size, new_size, desc, start, moving, end) \
    toy_log_event("COALESCE_BACKWARD", "coalesce_backward", desc, merged_into_hdr, new_size, 0, hdr, old_size, 0, "coalesced_bwd", start, moving, end)

#define LOG_REALLOC(hdr, size, call_str, desc, start, moving, end) \
    toy_log_event("REALLOC", call_str, desc, hdr, size, 1, NULL, 0, 0, "reallocated", start, moving, end)

#else // !TOY_VISUALIZE

#define toy_logger_init(fn) ((void)0)
#define toy_logger_close() ((void)0)
#define LOG_INIT(start, moving, end) ((void)0)
#define LOG_ALLOC(hdr, size, call_str, desc, start, moving, end) ((void)0)
#define LOG_SPLIT(orig_hdr, leftover_hdr, orig_n, leftover_size, call_str, desc, start, moving, end) ((void)0)
#define LOG_FREE(hdr, size, call_str, desc, start, moving, end) ((void)0)
#define LOG_COALESCE_FWD(hdr, merged_with_hdr, old_size, new_size, desc, start, moving, end) ((void)0)
#define LOG_COALESCE_BWD(hdr, merged_into_hdr, old_size, new_size, desc, start, moving, end) ((void)0)
#define LOG_REALLOC(hdr, size, call_str, desc, start, moving, end) ((void)0)

#endif // TOY_VISUALIZE

#endif // TOY_LOGGER_H
