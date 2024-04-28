#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/uio.h>

static int
pidfd_open(pid_t pid, unsigned int flags)
{
  return syscall(__NR_pidfd_open, pid, flags);
}

// https://serverfault.com/questions/938431/is-there-a-way-to-force-a-single-process-to-swap-most-all-of-its-memory-to-disk

#ifndef SYS_process_madvise
#define SYS_process_madvise 440
#endif

ssize_t process_madvise(
        int fd,
        const struct iovec *vec,
        size_t vlen,
        int advice,
        int flags)
{
    return syscall(SYS_process_madvise, fd, vec, vlen, advice, flags);
}

/* Retrieve a list of all anonymous mappings that this process
 * is using */
size_t get_anonymous_mappings(
        pid_t pid,
        struct iovec **vecs,
        int *nvecs)
{
    char buf[1024] = {0};
    FILE *maps = NULL;
    struct iovec *v = NULL;
    size_t total = 0;
    int nv = 0;

    v = alloca(32768 * sizeof(struct iovec));

    /* Open the map file for pid */
    snprintf(buf, 1023, "/proc/%u/maps", pid);
    maps = fopen(buf, "re");
    if (!maps) {
        fprintf(stderr, "Error: Cannot access process maps: %m\n");
        goto fail;
    }

    while (1) {
        char buf[1024] = {0};
        char path[256] = {0};
        char perm[32] = {0};
        uint64_t off, inode;
        uint8_t *e, *b, dev_min, dev_maj;
        int rc;

        if (!fgets(buf, 1023, maps))
            break;

        memset(path, 0, 256);
        memset(perm, 0, 32);
        rc = sscanf(buf, "%p-%p %s %lx %hhx:%hhx %lu %s", 
                &b, &e, perm, &off, &dev_maj, &dev_min, &inode, path);
	printf("%p-%p %s %ld %s \n", b, e, perm, (e-b)/4096, path);
        if (rc < 6) 
            continue;

        /* Must all be 0 to be an anonymous mapping */
        if ((off || dev_min || dev_maj || inode)) 
	  continue;

        if (strcmp(path, "[heap]") != 0 && strcmp(path, "[stack]") != 0 && path[0] != 0)
	  continue;

	printf("Adding %p-%p %s %ld %s \n", b, e, perm, (e-b)/4096, path);
	
        total += (e-b);
        v[nv].iov_base = b;
        v[nv].iov_len = (size_t)(e-b);
        nv++;
    }

    if (ferror(maps)) {
        fprintf(stderr, "Error reading file: %m\n");
        goto fail;
    }

    *nvecs = nv;
    *vecs = calloc(nv, sizeof(struct iovec));
    if (!(*vecs)) {
        fprintf(stderr, "Cannot allocate vectors: %m\n");
        goto fail;
    }

    memcpy((*vecs), v, nv * sizeof(struct iovec));

    fclose(maps);
    return total;

fail:
    if (maps)
        fclose(maps);
    if (*vecs)
        free(*vecs);
    return -1;
}

int main(
        int argc,
        char **argv)
{
    int n, rc;
    int pfd = -1;
    struct iovec *vecs = NULL;
    int nvecs = -1;
    size_t mapsz;
    pid_t pid = -1;

    if (argc < 2) {
        fprintf(stderr, "Error: Give process ID of process to swap out.\n");
        exit(EXIT_FAILURE);
    }

    pid = atoi(argv[1]);
    if (pid <= 0) {
        fprintf(stderr, "Error: Pid is not a valid process ID.\n");
        exit(EXIT_FAILURE);
    }

    pfd = pidfd_open(pid, 0);
    if (pfd < 0) {
        fprintf(stderr, "Error: Cannot access process: %m\n");
        exit(EXIT_FAILURE);
    }

    mapsz = get_anonymous_mappings(pid, &vecs, &nvecs);
    if (mapsz < 0)
        exit(EXIT_FAILURE);

    printf("Swapping out %.3f MiB of process memory\n", (double)mapsz / 1048576.0);

    /* Only 1024 vectors can be processed at once */
    n=0;
    mapsz = 0;
    while (n < nvecs) {
        int l;
        if (nvecs-n > 1024)
            l = 1024;
        else
            l = nvecs-n;

        rc = process_madvise(pfd, &vecs[n], l, MADV_PAGEOUT, 0);
        if (rc < 0) {
            fprintf(stderr, "Cannot swap out process: %m\n");
            //exit(EXIT_FAILURE);
        }

        n += l;
        mapsz += rc;
    }

    printf("Swapped out %.3f MiB of process memory\n", (double)mapsz / 1048576.0);
    free(vecs);
    exit(EXIT_SUCCESS);
}
