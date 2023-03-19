//
// Created by matt on 03/12/22.
//

#ifndef SMART_CPU_PINNING_H
#define SMART_CPU_PINNING_H

#include "utils.h"
#include "commands.h"
#include <sched.h>

#if defined __linux__

/*
 * Pins the specified cpu and prints warnings.
 * Returns 1 if successful, 0 otherwise.
 */
int pin_cpu(int cpu_to_pin, int num_processors)
{
    if (cpu_to_pin >= 0 && cpu_to_pin < num_processors)
    {
        int pid = getpid();
        cpu_set_t cpus;
        CPU_ZERO(&cpus);
        CPU_SET(cpu_to_pin, &cpus);

        if (sched_setaffinity(pid, sizeof(cpu_set_t), &cpus) == -1)
        {
            warn("Could not pin the benchmark to a core: variation in benchmarking may be higher.\n");
        }
        else
        {
            info("Pinned process %d to core %d of 0 - %d processors.\n", pid, cpu_to_pin, num_processors - 1);
            return 1;
        }
    }
    else
    {
        warn("\tCould not pin cpu %d to available cores 0 - %d: variation in benchmarking may be higher.\n", cpu_to_pin, num_processors - 1);
    }

    return 0;
}

/*
 * Sets the scheduler affinity to pin this process to one CPU core.
 * This can avoid benchmarking variation caused by processes moving from one core to another, causing cache misses.
 */
int pin_to_one_CPU_core(enum cpu_pin_type cpu_pinning, int cpu_to_pin, const char *not_enabled_message)
{
    int num_processors = (int)sysconf(_SC_NPROCESSORS_ONLN);
    switch (cpu_pinning)
    {
        case PINNING_OFF:
        {
            info("CPU pinning not enabled: %s", not_enabled_message);
            break;
        }
        case PIN_LAST_CPU:
        {
            if (pin_cpu(num_processors - 1, num_processors))
            {
                return num_processors - 1;
            }
            break;
        }
        case PIN_SPECIFIED_CPU:
        {
            if (pin_cpu(cpu_to_pin, num_processors))
            {
                return cpu_to_pin;
            }
            break;
        }
        default:
        {
            error_and_exit("Unknown cpu pinning option provided: %d", cpu_pinning);
        }
    }

    return -1;
}

#else

/*
 * Empty implementation for non linux systems, returns -1 to indicate no cpu was pinned.
 */
int pin_to_one_CPU_core(enum cpu_pin_type cpu_pinning, int cpu_to_pin, const char *not_enabled_message)
{
    warn("CPU pinning only enabled on linux: %s", not_enabled_message);
    return -1;
}

#endif

#endif //SMART_CPU_PINNING_H
