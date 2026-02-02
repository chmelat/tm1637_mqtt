#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mqtt_temp.h"

#define RECONNECT_DELAY_INIT  1
#define RECONNECT_DELAY_MAX   60

static struct mosquitto *mosq = NULL;
static volatile int16_t last_temp = TEMP_NO_DATA;
static volatile int connected = 0;
static char *topic_copy = NULL;

static time_t last_reconnect_attempt = 0;
static int reconnect_delay = RECONNECT_DELAY_INIT;
static time_t last_message_time = 0;
static int watchdog_timeout = 0;

static void on_message(struct mosquitto *m, void *userdata,
                       const struct mosquitto_message *msg)
{
    (void)m;
    (void)userdata;

    if (msg->payloadlen > 0 && msg->payloadlen < 32) {
        char buf[32];
        memcpy(buf, msg->payload, msg->payloadlen);
        buf[msg->payloadlen] = '\0';

        float temp;
        if (sscanf(buf, "%f", &temp) == 1) {
            if (temp >= -99.9f && temp <= 999.9f) {
                last_temp = (int16_t)(temp * 10);  // "-2.3" → -23
                last_message_time = time(NULL);
            } else {
                last_temp = TEMP_ERROR;  // overflow protection
            }
        } else {
            last_temp = TEMP_ERROR;
        }
    }
}

static void on_connect(struct mosquitto *m, void *userdata, int rc)
{
    (void)userdata;

    if (rc == 0) {
        connected = 1;
        reconnect_delay = RECONNECT_DELAY_INIT;
        fprintf(stderr, "MQTT: Connected to broker\n");
        mosquitto_subscribe(m, NULL, topic_copy, 0);
    } else {
        fprintf(stderr, "MQTT connect failed: %s\n",
                mosquitto_connack_string(rc));
    }
}

static void on_disconnect(struct mosquitto *m, void *userdata, int rc)
{
    (void)m;
    (void)userdata;
    (void)rc;

    connected = 0;
    fprintf(stderr, "MQTT: Disconnected from broker\n");
}

int mqtt_init(const char *broker, int port, const char *topic, int wdt_timeout)
{
    watchdog_timeout = wdt_timeout;

    mosquitto_lib_init();

    mosq = mosquitto_new(NULL, true, NULL);
    if (!mosq) {
        fprintf(stderr, "Error: mosquitto_new failed\n");
        return -1;
    }

    topic_copy = strdup(topic);
    if (!topic_copy) {
        fprintf(stderr, "Error: strdup failed\n");
        mosquitto_destroy(mosq);
        mosq = NULL;
        return -1;
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_disconnect_callback_set(mosq, on_disconnect);
    mosquitto_message_callback_set(mosq, on_message);

    if (mosquitto_connect(mosq, broker, port, 60) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error: Unable to connect to MQTT broker %s:%d\n",
                broker, port);
        mosquitto_destroy(mosq);
        mosq = NULL;
        free(topic_copy);
        topic_copy = NULL;
        return -1;
    }

    return 0;
}

int mqtt_loop(int timeout_ms)
{
    int rc = mosquitto_loop(mosq, timeout_ms, 1);
    if (rc != MOSQ_ERR_SUCCESS) {
        time_t now = time(NULL);

        if (now - last_reconnect_attempt >= reconnect_delay) {
            fprintf(stderr, "MQTT: Reconnecting (backoff %ds)...\n", reconnect_delay);
            last_reconnect_attempt = now;

            if (mosquitto_reconnect(mosq) != MOSQ_ERR_SUCCESS) {
                reconnect_delay *= 2;
                if (reconnect_delay > RECONNECT_DELAY_MAX) {
                    reconnect_delay = RECONNECT_DELAY_MAX;
                }
            }
        }
        return -1;
    }
    return 0;
}

int16_t mqtt_get_temp(void)
{
    if (last_message_time == 0) {
        return TEMP_NO_DATA;
    }

    if (watchdog_timeout > 0) {
        time_t now = time(NULL);
        if (now - last_message_time > watchdog_timeout) {
            return TEMP_STALE;
        }
    }

    return last_temp;
}

void mqtt_cleanup(void)
{
    if (mosq) {
        mosquitto_disconnect(mosq);
        mosquitto_destroy(mosq);
        mosq = NULL;
    }
    mosquitto_lib_cleanup();
    if (topic_copy) {
        free(topic_copy);
        topic_copy = NULL;
    }
}
