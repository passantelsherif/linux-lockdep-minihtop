#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>

// ══════════════════════════════════════════════════════════════════════════
//  cpu usage (Delta Method)

typedef struct
{
    unsigned long user, nice, system, idle, iowait, irq, softirq;
} CpuStats;

CpuStats read_cpu_stats(void)
{
    FILE *fp = fopen("/proc/stat", "r");
    CpuStats stats = {0};
    char line[256];

    if (fp)
    {
        fgets(line, sizeof(line), fp);
        sscanf(line, "cpu %lu %lu %lu %lu %lu %lu %lu",
               &stats.user, &stats.nice, &stats.system, &stats.idle,
               &stats.iowait, &stats.irq, &stats.softirq);
        fclose(fp);
    }
    return stats;
}

float get_cpu_usage(CpuStats prev, CpuStats curr)
{
    unsigned long prev_total = prev.user + prev.nice + prev.system +
                               prev.idle + prev.iowait + prev.irq + prev.softirq;
    unsigned long curr_total = curr.user + curr.nice + curr.system +
                               curr.idle + curr.iowait + curr.irq + curr.softirq;

    unsigned long prev_idle = prev.idle + prev.iowait;
    unsigned long curr_idle = curr.idle + curr.iowait;

    unsigned long total_diff = curr_total - prev_total;
    unsigned long idle_diff = curr_idle - prev_idle;

    if (total_diff == 0)
        return 0.0;

    float cpu = (float)(total_diff - idle_diff) / total_diff * 100.0;
    return (cpu > 100.0) ? 100.0 : (cpu < 0.0) ? 0.0
                                               : cpu;
}

// memory usage =========================================================

typedef struct
{
    unsigned long total, free;
} MemStats;

MemStats get_memory_usage(void)
{
    FILE *fp = fopen("/proc/meminfo", "r");
    MemStats mem = {0};
    char line[256], key[32];
    unsigned long value;

    if (fp)
    {
        while (fgets(line, sizeof(line), fp))
        {
            sscanf(line, "%31s %lu", key, &value);

            if (strcmp(key, "MemTotal:") == 0)
                mem.total = value;
            if (strcmp(key, "MemFree:") == 0)
                mem.free = value;
        }
        fclose(fp);
    }
    return mem;
}

// list running processes =====================================================

typedef struct
{
    int pid;
    char name[256];
} Process;

int get_process_name(int pid, char *name)
{
    char path[256];
    FILE *fp;

    snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    fp = fopen(path, "r");

    if (!fp)
        return 0;

    if (fgets(name, 256, fp))
    {
        name[strcspn(name, "\n")] = '\0';
    }

    fclose(fp);
    return 1;
}

int list_processes(Process *procs, int max)
{
    DIR *dir = opendir("/proc");
    struct dirent *entry;
    int count = 0;

    if (!dir)
        return 0;

    while ((entry = readdir(dir)) && count < max)
    {
        if (entry->d_type != DT_DIR)
            continue;

        int pid = atoi(entry->d_name);
        if (pid <= 0)
            continue;

        if (get_process_name(pid, procs[count].name))
        {
            procs[count].pid = pid;
            count++;
        }
    }

    closedir(dir);
    return count;
}

// System uptime ==========================================================

unsigned long get_uptime(void)
{
    FILE *fp = fopen("/proc/uptime", "r");
    double uptime = 0.0;

    if (fp)
    {
        fscanf(fp, "%lf", &uptime);
        fclose(fp);
    }

    return (unsigned long)uptime;
}

// progress bar ====================================

void draw_bar(float percentage, int width)
{
    int filled = (int)(percentage / 100.0 * width);
    if (filled > width)
        filled = width;

    printf("[");
    for (int i = 0; i < width; i++)
    {
        if (i < filled)
            printf("=");
        else
            printf("-");
    }
    printf("]");
}

// display ================================================================
void display(float cpu, MemStats mem, Process *procs, int count, unsigned long uptime)
{
    system("clear");

    unsigned long used = mem.total - mem.free;
    float mem_percent = (float)used / mem.total * 100.0;

    printf("mini-htop\n");
    printf("Press q to quit\n\n");

    // CPU with progress bar
    printf("CPU  ");
    draw_bar(cpu, 25);
    printf(" %.2f%%\n", cpu);

    // Memory with progress bar
    printf("MEM  ");
    draw_bar(mem_percent, 25);
    printf(" %.2f%% (%lu / %lu MB)\n", mem_percent, used / 1024, mem.total / 1024);

    // Process and Uptime info
    printf("PROC %d\n", count);
    printf("UPTIME %lu sec\n\n", uptime);

    // Process list header
    printf("PID    NAME\n");
    printf("─────────────────────────────────\n");

    // Process list
    for (int i = 0; i < count && i < 20; i++)
    {
        printf("%-6d %s\n", procs[i].pid, procs[i].name);
    }
}

int main(void)
{
    CpuStats prev_cpu, curr_cpu;
    MemStats mem;
    Process procs[256];
    int count;
    unsigned long uptime;

    prev_cpu = read_cpu_stats();
    sleep(1);

    // Set stdin to non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    while (1)
    {
        curr_cpu = read_cpu_stats();
        mem = get_memory_usage();
        count = list_processes(procs, 256);
        uptime = get_uptime();

        float cpu = get_cpu_usage(prev_cpu, curr_cpu);
        display(cpu, mem, procs, count, uptime);

        prev_cpu = curr_cpu;

        // check for q char to exit
        char c;
        if (read(STDIN_FILENO, &c, 1) > 0 && (c == 'q' || c == 'Q'))
        {
            break;
        }

        sleep(1);
    }

    printf("\nExiting...\n");
    return 0;
}
