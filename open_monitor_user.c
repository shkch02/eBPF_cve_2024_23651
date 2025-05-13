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

void handle_signal(int sig) { //종료 시그널 감지시 종료하게 메인루프만들어둠
    exiting = 1;
}

static int handle_event(void *ctx, void *data, size_t data_sz) { 
    struct event_t *e = data; //map 형태인 링버퍼에서 구조체 읽어서 data에 저장
    printf("[openat] PID=%d COMM=%s FILE=%s\n", e->pid, e->comm, e->filename);
    return 0;
}

int main() {
    struct open_monitor_bpf *skel; //스켈레톤 핸들러, eBPF프로그램 관리
    struct ring_buffer *rb; // 링 버퍼 핸들러
    int err; //오류 체크

    signal(SIGINT, handle_signal); //종료신호 받고 종료
    signal(SIGTERM, handle_signal);

    skel = open_monitor_bpf__open_and_load(); // BPF프로그램을 메모리에 로드하고 맵,섹션을 준비
    if (!skel) {
        fprintf(stderr, "failed to load skeleton\n"); //실패 
        return 1;
    }

    err = open_monitor_bpf__attach(skel); //SEC("kprobe/do_sys_openat2)를 실제커널 함수에 붙임
    if (err) {
        fprintf(stderr, "failed to attach\n");
        return 1;
    }

    rb = ring_buffer__new(bpf_map__fd(skel->maps.events), handle_event, NULL, NULL); //맵의 파일 디스크럽터를 가져와 링 버퍼로 데이터 수신받기위해 링버퍼 생성
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
