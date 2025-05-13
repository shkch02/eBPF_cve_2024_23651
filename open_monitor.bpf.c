#define __TARGET_ARCH_x86

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

char LICENSE[] SEC("license") = "Dual BSD/GPL";

#define MAX_FILENAME_LEN 256
#define MAX_COMM 16

struct event_t {
    u32 pid;
    char comm[MAX_COMM];
    char filename[MAX_FILENAME_LEN];
};

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 24);
} events SEC(".maps");

SEC("kprobe/do_sys_openat2")
int handle_open(struct pt_regs *ctx) {
    struct event_t *event;

    const char *filename = (const char *)PT_REGS_PARM2(ctx);
    if (!filename)
        return 0;

    event = bpf_ringbuf_reserve(&events, sizeof(*event), 0);
    if (!event)
        return 0;

    event->pid = bpf_get_current_pid_tgid() >> 32;
    bpf_get_current_comm(&event->comm, sizeof(event->comm));
    bpf_probe_read_user_str(&event->filename, sizeof(event->filename), filename);

    // 감시 대상: /etc 경로
    if (__builtin_memcmp(event->filename, "/etc", 4) == 0) {
        bpf_ringbuf_submit(event, 0);
    } else {
        bpf_ringbuf_discard(event, 0);
    }

    return 0;
}
