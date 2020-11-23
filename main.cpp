/*
 * Mbed OS Google Cloud example
 * Copyright (c) 2018-2020, Google LLC.
 * Copyright (c) 2019-2020, Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mbed.h"
#include "mbed_trace.h"
#include "mbedtls/debug.h"
#include "iotc_error.h"
#include "iotc_jwt.h"
#include "iotc.h"
#include "NTPClient.h"
#include "rtos/ThisThread.h"
#include "google_cloud_credentials.h"

#define MQTT_CONNECTION_TIMEOUT_SEC 10
#define MQTT_KEEPALIVE_TIMEOUT_SEC 20
#define JWT_EXPIRATION_PERIOD_SEC 3600

#define MBED_APP_IOTC_DEVICE_PATH  "projects/" MBED_CONF_APP_GOOGLE_CLOUD_PROJECT_ID \
                                           "/locations/" MBED_CONF_APP_GOOGLE_CLOUD_REGION "/registries/" \
                                           MBED_CONF_APP_GOOGLE_CLOUD_REGISTRY  "/devices/" \
                                           MBED_CONF_APP_GOOGLE_CLOUD_DEVICE_ID

#define MBED_APP_MQTT_TOPIC  "/devices/" MBED_CONF_APP_GOOGLE_CLOUD_DEVICE_ID "/events/"\
                                    MBED_CONF_APP_GOOGLE_CLOUD_MQTT_TOPIC


#define MBED_APP_MQTT_SUB_TOPIC  "/devices/" MBED_CONF_APP_GOOGLE_CLOUD_DEVICE_ID \
                                    "/commands/#"


/* Application variables. */
iotc_crypto_key_data_t iotc_connect_private_key_data;
static iotc_context_handle_t iotc_context = IOTC_INVALID_CONTEXT_HANDLE;
static bool message_received = false;

static iotc_timed_task_handle_t delayed_publish_task =
    IOTC_INVALID_TIMED_TASK_HANDLE;


static void on_message_sent(iotc_context_handle_t in_context_handle,
                            void *data, iotc_state_t state)
{
    if (state == IOTC_STATE_OK) {
        printf("Message sent successfully\n");
    } else {
        printf("Failed to send message, error: %s",
               iotc_get_state_string(state));
    }
}

void publish_function(iotc_context_handle_t context_handle,
                      iotc_timed_task_handle_t timed_task, void *user_data)
{
    static int max_try_receive = 10;
    iotc_state_t state = IOTC_STATE_OK;

    if (message_received) {
        if (IOTC_INVALID_TIMED_TASK_HANDLE != timed_task) {
            iotc_cancel_timed_task(timed_task);
        }
        iotc_events_stop();
    } else if (max_try_receive > 0) {
        char message[80] = {0};
        sprintf(message, "%d messages left to send, or until we receive a reply", max_try_receive);
        printf("publishing msg \"%s\"\n", message);

        state = iotc_publish(context_handle, MBED_APP_MQTT_TOPIC, message,
                             IOTC_MQTT_QOS_AT_MOST_ONCE,
                             /*callback=*/&on_message_sent, /*user_data=*/NULL);
        if (IOTC_STATE_OK != state) {
            printf("iotc_publish returned with error: %ul : %s\n", state,
                   iotc_get_state_string(state));
        }
        max_try_receive--;
    }
}

void on_message_received(iotc_context_handle_t context_handle,
                         iotc_sub_call_type_t call_type,
                         const iotc_sub_call_params_t *const params,
                         iotc_state_t in_out_state, void *user_data)
{
    if (call_type == IOTC_SUB_CALL_MESSAGE) {
        printf("Message body: %s\n", params->message.temporary_payload_data);
        message_received = true;
    }
}

