#define __TARGET_ARCH_x86

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

char LICENSE[] SEC("license") = "Dual BSD/GPL"; //eBPF는 라이센스 명시필수

#define MAX_FILENAME_LEN 256
#define MAX_COMM 16


//커널영역 분석
struct event_t { //유저 스페이스로 넘길 정보들의 구조 정의
    u32 pid;  //PID
    char comm[MAX_COMM]; //프로세스 이름
    char filename[MAX_FILENAME_LEN]; //열려고 한 파일 이름
};

struct { // 커널단 - 유저단 통신위해 map 사용
    __uint(type, BPF_MAP_TYPE_RINGBUF); //링버퍼사용, 유저는 해당 링 버퍼를 읽는다
    __uint(max_entries, 1 << 24); //버퍼크기 지정
} events SEC(".maps");

SEC("kprobe/do_sys_openat2") //do_sys_openat2에 kprobe를 후킹한다
int handle_open(struct pt_regs *ctx) { //시스템콜 인수 꺼내옴
    struct event_t *event;

    const char *filename = (const char *)PT_REGS_PARM2(ctx); //파일명 추출
    if (!filename) //파일명 널체크
        return 0;

    event = bpf_ringbuf_reserve(&events, sizeof(*event), 0); //링버퍼에 쓸데이터 공간 할당
    if (!event)
        return 0;

    event->pid = bpf_get_current_pid_tgid() >> 32; //bpf_get_current_pid_tgid()는 상위 32bit에 pid값 갖고있음, 그 값 추출
    bpf_get_current_comm(&event->comm, sizeof(event->comm)); // bpf_get_current_comm()로 명령어 이름 추출
    bpf_probe_read_user_str(&event->filename, sizeof(event->filename), filename); //bpf_probe_read_user_str() 파일명 복사

    // 감시 대상: /etc 경로인지 확인
    if (__builtin_memcmp(event->filename, "/etc", 4) == 0) { //경로명 앞 4글자 확인
        bpf_ringbuf_submit(event, 0);
    } else {
        bpf_ringbuf_discard(event, 0);
    }

    return 0;
}
