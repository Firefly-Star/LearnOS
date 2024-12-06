#include "IPC.h"

struct ipc_hash_table* semephore_table;

static int hash(key_t key)
{
    return key % TABLE_SIZE;
}



void ipc_init()
{

}