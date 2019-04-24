#include "csim.h"
#include <stdio.h>

#define DATABASE 1000L
#define CLIENTS 10L
#define SERVERS 1L
#define NODES 11L
#define SIMTIME 100.0
#define CACHE 100L

typedef struct msg *msg_t;

struct data_tuple
{
    long data_id;
    double data_ts;
};

struct msg
{
    struct data_tuple bcastData[DATABASE];
    double timestamp;
    long data_id;
    msg_t link;
};

struct s_node
{
    double data_items[DATABASE]; //server database
    long bcast_data[DATABASE];   //Lbcast
} server;

struct cdata
{
    long valid_bit;
    long data_item_id;
    double last_updated;
    double last_accessed;
};

struct c_node
{
    struct cdata cache[CACHE];
    double last_interval_timestamp;
    long req_data[DATABASE];
};

struct c_node clientNodes[CLIENTS];
msg_t msg_queue;

struct nde
{
    MBOX mailbox;
};

struct nde node[NODES];

void init();
void serverProc();
void clientProc();
void updateDB();
void rcv_cl_qry();
void bcastIR();
void send_qry();
void rcv_sv_IR();
msg_t build_msg();

void send_msg();
void sending_message();
msg_t new_msg();
long msg_cnt(MBOX m);

void sim()
{
    //    create("sim");
    init();
    //    hold(SIMTIME);
}

void init()
{
    long i, j;
    char str[24];
    max_servers(SERVERS * SERVERS + SERVERS);
    max_mailboxes(NODES * NODES + NODES);
    max_events(2 * NODES * NODES);
    msg_queue = NIL;

    for (i = 0; i < NODES; i++)
    {
        sprintf(str, "mailbox.%d", i);
        node[i].mailbox = mailbox(str);
    }

    serverProc();
    clientProc();
}

void serverProc()
{
    updateDB();
    rcv_cl_qry();
    bcastIR();
}

void updateDB()
{
    double uProb;
    long rand;
    //create("updateProcess");
    //hold(50.0);
    while (clock < SIMTIME)
    {
        uProb = uniform(0.0, 1.0);
        rand = (uProb < 0.33) ? random(1, 50) : random(51, 1000);
        {
            server.data_items[rand] = clock;
            server.bcast_data[rand] = rand;
        }
        hold(2.0);
    }
}

void rcv_cl_qry()
{
    long cnt;
    long i;
    long clData[DATABASE];
    msg_t cl_msg;
    cl_msg = build_msg(1);
    send(node[CLIENTS].mailbox, (long)cl_msg);
    status_mailboxes();
    cnt = msg_cnt(node[CLIENTS].mailbox);
    if (cnt > 0)
    {
        for (i = 0; i < cnt; i++)
        {
            receive(node[CLIENTS].mailbox, (long *)&cl_msg);
            server.bcast_data[cl_msg->data_id] = clData[cl_msg->data_id] = cl_msg->data_id;
        }
    }
}

void bcastIR()
{
    printf("found here----------------------");
    long i;
    msg_t irData;
    irData = build_msg(0);
    irData->timestamp = clock;
    for (i = 0; i < CLIENTS; i++)
    {
        send(node[i].mailbox, (long)irData);
    }
    status_mailboxes();
}

void clientProc()
{
    send_qry();
}

void send_qry()
{
    printf("\n Found here in client too --------------------------________________________++++++++++++++++++++++");
    long i, j, k;
    msg_t query;
    for (i = 0; i < CLIENTS; i++)
    {
        query = build_msg(1);
        for (j = 0; j < CACHE; j++)
        {
            k = clientNodes[i].cache[j].data_item_id;
            if (k == query->data_id)
            {
            }
        }
        send(node[CLIENTS].mailbox, (long)query);
    }
    status_mailboxes();
}

void rcv_sv_IR()
{
}

msg_t build_msg(n) long n;
{
    msg_t mes;
    double aProb;
    long i, rand;
    if (msg_queue == NIL)
    {
        mes = (msg_t)do_malloc(sizeof(struct msg));
    }
    else
    {
        mes = msg_queue;
        msg_queue = msg_queue->link;
    }
    if (n < 1)
    {
        for (i = 0; i < DATABASE; i++)
        {
            mes->bcastData[i].data_id = server.bcast_data[i];
            mes->bcastData[i].data_ts = server.data_items[i];
        }
    }
    else
    {
        aProb = uniform(0.0, 1.0);
        rand = (aProb < 0.8) ? random(1, 50) : random(51, 1000);
        mes->data_id = rand;
    }
    return mes;
}
