#include "csim.h"
#include <stdio.h>

#define DATABASE 1000L
#define CLIENTS 10L
#define SERVERS 1L
#define NODES 11L
#define SIMTIME 5000.0
#define CACHE 100L

double mean_uat;
double mean_qgt;

typedef struct msg *msg_t;

struct msg
{
    long data_id;
    double data_ts;
    double timestamp;
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
    long hit;
    long miss;
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
long checkItemExists();
void printCache();
msg_t build_msg();

long msg_cnt(MBOX m);

void sim()
{
    create("sim");
    init();
    hold(SIMTIME);
    printCache();
}

void init()
{
    long i, j;
    char str[24];
    max_servers(SERVERS * SERVERS + SERVERS);
    max_mailboxes(NODES * NODES + NODES);
    max_events(10 * NODES * NODES * NODES);
    max_messages(10 * NODES * NODES * NODES);
    max_processes(10 * NODES * NODES * NODES);

    printf("\n What is the Mean Update Arrival Time you Desire ? (1 - 10000) : ");
    scanf("\n %lf", &mean_uat);
    printf("\n What is the mean Query Genertate time you Desire ? (50 - 300) items :");
    scanf("\n %lf", &mean_qgt);

    msg_queue = NIL;

    for (i = 0; i < NODES; i++)
    {
        sprintf(str, "mailbox.%d", i);
        node[i].mailbox = mailbox(str);
    }

    serverProc();
    clientProc();
}

void printCache()
{
    long i, k;
    for (i = 0; i < CLIENTS; i++)
    {
        for (k = 0; k < CACHE; k++)
        {
            printf("\n Client %ld has cache item %ld at position %ld", i, clientNodes[i].cache[k].data_item_id, k);
        }
        printf("\n Cache Hit : %ld", clientNodes[i].hit);
        printf("\n Cache Miss : %ld", clientNodes[i].miss);
        printf("\n ------------------------------------------------------------------------------------------ \n");
    }
}

void serverProc()
{
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
    double uProb;
    long rand_data;
    create("update");
    while (clock < SIMTIME)
    {
        hold(expntl(mean_uat));
        uProb = uniform(0.0, 1.0);
        printf("\n The Update probability is %lf", uProb);
        if (uProb < 0.33)
        {
            rand_data = uniform(1, 50);
            printf("\n Hot data item %ld picked for update at %3.3f", rand_data, clock);
        }
        else
        {
            rand_data = uniform(51, 1000);
            printf("\n Cold data item %ld picked for update at %3.3f", rand_data, clock);
        }

        server.data_items[rand_data] = clock;
        server.bcast_data[rand_data] = rand_data;
        printf("\n The data is updated at position %ld with the updated time %lf at %3.3f", rand_data, server.data_items[rand_data], clock);
    }
}

void rcv_cl_qry()
{
    long i, cnt = 0;
    msg_t cl_msg;
    create("recli");
    while (clock < SIMTIME)
    {
        hold(expntl(10.0));
        cnt = msg_cnt(node[CLIENTS].mailbox);
        if (cnt > 0)
        {
            printf("\n The Servers Mailbox has %ld requests from clients at %3.3f", cnt, clock);
            for (i = 0; i < cnt; i++)
            {
                receive(node[CLIENTS].mailbox, (long *)&cl_msg);
                printf("\n The requested data item from client is %ld", cl_msg->data_id);
                server.bcast_data[cl_msg->data_id] = cl_msg->data_id;
                printf("\n The requested data item is stored in Lbcast at position %ld with data request id %ld at %3.3f", cl_msg->data_id, server.bcast_data[cl_msg->data_id], clock);
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
        printf("\n The server initiated the broadcast to all clients at %3.3f", clock);
        for (i = 0; i < CLIENTS; i++)
        {
            printf("\n The server broadcasted data items to client %ld at %3.3f", i, clock);
            for (j = 1; j <= DATABASE; j++)
            {
                if ((server.bcast_data[j] >= 1) && (server.bcast_data[j] <= 1000))
                {
                    irData = build_msg(0);
                    irData->timestamp = clock;
                    irData->data_id = server.bcast_data[j];
                    irData->data_ts = server.data_items[j];
                    send(node[i].mailbox, (long)irData);
                    hold(0.8);
                }
            }
        }
        for (i = 0; i < DATABASE; i++)
        {
            server.bcast_data[i] = 0;
        }
    }
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
    long found;
    msg_t query;
    create("send");
    while (clock < SIMTIME)
    {
        hold(expntl(mean_qgt));
        for (i = 0; i < CLIENTS; i++)
        {
            query = build_msg(1);

            printf("\n The client %ld queried for data item with id %ld at %3.3f", i, query->data_id, clock);
            found = 0;
            for (j = 0; j < CACHE; j++)
            {
                k = clientNodes[i].cache[j].data_item_id;
                if (k == query->data_id)
                {
                    printf("\n The queried data was found in cache at location %ld, waiting for next IR to validate the data at %3.3f", j, clock);
                    clientNodes[i].cache[j].last_accessed = clock;
                    clientNodes[i].req_data[query->data_id] = query->data_id;
                    found = 1;
                }
                else
                {
                    printf("\n The queried data was not found, sending a request for data with id %ld to server at %3.3f", query->data_id, clock);
                    send(node[CLIENTS].mailbox, (long)query);
                    break;
                }
            }
            if (clientNodes[i].cacheFull == 1)
                (found == 1) ? (clientNodes[i].hit += 1) : (clientNodes[i].miss += 1);
        }
    }
}

void rcv_sv_IR()
{
    long i, j, k, l, cnt;
    long lru_id = 0, duplicate;
    double last_accessed;
    create("rcv");
    while (clock < SIMTIME)
    {
        hold(expntl(10.0));
        msg_t sv_msg;
        for (i = 0; i < CLIENTS; i++)
        {
            last_accessed = clientNodes[i].cache[0].last_accessed;
            cnt = msg_cnt(node[i].mailbox);
            if (cnt > 0)
            {
                printf("\n The client %ld has received broadcast with %ld data items at %3.3f", i, cnt, clock);
                for (j = 0; j < cnt; j++)
                {
                    receive(node[i].mailbox, (long *)&sv_msg);
                    printf("\n The client %ld received data with data id %ld and updated as %lf", i, sv_msg->data_id, sv_msg->data_ts);
                    for (k = 0; k < CACHE; k++)
                    {
                        if (clientNodes[i].cache[CACHE - 1].data_item_id > 0)
                        {
                            clientNodes[i].cacheFull = 1;
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
                    duplicate = checkItemExists(i, sv_msg->data_id);
                    if (duplicate > 0)
                    {
                        if (clientNodes[i].cache[duplicate].last_updated < sv_msg->data_ts)
                        {
                            clientNodes[i].cache[duplicate].last_updated = sv_msg->data_ts;
                            printf("\n Data item with id %ld updated with %lf in cache at position %ld for client %ld at %3.3f", clientNodes[i].cache[k].data_item_id, clientNodes[i].cache[k].last_updated, k, i, clock);
                        }
                    }
                    else if (duplicate < 1)
                    {
                        for (l = 0; l < CACHE; l++)
                        {
                            if (clientNodes[i].cache[l].data_item_id < 1)
                            {
                                clientNodes[i].cache[l].valid_bit = 1;
                                clientNodes[i].cache[l].data_item_id = sv_msg->data_id;
                                clientNodes[i].cache[l].last_updated = sv_msg->data_ts;
                                printf("\n Data item with id %ld inserted in cache at position %ld for client %ld at %3.3f", clientNodes[i].cache[l].data_item_id, l, i, clock);
                                break;
                            }
                        }
                        if (clientNodes[i].cacheFull == 1)
                        {
                            clientNodes[i].cache[lru_id].data_item_id = sv_msg->data_id;
                            clientNodes[i].cache[lru_id].last_updated = sv_msg->data_ts;
                            clientNodes[i].cache[lru_id].last_accessed = clock;
                            printf("\n Cache full for client %ld", i);
                            printf("Applying LRU policy to insert the new data item with id %ld in the cache at %ld position at %3.3f", i, clientNodes[i].cache[lru_id].data_item_id, lru_id, clock);
                        }
                    }
                }
            }
        }
    }
}

long checkItemExists(long clientId, long dataId)
{
    long dup, i;
    for (i = 0; i < CACHE; i++)
    {
        if (clientNodes[clientId].cache[i].data_item_id == dataId)
        {
            dup = i;
            break;
        }
    }
    return dup;
}

msg_t build_msg(n) long n;
{
    msg_t mes;
    double aProb;
    long i, rand_data;
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
        printf("\n The Update probability is %lf", aProb);
        if (aProb < 0.8)
        {
            rand_data = uniform(1, 50);
            printf("\n Hot data item %ld is queried at %3.3f", rand_data, clock);
        }
        else
        {
            rand_data = uniform(51, 1000);
            printf("\n Cold data item %ld is queried at %3.3f", rand_data, clock);
        }

        mes->data_id = rand_data;
    }
    return mes;
}