void on_connection_state_changed(iotc_context_handle_t in_context_handle,
                                 void *data, iotc_state_t state)
{
    iotc_connection_data_t *conn_data = (iotc_connection_data_t *)data;

    switch (conn_data->connection_state) {
        /* IOTC_CONNECTION_STATE_OPENED means that the connection has been
           established and the IoTC Client is ready to send/recv messages */
        case IOTC_CONNECTION_STATE_OPENED:
            printf("connected to %s:%d\n", conn_data->host, conn_data->port);
            iotc_subscribe(in_context_handle,
                           MBED_APP_MQTT_SUB_TOPIC,
                           IOTC_MQTT_QOS_AT_LEAST_ONCE,
                           &on_message_received, NULL);

            /* Publish immediately upon connect. 'publish_function' is defined
               in this example file and invokes the IoTC API to publish a
               message. */
            publish_function(in_context_handle, IOTC_INVALID_TIMED_TASK_HANDLE,
                             /*user_data=*/NULL);

            /* Create a timed task to publish every 3 seconds. */
            delayed_publish_task =
                iotc_schedule_timed_task(in_context_handle, publish_function, 3, 15,
                                         /*user_data=*/NULL);
            break;

        /* IOTC_CONNECTION_STATE_OPEN_FAILED is set when there was a problem
           when establishing a connection to the server. The reason for the error
           is contained in the 'state' variable. Here we log the error state and
           exit out of the application. */
        case IOTC_CONNECTION_STATE_OPEN_FAILED:
            printf("ERROR!\tConnection has failed reason %d : %s\n\n", state,
                   iotc_get_state_string(state));

            /* exit it out of the application by stopping the event loop. */
            iotc_events_stop();
            break;

        /* IOTC_CONNECTION_STATE_CLOSED is set when the IoTC Client has been
            disconnected. The disconnection may have been caused by some external
            issue, or user may have requested a disconnection. In order to
           distinguish between those two situation it is advised to check the state
           variable value. If the state == IOTC_STATE_OK then the application has
           requested a disconnection via 'iotc_shutdown_connection'. If the state !=
           IOTC_STATE_OK then the connection has been closed from one side.
          */
        case IOTC_CONNECTION_STATE_CLOSED: {
            /* When the connection is closed it's better to cancel some of previously
               registered activities. Using cancel function on handler will remove the
               handler from the timed queue which prevents the registered handle to be
               called when there is no connection. */
            if (IOTC_INVALID_TIMED_TASK_HANDLE != delayed_publish_task) {
                iotc_cancel_timed_task(delayed_publish_task);
                delayed_publish_task = IOTC_INVALID_TIMED_TASK_HANDLE;
            }

            if (state == IOTC_STATE_OK) {
                /* The connection has been closed intentionally. Therefore, stop
                 * the event processing loop as there's nothing left to do
                 * in this example. */
                iotc_events_stop();
            } else {
                printf("connection closed - reason %d : %s!\n", state,
                       iotc_get_state_string(state));
                /* The disconnection was unforeseen.  Try to reconnect to the server
                   with the previously set username and client_id, but regenerate
                   the client authentication JWT password in case the disconnection
                   was due to an expired JWT. */
                char jwt[IOTC_JWT_SIZE] = {0};
                size_t bytes_written = 0;
                state = iotc_create_iotcore_jwt(MBED_CONF_APP_GOOGLE_CLOUD_DEVICE_ID,
                                                JWT_EXPIRATION_PERIOD_SEC,
                                                &iotc_connect_private_key_data, jwt,
                                                IOTC_JWT_SIZE, &bytes_written);
                if (IOTC_STATE_OK != state) {
                    printf(
                        "iotc_create_iotcore_jwt returned with error"
                        " when attempting to reconnect: %ul\n",
                        state);
                } else {
                    iotc_connect(in_context_handle, conn_data->username, jwt,
                                 conn_data->client_id, conn_data->connection_timeout,
                                 conn_data->keepalive_timeout,
                                 &on_connection_state_changed);
                }
            }
        }
        break;
        default:
            printf("wrong value\n");
            break;
    }
}

