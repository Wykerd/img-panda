/*#include <lexbor/html/parser.h>
#include <lexbor/dom/interfaces/element.h>
#include "img-panda/identify/metadata.h"
#include "img-panda/identify/wordpress.h"*/

#include "img-panda/comics/index.h"
#include <stdio.h>

void
handle_complete (imp_wc_indexer_state_t *state, imp_net_status_t *status) {
    printf("--------------------\nINDEXING IS COMPLETE\nStatus: %d\nCode: %d\n--------------------\n", status->type, status->code);
    imp_wc_indexer_shutdown(state);
}

int
main(int argc, const char *argv[])
{
    imp_wc_indexer_state_t state;
    imp_wc_indexer_init (uv_default_loop(), &state);
    imp_wc_indexer_run (&state, "https://www.comicwebsite.com/", 29, handle_complete);
    uv_run(state.loop, UV_RUN_DEFAULT);
    puts("SHUTDOWN");
    //imp_wc_indexer_shutdown(&state);
}
