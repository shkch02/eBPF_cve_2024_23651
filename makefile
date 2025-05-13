BPF_OBJ=open_monitor.bpf.o
USER_OBJ=open_monitor_user

all: $(BPF_OBJ) $(USER_OBJ)

$(BPF_OBJ): open_monitor.bpf.c
	clang -g -O2 -target bpf -c $< -o $@

$(USER_OBJ): open_monitor_user.c
	gcc -I. -g -o $@ $< -lbpf -lelf -lz

clean:
	rm -f *.o *.skel.* open_monitor_user
