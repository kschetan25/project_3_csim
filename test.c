#include "csim.h"
#include <stdio.h>

#define DATABASE 1000L
#define CLIENTS 100L
#define SERVERS 1L
#define NODES 101L
#define SIMTIME 100.0
#define CACHE 100L

typedef struct msg *msg_t;

struct msg
{
    double data_ts;
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
    long cacheFull;
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

long msg_cnt(MBOX m);

void sim()
{
    create("sim");
    init();
    hold(SIMTIME);
}

void init()
{
    long i, j;
    char str[24];
    max_servers(SERVERS * SERVERS + SERVERS);
    max_mailboxes(NODES * NODES + NODES);
    max_events(NODES * NODES * NODES);
    max_messages(NODES * NODES * NODES);
    max_processes(NODES * NODES * NODES);

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
    printf("\n ************************************ SERVER functionality in progress ************************************");
    create("server");
    while (clock < SIMTIME)
    {
        hold(expntl(10.0));
        updateDB();
        rcv_cl_qry();
        bcastIR();
    }
}

void updateDB()
{
    printf("\n ************************************ Server Update Initiated ************************************");
    double uProb;
    long rand;
    create("update");
    while (clock < SIMTIME)
    {
        hold(expntl(20.0));
        uProb = uniform(0.0, 1.0);
        printf("\n The Update probability is %lf", uProb);
        if (uProb < 0.33)
        {
            rand = uniform(1, 51);
            printf("\n Hot data item %ld picked for update", rand);
        }
        else
        {
            rand = uniform(51, 1000);
            printf("\n Cold data item %ld picked for update", rand);
        }

        server.data_items[rand] = clock;
        server.bcast_data[rand] = rand;
        printf("\n The data is updated at position %ld with the updated time %lf", rand, server.data_items[rand]);
    }
}

void rcv_cl_qry()
{
    create("recli");
    while (clock < SIMTIME)
    {
        hold(expntl(10.0));
        long i, cnt = 0;
        msg_t cl_msg;
        cnt = msg_cnt(node[CLIENTS].mailbox);
        printf("\n The Servers Mailbox has %ld requests from clients", cnt);
        if (cnt > 0)
        {
            for (i = 0; i < cnt; i++)
            {
                receive(node[CLIENTS].mailbox, (long *)&cl_msg);
                printf("\n The requested data item from client is %ld", cl_msg->data_id);
                server.bcast_data[cl_msg->data_id] = cl_msg->data_id;
                printf("\n The requested data item is stored in Lbcast at position %ld with data request id %ld", cl_msg->data_id, server.bcast_data[cl_msg->data_id]);
            }
        }
    }
}

void bcastIR()
{
    long i, j;
    msg_t irData;
    create("bcast");
    while (clock < SIMTIME)
    {
        hold(20.0);
        for (i = 0; i < CLIENTS; i++)
        {
            for (j = 1; j <= DATABASE; j++)
            {
                irData = build_msg(0);
                irData->timestamp = clock;
                irData->data_id = server.bcast_data[j];
                irData->data_ts = server.data_items[j];
                send(node[i].mailbox, (long)irData);
            }
        }
    }
    status_processes();
}

void clientProc()
{
    create("client");
    while (clock < SIMTIME)
    {
        hold(expntl(10.0));
        send_qry();
        rcv_sv_IR();
    }
}

void send_qry()
{
    long i, j, k;
    long hit, miss;
    create("send");
    while (clock < SIMTIME)
    {
        hold(expntl(20.0));
        msg_t query;
        for (i = 0; i < CLIENTS; i++)
        {
            query = build_msg(1);
            for (j = 0; j < CACHE; j++)
            {
                k = clientNodes[i].cache[j].data_item_id;
                if (k == query->data_id)
                {
                    hit++;
                    clientNodes[i].req_data[query->data_id] = query->data_id;
                }
                else
                {
                    miss++;
                }
            }
            send(node[CLIENTS].mailbox, (long)query);
        }
        status_mailboxes();
    }
}

void rcv_sv_IR()
{
    long i, j, k, l, cnt;
    long lru_id;
    double last_accessed;
    create("rcv");
    while (clock < SIMTIME)
    {
        hold(expntl(1));
        msg_t sv_msg;
        for (i = 0; i < CLIENTS; i++)
        {
            last_accessed = clientNodes[i].cache[0].last_accessed;
            lru_id = clientNodes[i].cache[0].data_item_id;
            cnt = msg_cnt(node[i].mailbox);
            if (cnt > 0)
            {
                for (j = 0; j < cnt; j++)
                {
                    receive(node[i].mailbox, (long *)&sv_msg);
                    for (k = 0; k < CACHE; k++)
                    {
                        if (clientNodes[i].cache[CACHE - 1].data_item_id > 0)
                        {
                            if (clientNodes[i].cache[k].last_accessed < last_accessed)
                            {
                                lru_id = k;
                                last_accessed = clientNodes[i].cache[k].last_accessed;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                    for (k = 0; k < CACHE; k++)
                    {
                        if (clientNodes[i].cache[k].data_item_id < 1)
                        {
                            clientNodes[i].cache[k].valid_bit = 1;
                            clientNodes[i].cache[k].data_item_id = sv_msg->data_id;
                            clientNodes[i].cache[k].last_updated = sv_msg->data_ts;
                        }
                        else if (clientNodes[i].cache[k].data_item_id == sv_msg->data_id)
                        {
                            if (clientNodes[i].cache[k].last_updated < sv_msg->data_ts)
                                clientNodes[i].cache[k].last_updated = sv_msg->data_ts;
                        }
                        else if (k == CACHE)
                        {
                            clientNodes[i].cacheFull = 1;
                            clientNodes[i].cache[lru_id].data_item_id = sv_msg->data_id;
                            clientNodes[i].cache[lru_id].last_updated = sv_msg->data_ts;
                            clientNodes[i].cache[lru_id].last_accessed = simtime();
                        }
                    }
                }
            }
        }
    }
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
    if (n > 0)
    {
        aProb = uniform(0.0, 1.0);
        rand = (aProb < 0.8) ? uniform(1, 50) : uniform(51, 1000);
        mes->data_id = rand;
    }
    return mes;
}
