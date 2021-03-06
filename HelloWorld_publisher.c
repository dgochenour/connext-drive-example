
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rti_me_c.h"
#include "wh_sm/wh_sm_history.h"
#include "rh_sm/rh_sm_history.h"

#include "HelloWorld.h"
#include "HelloWorldSupport.h"
#include "HelloWorldPlugin.h"

#include "HelloWorldApplication.h"

#include "disc_dpse/disc_dpse_dpsediscovery.h"

void
HelloWorldPublisher_on_publication_matched(
    void *listener_data,
    DDS_DataWriter *writer,
    const struct DDS_PublicationMatchedStatus *status)
{
    if (status->current_count_change > 0)
    {
        printf("Matched a subscriber\n");
    }
    else if (status->current_count_change < 0)
    {
        printf("Unmatched a subscriber\n");
    }
}

int
publisher_main_w_args(
    DDS_Long domain_id,
    char *udp_intf,
    char *peer,
    DDS_Long sleep_time,
    DDS_Long count)
{
    DDS_Publisher *publisher;
    DDS_DataWriter *datawriter;
    HelloWorldDataWriter *hw_datawriter;
    struct DDS_DataWriterQos dw_qos = DDS_DataWriterQos_INITIALIZER;
    DDS_ReturnCode_t retcode;
    HelloWorld *sample = NULL;
    struct Application *application = NULL;
    DDS_Long i;
    struct DDS_DataWriterListener dw_listener = DDS_DataWriterListener_INITIALIZER;
    int ret_value = -1;
    struct DDS_SubscriptionBuiltinTopicData rem_subscription_data =
    DDS_SubscriptionBuiltinTopicData_INITIALIZER;

    sample = HelloWorldTypeSupport_create_data();
    if (sample == NULL)
    {
        printf("failed HelloWorldTypeSupport_create_data\n");
        return -1;
    }

    application = Application_create(
        "publisher",
        "subscriber",
        domain_id,
        udp_intf,
        peer,
        sleep_time,
        count);

    if (application == NULL)
    {
        printf("failed Application create\n");
        goto done;
    }

    publisher = DDS_DomainParticipant_create_publisher(
        application->participant,
        &DDS_PUBLISHER_QOS_DEFAULT,
        NULL,
        DDS_STATUS_MASK_NONE);
    if (publisher == NULL)
    {
        printf("publisher == NULL\n");
        goto done;
    }

    #ifdef USE_RELIABLE_QOS
    dw_qos.reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;
    #else
    dw_qos.reliability.kind = DDS_BEST_EFFORT_RELIABILITY_QOS;
    #endif
    dw_qos.resource_limits.max_samples_per_instance = 32;
    dw_qos.resource_limits.max_instances = 2;
    dw_qos.resource_limits.max_samples = dw_qos.resource_limits.max_instances *
    dw_qos.resource_limits.max_samples_per_instance;
    dw_qos.history.depth = 32;
    dw_qos.protocol.rtps_reliable_writer.heartbeat_period.sec = 0;
    dw_qos.protocol.rtps_reliable_writer.heartbeat_period.nanosec = 250000000;

    dw_listener.on_publication_matched = HelloWorldPublisher_on_publication_matched;

    dw_qos.protocol.rtps_object_id = 100;

    datawriter = DDS_Publisher_create_datawriter(
        publisher,
        application->topic,
        &dw_qos,
        &dw_listener,
        DDS_PUBLICATION_MATCHED_STATUS);

    if (datawriter == NULL)
    {
        printf("datawriter == NULL\n");
        goto done;
    }

    rem_subscription_data.key.value[DDS_BUILTIN_TOPIC_KEY_OBJECT_ID] = 200;
    rem_subscription_data.topic_name = DDS_String_dup(application->topic_name);
    rem_subscription_data.type_name = DDS_String_dup(application->type_name);

    #ifdef USE_RELIABLE_QOS
    rem_subscription_data.reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;
    #else
    rem_subscription_data.reliability.kind  = DDS_BEST_EFFORT_RELIABILITY_QOS;
    #endif

    if (DDS_RETCODE_OK != DPSE_RemoteSubscription_assert(
        application->participant,
        "subscriber",
        &rem_subscription_data,
        HelloWorld_get_key_kind(HelloWorldTypePlugin_get(), NULL)))
    {
        printf("failed to assert remote subscription\n");
        goto done;
    }

#ifdef USE_ADMINCONSOLE
    // assert the remote reader of Admin Console... this is the DataReader that
    // is created when the user subscribes to the "Example HelloWorld" Topic 
    // from Admin Console
    rem_subscription_data.key.value[DDS_BUILTIN_TOPIC_KEY_OBJECT_ID] = k_OBJ_ID_ADMINCONSOLE_DR01;
    rem_subscription_data.topic_name = DDS_String_dup(application->topic_name);
    rem_subscription_data.type_name = DDS_String_dup(application->type_name);

    #ifdef USE_RELIABLE_QOS
    rem_subscription_data.reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;
    #else
    rem_subscription_data.reliability.kind  = DDS_BEST_EFFORT_RELIABILITY_QOS;
    #endif
    
    if (DDS_RETCODE_OK != DPSE_RemoteSubscription_assert(
        application->participant,
        k_PARTICIPANT_ADMINCONSOLE_NAME, // Admin Console's participant name
        &rem_subscription_data,
        HelloWorld_get_key_kind(HelloWorldTypePlugin_get(), NULL)))
    {
        printf("failed to assert remote subscription (Admin Console)\n");
        goto done;
    }
#endif

    hw_datawriter = HelloWorldDataWriter_narrow(datawriter);

    #ifdef RTI_CERT
    #ifdef RTI_VXWORKS
    /** End initialization, disable further dynamic memory allocation ***/
    memAllocDisable();
    #endif
    #endif

    for (i = 0; (application->count <= 0) || (i < application->count); ++i)
    {

        /* TODO set sample attributes here */

        retcode = HelloWorldDataWriter_write(
            hw_datawriter,
            sample,
            &DDS_HANDLE_NIL);
        if (retcode != DDS_RETCODE_OK)
        {
            printf("Failed to write sample\n");
        } 
        else
        {
            printf("Written sample %d\n",(i+1));
        } 

        /* OSAPI_Thread_sleep() is not included in Connext DDS Cert because it might not
        * be thread safe. For this reason a call to OSAPI_Thread_sleep() cannot be done
        * in this example. Instead we use the code below.
        */
        #if OSAPI_INCLUDE_POSIX
        {
            struct timespec remain, next;
            int rval;
            RTI_UINT32 is;

            is = application->sleep_time / 1000;

            next.tv_sec = is;
            next.tv_nsec = (application->sleep_time - (is * 1000U)) * 1000000U;

            do
            {
                rval = nanosleep(&next, &remain);
                if ((rval == -1) && (errno == EINTR))
                {
                    next = remain;
                }
            } while ((rval == -1) && (errno == EINTR));
        }
        #elif RTI_VXWORKS
        taskDelay(((sysClkRateGet() * application->sleep_time) + 500) / 1000);
        #else
        #error "Implementation of sleep() needed"
        #endif
    }

    ret_value = 0;

    done:

    #ifndef RTI_CERT
    if (application != NULL)
    {
        Application_delete(application);
    }

    if (sample != NULL)
    {
        HelloWorldTypeSupport_delete_data(sample);
    }

    #endif
    #ifndef RTI_CERT
    retcode = DDS_DataWriterQos_finalize(&dw_qos);
    if (retcode != DDS_RETCODE_OK)
    {
        printf("Cannot finalize DataWriterQos\n");
        return -1;
    }

    if (DDS_SubscriptionBuiltinTopicData_finalize(&rem_subscription_data) !=
    DDS_BOOLEAN_TRUE)
    {
        printf("Cannot finalize DDS_SubscriptionBuiltinTopicData\n");
        return -1;
    }

    #endif

    return ret_value;
}

