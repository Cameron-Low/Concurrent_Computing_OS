#include "libc.h"

// String to int
int atoi(char* x) {
    char* p = x;
    bool s = false;
    int r = 0;

    // Check the sign
    if (*p == '-') {
        s =  true;
        p++;
    } else if (*p == '+') {
        s = false;
        p++;
    } else {
        s = false;
    }

    for (int i = 0; *p != '\x00'; i++, p++) {
        r = s ? (r * 10) - (*p - '0') : (r * 10) + (*p - '0') ;
    }

    return r;
}

// Int to string
void itoa(char* r, int x) {
    char* p = r;
    int t, n;

    if (x < 0) {
        p++;
        t = -x;
        n = t;
    } else {
        t = +x;
        n = t;
    }

    do {
        p++;
        n /= 10;
    } while (n);

    *p-- = '\0';

    do {
        *p-- = '0' + (t % 10);
        t /= 10;
    } while(t);

    if( x < 0 ) {
        *p-- = '-';
    }
}

void yield() {
  asm volatile( "svc %0     \n" // make system call SYS_YIELD
              :
              : "I" (SYS_YIELD)
              : );

  return;
}

int write( int fd, const void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = fd
                "mov r1, %3 \n" // assign r1 =  x
                "mov r2, %4 \n" // assign r2 =  n
                "svc %1     \n" // make system call SYS_WRITE
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r) 
              : "I" (SYS_WRITE), "r" (fd), "r" (x), "r" (n)
              : "r0", "r1", "r2" );

  return r;
}

int  read( int fd,       void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = fd
                "mov r1, %3 \n" // assign r1 =  x
                "mov r2, %4 \n" // assign r2 =  n
                "svc %1     \n" // make system call SYS_READ
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r) 
              : "I" (SYS_READ),  "r" (fd), "r" (x), "r" (n) 
              : "r0", "r1", "r2" );

  return r;
}

int  fork() {
  int r;

  asm volatile( "svc %1     \n" // make system call SYS_FORK
                "mov %0, r0 \n" // assign r  = r0 
              : "=r" (r) 
              : "I" (SYS_FORK)
              : "r0" );

  return r;
}

void exit( int x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 =  x
                "svc %0     \n" // make system call SYS_EXIT
              :
              : "I" (SYS_EXIT), "r" (x)
              : "r0" );

  return;
}

void exec( const void* x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 = x
                "svc %0     \n" // make system call SYS_EXEC
              :
              : "I" (SYS_EXEC), "r" (x)
              : "r0" );

  return;
}

int  kill( int pid, int x ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 =  pid
                "mov r1, %3 \n" // assign r1 =    x
                "svc %1     \n" // make system call SYS_KILL
                "mov %0, r0 \n" // assign r0 =    r
              : "=r" (r) 
              : "I" (SYS_KILL), "r" (pid), "r" (x)
              : "r0", "r1" );

  return r;
}

void nice( int pid, int x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 =  pid
                "mov r1, %2 \n" // assign r1 =    x
                "svc %0     \n" // make system call SYS_NICE
              : 
              : "I" (SYS_NICE), "r" (pid), "r" (x)
              : "r0", "r1" );

  return;
}

uint32_t* sem_init(int val) {
    uint32_t* sem;
    asm volatile( "mov r0, %2 \n" // assign r0 = val
                  "svc %1     \n" // make system call SYS_SEM_INIT
                  "mov %0, r0 \n"
              : "=r" (sem)
              : "I" (SYS_SEM_INIT), "r" (val)
              : );
    return sem;
}

void sem_close(uint32_t* sem) {
    asm volatile( "mov r0, %1 \n" // assign r0 = sem
                  "svc %0     \n" // make system call SYS_SEM_CLOSE
              :
              : "I" (SYS_SEM_CLOSE), "r" (sem)
              : );
}

void list_procs() {
    char** names;
    int* ids;
    int len;
    asm volatile( "svc %3 \n" // make system call SYS_LIST_PROC
                  "mov %0, r0 \n" // names = r0
                  "mov %1, r1 \n" // ids = r1
                  "mov %2, r2 \n" // len = r2
                : "=r" (names), "=r" (ids), "=r" (len)
                : "I" (SYS_LIST_PROC)
                : );

    print("Active PIDS\n");
    for (int i = 0; i < len; i++) {
        printI(ids[i]);
        print("\n");
    }
}

int open(const char* path) {
    int fd;
    asm volatile( "mov r0, %2 \n" // assign r0 = path
                  "svc %1     \n" // make system call SYS_OPEN
                  "mov %0, r0 \n" // fd = r0
              : "=r" (fd)
              : "I" (SYS_OPEN), "r" (path)
              : );
    return fd;
}

void close(int fd) {
    asm volatile( "mov r0, %1 \n" // assign r0 = fd
                  "svc %0     \n" // make system call SYS_CLOSE
              :
              : "I" (SYS_CLOSE), "r" (fd)
              : );
}

void remove(const char* path) {
    asm volatile( "mov r0, %1 \n" // assign r0 = path
                  "svc %0     \n" // make system call SYS_REMOVE
              :
              : "I" (SYS_REMOVE), "r" (path)
              : );
}

void mkdir(const char* path) {
    asm volatile( "mov r0, %1 \n" // assign r0 = path
                  "svc %0     \n" // make system call SYS_MKDIR
              :
              : "I" (SYS_MKDIR), "r" (path)
              : );
}

void rmdir(const char* path) {
    asm volatile( "mov r0, %1 \n" // assign r0 = path
                  "svc %0     \n" // make system call SYS_RMDIR
              :
              : "I" (SYS_RMDIR), "r" (path)
              : );
}

void chdir(const char* path) {
    asm volatile( "mov r0, %1 \n" // assign r0 = path
                  "svc %0     \n" // make system call SYS_CHDIR
              :
              : "I" (SYS_CHDIR), "r" (path)
              : );
}

char* getcwd() {
    char* cwd;
    asm volatile( "svc %1     \n" // make system call SYS_GETCWD
                  "mov %0, r0 \n"
              : "=r" (cwd)
              : "I" (SYS_GETCWD)
              : );
    return cwd;
}

void listdir(const char* path) {
    asm volatile( "mov r0, %1 \n" // assign r0 = path
                  "svc %0     \n" // make system call SYS_LISTDIR
              :
              : "I" (SYS_LISTDIR), "r" (path)
              : );
}

void* load(int fd) {
    void* ptr;
    asm volatile( "mov r0, %2 \n" // assign r0 = fd
                  "svc %1     \n" // make system call SYS_LOAD
                  "mov %0, r0 \n"
              : "=r" (ptr)
              : "I" (SYS_LOAD), "r" (fd)
              : );
    return ptr;
}

void print(char* str) {
    write(STDOUT_FILENO, str, strlen(str));
}

void printI(int i) {
    char v[11];
    itoa(v, i);
    write(STDOUT_FILENO, v, strlen(v));
}


uint32_t val = 135;

uint32_t rand() {
    const uint32_t M = 65537;
    const uint32_t A = 75;
    const uint32_t Q = M / A;
    const uint32_t R = M % A;

    uint32_t div = val / Q;
    uint32_t rem = val % Q;

    uint32_t s = rem * A;
    uint32_t t = div * R;
    val = s - t;

    if (val < 0) {
        val += M;
    }
    return val;
}
