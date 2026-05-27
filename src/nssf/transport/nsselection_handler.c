#include "nsselection_handler.h"
#include "NSSelectionGet.h"

#include <stdio.h>
#include <string.h>

static const char NSSELECTION_GET_501_BODY[] =
    "{"
    "\"type\":\"urn:5gc:nssf:tracer-bullet-stub\","
    "\"title\":\"Not Implemented\","
    "\"status\":501,"
    "\"detail\":\"Phase 1 tracer-bullet stub\""
    "}";

int nsselection_get_handle(void *unused_request,
                           char *out_body,
                           size_t out_size)
{
    (void)unused_request;

    if (out_body != NULL && out_size > 0) {
        size_t copy_len = sizeof(NSSELECTION_GET_501_BODY) - 1;
        if (copy_len >= out_size) {
            copy_len = out_size - 1;
        }
        memcpy(out_body, NSSELECTION_GET_501_BODY, copy_len);
        out_body[copy_len] = '\0';
    }

    return 501;
}