#if !(defined(RTI_VXWORKS) && !defined(__RTP__))
int
main(int argc, char **argv)
{
    DDS_Long i = 0;
    DDS_Long domain_id = 0;
    char *peer = NULL;
    char *udp_intf = NULL;
    DDS_Long sleep_time = 1000;
    DDS_Long count = 0;

    for (i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "-domain"))
        {
            ++i;
            if (i == argc)
            {
                printf("-domain <domain_id>\n");
                return -1;
            }
            domain_id = strtol(argv[i], NULL, 0);
        }
        else if (!strcmp(argv[i], "-udp_intf"))
        {
            ++i;
            if (i == argc)
            {
                printf("-udp_intf <interface>\n");
                return -1;
            }
            udp_intf = argv[i];
        }
        else if (!strcmp(argv[i], "-peer"))
        {
            ++i;
            if (i == argc)
            {
                printf("-peer <address>\n");
                return -1;
            }
            peer = argv[i];
        }
        else if (!strcmp(argv[i], "-sleep"))
        {
            ++i;
            if (i == argc)
            {
                printf("-sleep_time <sleep_time>\n");
                return -1;
            }
            sleep_time = strtol(argv[i], NULL, 0);
        }
        else if (!strcmp(argv[i], "-count"))
        {
            ++i;
            if (i == argc)
            {
                printf("-count <count>\n");
                return -1;
            }
            count = strtol(argv[i], NULL, 0);
        }
        else if (!strcmp(argv[i], "-h"))
        {
            Application_help(argv[0]);
            return 0;
        }
        else
        {
            printf("unknown option: %s\n", argv[i]);
            return -1;
        }
    }

    return publisher_main_w_args(domain_id, udp_intf, peer, sleep_time, count);
}
#elif defined(RTI_VXWORKS)
int
publisher_main(void)
{
    /* Explicitly configure args below */
    DDS_Long domain_id = 0;
    char *peer = "127.0.0.1";
    char *udp_intf = NULL;
    DDS_Long sleep_time = 1000;
    DDS_Long count = 0;

    return publisher_main_w_args(domain_id, udp_intf, peer, sleep_time, count);
}
#endif