int ntp_client(NetworkInterface *net)
{
    NTPClient ntp(net);
    ntp.set_server("time.google.com", 123);
    time_t timestamp = ntp.get_timestamp();
    if (timestamp < 0) {
        printf("Failed to get the current time, error: %ld\n", (long int)timestamp);
        return -1;
    }
    printf("Time: %s", ctime(&timestamp));
    set_time(timestamp);
    return 0;
}

int main(int argc, char *argv[])
{
    iotc_state_t state = IOTC_STATE_OK;

    auto net = NetworkInterface::get_default_instance();
    if (!net) {
        printf("Error! No network inteface found.\n");
        return -1;
    }
    int ret = net->connect();
    if (ret != 0) {
        printf("Connection error: %d", ret);
        return -1;
    }
    if (ntp_client(net) < 0) {
        return -1;
    }

    /* Format the key type descriptors so the client understands
       which type of key is being reprenseted. In this case, a PEM encoded
       byte array of a ES256 key. */
    iotc_connect_private_key_data.crypto_key_signature_algorithm =
        IOTC_CRYPTO_KEY_SIGNATURE_ALGORITHM_ES256;
    iotc_connect_private_key_data.crypto_key_union_type =
        IOTC_CRYPTO_KEY_UNION_TYPE_PEM;
    iotc_connect_private_key_data.crypto_key_union.key_pem.key = google_cloud::credentials::clientKey;

    /* Initialize iotc library and create a context to use to connect to the
     * GCP IoT Core Service. */
    state = iotc_initialize();

    if (IOTC_STATE_OK != state) {
        printf(" iotc failed to initialize, error: %d\n", state);
        return -1;
    }

    /*  Create a connection context. A context represents a Connection
        on a single socket, and can be used to publish and subscribe
        to numerous topics. */
    iotc_context = iotc_create_context();
    if (IOTC_INVALID_CONTEXT_HANDLE >= iotc_context) {
        printf(" iotc failed to create context, error: %d\n", (int)-iotc_context);
        return -1;
    }

    /* Generate the client authentication JWT, which will serve as the MQTT
     * password. */
    char jwt[IOTC_JWT_SIZE] = {0};
    size_t bytes_written = 0;
    state = iotc_create_iotcore_jwt(
                MBED_CONF_APP_GOOGLE_CLOUD_PROJECT_ID,
                JWT_EXPIRATION_PERIOD_SEC, &iotc_connect_private_key_data, jwt,
                IOTC_JWT_SIZE, &bytes_written);

    if (IOTC_STATE_OK != state) {
        printf("iotc_create_iotcore_jwt returned with error: %ul : %s\n", state,
               iotc_get_state_string(state));
        return -1;
    }

    /*  Queue a connection request to be completed asynchronously.
    The 'on_connection_state_changed' parameter is the name of the
    callback function after the connection request completes, and its
    implementation should handle both successful connections and
    unsuccessful connections as well as disconnections. */
    state = iotc_connect(iotc_context, /*username=*/NULL, /*password=*/jwt,
                         /*client_id=*/MBED_APP_IOTC_DEVICE_PATH, MQTT_CONNECTION_TIMEOUT_SEC,
                         MQTT_KEEPALIVE_TIMEOUT_SEC, &on_connection_state_changed);

    if (IOTC_STATE_OK != state) {
        printf("iotc_connect returned with error: %ul : %s\n", state,
               iotc_get_state_string(state));
        return -1;
    }

    /* Regularly call the function iotc_events_process_blocking() to process
       connection requests, and for the client to regularly check the sockets for
       incoming data in endless loop.
    */
    iotc_events_process_blocking();

    /*  Cleanup the default context, releasing its memory */
    iotc_delete_context(iotc_context);

    /* Cleanup internal allocations that were created by iotc_initialize. */
    iotc_shutdown();

    printf("Done\n");

    return 0;
}
