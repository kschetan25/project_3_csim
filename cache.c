#include "csim.h"
#include <stdio.h>

#define MAX_SIZE 1000
#define MAX_DATABASE 1000L
#define BCAST_INTERVAL 20.0
#define NUM_CLIENTS 10L
#define NUM_SERVERS 1L
#define TRANS_TIME 0.8
#define TOTAL_NODES 101L
#define CACHE_SIZE 100L

#define REQUEST 1L
#define REPLY 2L

typedef struct msg *msg_t;
struct msg
{

} msg_t msg_queue;

struct cdata
{
    int valid_bit;        //cache valid bit
    int data_item_id;     //cache data item
    double last_updated;  //cache last updated time of data item
    double last_accessed; //cache last accessed time of data item
};

struct s_node
{
    double data_items[MAX_SIZE]; //server database
    int data_id;                 //req data items
    int data_tstamp;             //timestamp
    double bcast_data[MAX_SIZE]; //Lbcast
} server;

struct c_node
{
    double data_items[MAX_SIZE];
    struct cdata cache[MAX_SIZE];
    double last_interval_timestamp;
} client;

struct nde
{
    MBOX input;
};

double bcast[MAX_SIZE];
struct nde node[TOTAL_NODES];
struct cdata cache[10];
double simt = 0.0;
double mean_update_time = 0.0;

void getSimTime();
void serverProcess();
void init();
void broadcastIR();
void buildIR();
void updateDataItems();
msg_t new_msg();

void sim()
{
    //create("sim");
    getSimTime();
    printf("\n ----------------------- %lf", simt);
    init();
    //hold(simt);
}

void init()
{
    long i;
    char str[24];
    max_servers(NUM_SERVERS * NUM_SERVERS + NUM_SERVERS);
    max_mailboxes(TOTAL_NODES);
    max_events(2 * TOTAL_NODES * TOTAL_NODES);
    max_mailboxes(TOTAL_NODES);
    msg_queue = NIL;

    for (i = 0; i < TOTAL_NODES; i++)
    {
        sprintf(str, "input.%d", i);
        node[i].input = mailbox(str);
    }

    serverProcess();
    // clientProcess();
}

void getSimTime()
{
    printf("\n Enter Mean arrival update time(25 to 300): ");
    scanf("%lf", &mean_update_time);
    simt = CACHE_SIZE * mean_update_time * 10;
    printf("====== The aim time ia %lf", simt);
}

void serverProcess()
{
    //create("server proc");
    while (clock < simt)
    {
        // broadcastIR();
        updateDataItems();
        // listenClient();
    }
}
void broadcastIR()
{
    int bcast[MAX_SIZE] = 0.0;
    //create("broadcastProcess");
    while (clock < simt)
    {
        buildIR();
        //hold(20.0);
    }
}

void buildIR()
{
}

void updateDataItems()
{
    long ran;
    long uni;
    //create("updateProcess");
    //hold(50.0);
    while (clock < simt)
    {
        ran = random(0, 1000);
        if (ran < 0.33)
        {
            uni = uniform(1, 50);
            server.data_items[uni] = clock;
            server.bcast_data[uni] = uni;
        }
    }
}

void listenClient(items)
long items;
{
    
}

// msg_t new_msg(from) long from;
// {
//     msg_t m;
//     long i;
//     if (msg_queue == NIL)
//     {
//         m = (msg_t)do_malloc(sizeof(struct msg));
//     }
//     else
//     {
//         m = msg_queue;
//         msg_queue = msg_queue->link;
//     }
//     do
//     {
//         i = random(0l, NUM_NODES - 1);
//     } while (i == from);
//     m->to = i;
//     m->tag = 0;
//     m->from = from;
//     m->type = REQUEST;
//     m->start_time = clock;
//     return (m);
// }

// void return_msg(m)
//     msg_t m;
// {
//     m->link = msg_queue;
//     msg_queue = m;
// }