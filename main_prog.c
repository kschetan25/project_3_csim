#include "csim.h"
#include <stdio.h>

#define DATABASE 1000L
#define CLIENTS 10L
#define SERVERS 1L
#define NODES 11L
#define SIMTIME 100.0

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
// void rcv_sv_IR();
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
    // clientProc();
}

void serverProc()
{
    updateDB();
    rcv_cl_qry();
    bcastIR();
}

void updateDB()
{
    double rand;
    long udata;
    //create("updateProcess");
    //hold(50.0);
    while (clock < SIMTIME)
    {
        rand = random(0, 1);
        udata = (rand < 0.33) ? uniform(1, 50) : uniform(51, 1000);
        {
            server.data_items[udata] = clock;
            server.bcast_data[udata] = udata;
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
    printf("\n found here---------------------- \n");
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

void send_qry()
{

}

msg_t build_msg(n) long n;
{
    msg_t mes;
    long i;
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
        mes->data_id = 2;
    }
    return mes;
}
