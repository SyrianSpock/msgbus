#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include "../../messagebus.h"
#include "../posix/port.h"
#include <pb.h>
#include <pb_encode.h>
#include <pb_decode.h>
#include "generated/simple.pb.h"

messagebus_t bus;
#define TOPIC_NAME "test"

#define PB_PUBLISH(topic, msg, type) do { \
    uint8_t buffer[type##_size]; \
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer)); \
    pb_encode(&stream, type##_fields, msg); \
    messagebus_topic_publish(topic, buffer, sizeof(buffer)); \
    } while (0);

#define PB_WAIT(topic, msg, type) do { \
    uint8_t buffer[type##_size]; \
    messagebus_topic_wait(topic, &buffer, sizeof buffer); \
    pb_istream_t stream = pb_istream_from_buffer(buffer, sizeof(buffer)); \
    pb_decode(&stream, type##_fields, msg); \
    } while(0);


static void* producer(void *p)
{
    messagebus_topic_t *topic;
    SimpleMessage msg;
    int producer_number = (int)p;

    printf("[publisher %d] waiting for topic myint\n", producer_number);

    msg.counter = 0;

    while (1) {
        topic = messagebus_find_topic_blocking(&bus, TOPIC_NAME);

        printf("[publisher %d] writing %d on topic %s\n",
                producer_number, msg.counter, topic->name);

        PB_PUBLISH(topic, &msg, SimpleMessage);

        msg.counter += 1;

        sleep(2);
    }

    return NULL;
}

static void *consumer(void *p)
{
    messagebus_topic_t *topic;
    int consumer_number = (int)p;
    SimpleMessage msg;

    printf("[consumer %d] waiting for topic myint\n", consumer_number);

    while (1) {
        topic = messagebus_find_topic_blocking(&bus, TOPIC_NAME);

        PB_WAIT(topic, &msg, SimpleMessage);

        printf("[consumer %d] read %d on topic %s\n",
               consumer_number, msg.counter, topic->name);
    }

    return NULL;
}

static void create_consumers_producers(void)
{
    /* Creates a few consumer threads. */
    pthread_t producer_thd, consumer_thd;
    pthread_create(&consumer_thd, NULL, consumer, (void *)1);
    pthread_create(&consumer_thd, NULL, consumer, (void *)2);
    pthread_create(&consumer_thd, NULL, consumer, (void *)3);

    /* Creates the producer threads, slightly offset */
    pthread_create(&producer_thd, NULL, producer, (void *)1);
    sleep(3);
    pthread_create(&producer_thd, NULL, producer, (void *)2);
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
    uint8_t topic_buffer[128];

    condvar_wrapper_t wrapper = {PTHREAD_MUTEX_INITIALIZER,
                                 PTHREAD_COND_INITIALIZER};
    messagebus_topic_init(&topic, &wrapper, &wrapper,
                          &topic_buffer,
                          sizeof topic_buffer);

    messagebus_advertise_topic(&bus, &topic, TOPIC_NAME);

    create_consumers_producers();

    while(1) {
    }
}
