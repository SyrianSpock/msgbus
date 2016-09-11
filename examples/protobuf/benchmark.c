#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include "../../messagebus.h"
#include "../../messagebus_protobufs.h"
#include "../posix/port.h"
#include "simple.pb.h"

messagebus_t bus;
#define TOPIC_NAME "test"
#define ITERATIONS 10000000

float compute_duration(clock_t start, clock_t stop)
{
    return (stop - start)/((float)CLOCKS_PER_SEC);
}

void benchmark_no_serialization(void)
{
    messagebus_topic_t *topic;
    uint8_t buffer[SimpleMessage_size];
    clock_t start, stop;
    float duration;

    memset(buffer, 0x55, sizeof(buffer));
    topic = messagebus_find_topic_blocking(&bus, TOPIC_NAME);

    printf("Starting benchmark without serialization.\n");
    printf("Publishing %d messages...\n", ITERATIONS);

    start = clock();

    for (uint64_t i = 0; i < ITERATIONS; i++) {
        messagebus_topic_publish(topic, buffer, sizeof buffer);
    }

    stop = clock();

    duration = compute_duration(start, stop);

    printf("Took %.2f seconds (%.1f messages per second)\n",
            duration, ITERATIONS / duration);
}

void benchmark_pb_serialization(void)
{
    messagebus_topic_t *topic;
    SimpleMessage msg;

    clock_t start, stop;
    float duration;

    msg.counter = 42;

    topic = messagebus_find_topic_blocking(&bus, TOPIC_NAME);

    printf("Starting benchmark with protobuf serialization.\n");
    printf("Publishing %d messages...\n", ITERATIONS);

    start = clock();

    for (uint64_t i = 0; i < ITERATIONS; i++) {
        MESSAGEBUS_PB_PUBLISH(topic, &msg, SimpleMessage);
    }

    stop = clock();

    duration = compute_duration(start, stop);

    printf("Took %.2f seconds (%.1f messages per second)\n",
            duration, ITERATIONS / duration);
}


int main(int argc, const char **argv)
{
    (void) argc;
    (void) argv;

    /* Create the message bus. */
    condvar_wrapper_t bus_sync = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
    messagebus_init(&bus, &bus_sync, &bus_sync);

    /* Creates a topic and publish it on the bus. */
    messagebus_topic_t topic;
    uint8_t topic_buffer[SimpleMessage_size];

    condvar_wrapper_t wrapper = {PTHREAD_MUTEX_INITIALIZER,
                                 PTHREAD_COND_INITIALIZER};

    messagebus_topic_init(&topic, &wrapper, &wrapper,
                          &topic_buffer,
                          sizeof topic_buffer);

    messagebus_advertise_topic(&bus, &topic, TOPIC_NAME);

    /* Runs the benchmarks. */
    benchmark_no_serialization();
    benchmark_pb_serialization();
}
