#include <stdio.h>
#include <sys/uio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>

ssize_t process_vm_readv(pid_t pid,
                         const struct iovec *local_iov,
                         unsigned long liovcnt,
                         const struct iovec *remote_iov,
                         unsigned long riovcnt,
                         unsigned long flags);

ssize_t process_vm_writev(pid_t pid,
                          const struct iovec *local_iov,
                          unsigned long liovcnt,
                          const struct iovec *remote_iov,
                          unsigned long riovcnt,
                          unsigned long flags);


const char * gParseErrorDetails = NULL;
const char * gHelp = "usage: mem pid -r[ead]/-w[rite] address data";

enum ActionType {
    MEM_READ,
    MEM_WRITE,
};

typedef struct _InputArgs {
    enum ActionType actionType;
    int targetPid;
    int targetAddress;
    char* dataToWrite;
} InputArgs, * pInputArgs;

void setError(const char * e) {gParseErrorDetails = e;}

void printHelp() {printf("%s", gHelp);};

pInputArgs parseArgs(int argc, const char ** argv) {
    if (argc < 4) {
        setError("Wrong number of arguments!");
        return NULL;
    }
    pInputArgs inputArgs = malloc(sizeof(InputArgs));
    inputArgs->targetPid = (int)strtol(argv[1], NULL, 10);
    if (!strcmp("-r", argv[2])) {
        if (argc != 4) {
            setError("Wrong number of arguments for read mode!");
            free(inputArgs);
            return NULL;
        }
        inputArgs->actionType = MEM_READ;
        inputArgs->targetAddress = (int)strtol(argv[3], NULL, 16);
        return inputArgs;
    }
    else if (!strcmp("-w", argv[2])) {
        inputArgs->actionType = MEM_WRITE;
        inputArgs->targetAddress = (int)strtol(argv[3], NULL, 16);
        inputArgs->dataToWrite = (char *)argv[4];
        return inputArgs;
    }
    else {
        setError("Wrong action type specified (nor read nor write)!");
        free(inputArgs);
        return NULL;
    }
}

void * read(int pid, int addr) {
    int len = 4;
    struct iovec local, remote;
    char * localBuf = malloc(len + 1);
    localBuf[len] = '\0';
    local.iov_base = localBuf;
    local.iov_len = len;
    remote.iov_base = (void *)addr;
    remote.iov_len = len;
    ssize_t readRes = process_vm_readv(pid, &local, 1, &remote, 1, 0);
    if (readRes == -1) {
        printf("Error reading data! %s\n", strerror(errno));
    }
    return localBuf;
}

int write(int pid, int addr, void * data) {
    int len = 4;
    struct iovec local, remote;
    char * localBuf = (char *)data;
    local.iov_len = len;
    local.iov_base = localBuf;
    remote.iov_len = len;
    remote.iov_base = (void *)addr;
    ssize_t writeRes = process_vm_writev(pid, &local, 1, &remote, 1, 0);
    if (writeRes == -1) {
        printf("Error writing data! %s\n", strerror(errno));
        return 0;
    }
    else {
        return 1;
    }
}

int main(int argc, const char ** argv) {
    if (argc == 1) {
        printHelp();
        return 0;
    }
    pInputArgs inputArgs = parseArgs(argc, argv);
    if (!inputArgs) {
        printf("Could not parse arguments: %s", gParseErrorDetails);
        return 1;
    }
    if (inputArgs->actionType == MEM_READ) {
        char * readData = (char *)read(inputArgs->targetPid, inputArgs->targetAddress);
        printf("%s", readData);
    } else if (inputArgs->actionType == MEM_WRITE) {
        return write(inputArgs->targetPid, inputArgs->targetAddress, inputArgs->dataToWrite);
    }
}
