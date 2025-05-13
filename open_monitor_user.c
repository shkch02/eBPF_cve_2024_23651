#include <bpf/libbpf.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "open_monitor.skel.h" // skeleton 헤더
#include <linux/types.h>

#define MAX_FILENAME_LEN 256
#define MAX_COMM 16

struct event_t {
    uint32_t pid;
    char comm[MAX_COMM];
    char filename[MAX_FILENAME_LEN];
};

static volatile sig_atomic_t exiting = 0;

void handle_signal(int sig) {
    exiting = 1;
}

static int handle_event(void *ctx, void *data, size_t data_sz) {
    struct event_t *e = data;
    printf("[openat] PID=%d COMM=%s FILE=%s\n", e->pid, e->comm, e->filename);
    return 0;
}

int main() {
    struct open_monitor_bpf *skel;
    struct ring_buffer *rb;
    int err;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    skel = open_monitor_bpf__open_and_load();
    if (!skel) {
        fprintf(stderr, "failed to load skeleton\n");
        return 1;
    }

    err = open_monitor_bpf__attach(skel);
    if (err) {
        fprintf(stderr, "failed to attach\n");
        return 1;
    }

    rb = ring_buffer__new(bpf_map__fd(skel->maps.events), handle_event, NULL, NULL);
    if (!rb) {
        fprintf(stderr, "failed to setup ring buffer\n");
        return 1;
    }

    printf("listening for openat() calls on /etc... Ctrl+C to exit\n");
    while (!exiting) {
        ring_buffer__poll(rb, 100);
    }

    ring_buffer__free(rb);
    open_monitor_bpf__destroy(skel);
    return 0;
}
