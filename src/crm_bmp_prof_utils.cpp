#include "crm_bmp_prof.h"



//
// convert count pulses at a given freq (1/freq) to duration in nsecs, i.e. units for 1.0E9 (1/freq).
//
int64_t counter2nsecs(int64_t active_counter_frequency, int64_t run_time_consumption)
{
    if (active_counter_frequency > 0 && active_counter_frequency < 1000000000)
    {
        int64_t multiplier = 1000000000 / active_counter_frequency;
        int64_t modulo = 1000000000 % active_counter_frequency;
        int64_t ret;

        // to prevent overflow in 64-bit for very large numbers:
        ret = multiplier * run_time_consumption + (modulo * run_time_consumption) / active_counter_frequency;

        return ret;
    }
    else
    {
        return run_time_consumption;
    }
}






void fprintf_nsecs2dhmsss(FILE *of, int day_width, int subsecond_width, int64_t nsecs)
{
    int secs;
    int minutes;
    int hours;
    int days;
    int sspower = subsecond_width;

    secs = (int)(nsecs / 1000000000);
    nsecs %= 1000000000;
    minutes = secs / 60;
    secs %= 60;
    hours = minutes / 60;
    minutes %= 60;
    days = hours / 24;
    hours %= 24;

    // fetch only the N most significant decimal digits out of nsecs:
    while (sspower-- > 0)
    {
        nsecs *= 10;
    }
    nsecs /= 1000000000;

    // Days:Hours:Minutes:Seconds.subseconds
    fprintf(of, "%*d:%02d:%02d:%02d.%0*d",
            day_width, days,
            hours,
            minutes,
            secs,
            subsecond_width, (int)nsecs);
}



//
// Stripped-down statement table, copied from crm_compiler.c
//
// WARNING: one line for each opcode: extra lines for CRM_NOOP, etc. have been ditched!
//
static const STMT_DEF_TYPE stmt_table[] =
{
    { "(bogus)",   CRM_BOGUS,         0 },
    { ":label:",   CRM_LABEL,         0 },
    { "#",         CRM_NOOP,          0 },
    { "insert",    CRM_INSERT,        0 },
    { "{",         CRM_OPENBRACKET,   0 },
    { "}",         CRM_CLOSEBRACKET,  0 },
    { "accept",    CRM_ACCEPT,        1 },
    { "alius",     CRM_ALIUS,         1 },
    { "alter",     CRM_ALTER,         1 },
    { "call",      CRM_CALL,          1 },
    { "cssmerge",  CRM_CSS_MERGE,     1 },
    { "cssdiff",   CRM_CSS_DIFF,      1 },
    { "cssbackup", CRM_CSS_BACKUP,    1 },
    { "cssrestore", CRM_CSS_RESTORE, 1 },
    { "cssinfo",   CRM_CSS_INFO,      1 },
    { "cssanalyze", CRM_CSS_ANALYZE, 1 },
    { "csscreate", CRM_CSS_CREATE,    1 },
    { "cssmigrate", CRM_CSS_MIGRATE, 1 },
    { "debug",     CRM_DEBUG,         0 },
    { "eval",      CRM_EVAL,          1 },
    { "exit",      CRM_EXIT,          1 },
    { "fail",      CRM_FAIL,          1 },
    { "fault",     CRM_FAULT,         1 },
    { "goto",      CRM_GOTO,          0 },
    { "hash",      CRM_HASH,          1 },
    { "input",     CRM_INPUT,         1 },
    { "intersect", CRM_INTERSECT,     1 },
    { "isolate",   CRM_ISOLATE,       1 },
    { "lazy",      CRM_LAZY,          1 },
    { "learn",     CRM_LEARN,         1 },
    { "classify",  CRM_CLASSIFY,      1 },
    { "liaf",      CRM_LIAF,          1 },
    { "match",     CRM_MATCH,         1 },
    { "mutate",    CRM_MUTATE,        1 },
    { "output",    CRM_OUTPUT,        1 },
    { "clump",     CRM_CLUMP,         1 },
    { "pmulc",     CRM_PMULC,         1 },
    { "return",    CRM_RETURN,        1 },
    { "routine",   CRM_ROUTINE,       1 },
    { "sort",      CRM_SORT,          1 },
    { "syscall",   CRM_SYSCALL,       1 },
    { "translate", CRM_TRANSLATE,     1 },
    { "trap",      CRM_TRAP,          1 },
    { "union",     CRM_UNION,         1 },
    { "window",    CRM_WINDOW,        1 },
};


const STMT_DEF_TYPE *get_stmt_def(int opcode)
{
    int i;

    for (i = 0; i < WIDTHOF(stmt_table); i++)
    {
        if (stmt_table[i].stmt_code == opcode)
        {
            return &stmt_table[i];
        }
    }
    return &stmt_table[0];
}







typedef struct
{
    crmhash_t hash;
    char      token[16];
} HASH_CHECKCACHE_ELEM;


typedef struct
{
    int left;
    int right;
    int parent;
    int same;
} HASH_CHECKCACHE_TREENODE;


static HASH_CHECKCACHE_TREENODE *h_nodes = NULL;
static HASH_CHECKCACHE_ELEM *h_store = NULL;
static int hash_checkcache_size = 0;


int init_hash_checkcache(int size)
{
    int i;

    h_nodes = (HASH_CHECKCACHE_TREENODE *)calloc(size, sizeof(h_nodes[0]));
    h_store = (HASH_CHECKCACHE_ELEM *)calloc(size, sizeof(h_store[0]));

    hash_checkcache_size = size;

    // init tree
    for (i = 0; i < size; i++)
    {
        h_nodes[i].parent = -1;
        h_nodes[i].left = -1;
        h_nodes[i].right = -1;
    }

    return 0;
}


static int hash_checkcache_tree_root = -1;


void remove_hash_checkcache_node(int index)
{
    int l;
    int r;
    int p;

    // remove overwritten element from the tree.

    // remove: find which of the two branches (left & right) has the first available connection (least deep)
    // then connect the other branch to that connection, and connect the branch itself to parent.
    l = h_nodes[index].left;
    r = h_nodes[index].right;
    p = h_nodes[index].parent;
    if (l != -1 && r != -1)
    {
        // complex case: connect as described above.
        for ( ; ;)
        {
            if (h_nodes[l].right == -1)
            {
                // connect 'right' branch to right node there.
                r = h_nodes[index].right;
                h_nodes[l].right = r;
                h_nodes[r].parent = l;

                // also correct our parent now:
                l = h_nodes[index].left;
                if (p != -1)
                {
                    if (h_nodes[p].left == index)
                    {
                        h_nodes[p].left = l;
                    }
                    else
                    {
                        h_nodes[p].right = l;
                    }
                }
                else
                {
                    // we are the root! Move the root!
                    hash_checkcache_tree_root = l;
                }

                // we're done!
                break;
            }
            else if (h_nodes[r].left == -1)
            {
                // connect 'right' branch to right node there.
                l = h_nodes[index].left;
                h_nodes[r].left = l;
                h_nodes[l].parent = r;

                // also correct our parent now:
                r = h_nodes[index].right;
                if (p != -1)
                {
                    if (h_nodes[p].left == index)
                    {
                        h_nodes[p].left = r;
                    }
                    else
                    {
                        h_nodes[p].right = r;
                    }
                }
                else
                {
                    // we are the root! Move the root!
                    hash_checkcache_tree_root = r;
                }

                // we're done!
                break;
            }
            else
            {
                // go one level deeper into both branches: walk the outer edges of both wehere they are closest together.
                l = h_nodes[l].right;
                r = h_nodes[r].left;
            }
        }
    }
    else if (l != -1)
    {
        // only left branch exists: easy: connect to parent as is.
        if (p != -1)
        {
            if (h_nodes[p].left == index)
            {
                h_nodes[p].left = l;
            }
            else
            {
                h_nodes[p].right = l;
            }
        }
        else
        {
            // we are the root! Move the root!
            hash_checkcache_tree_root = l;
        }
        h_nodes[l].parent = p;
    }
    else     // if (r != -1)
    {
        // only right branch or NO BRANCH (r == -1) exists: easy: connect to parent as is.
        // If we are the last node, the root is automagically also reset to -1 once we're done here.
        if (p != -1)
        {
            if (h_nodes[p].left == index)
            {
                h_nodes[p].left = r;
            }
            else
            {
                h_nodes[p].right = r;
            }
        }
        else
        {
            // we are the root! Move the root!
            //
            // WARNING: extra conditional in here to prevent tree destruction by removing an
            //          already detached node (which has parent == -1):
            if (index == hash_checkcache_tree_root)
            {
                hash_checkcache_tree_root = r;
            }
        }
        h_nodes[r].parent = p;
    }

    h_nodes[index].parent = -1;
    h_nodes[index].left = -1;
    h_nodes[index].right = -1;
}




//
// return: 0: OK; 1: partial collision detected; insert OK; 2: full collision detected: insert OK
//
int insert_hash_checkcache_node(int index)
{
    int i;


    // insert element into tree:

    i = hash_checkcache_tree_root;
    if (i == -1)
    {
        // easy: empty tree, so we are root from now.
        hash_checkcache_tree_root = index;
    }
    else
    {
        // traverse tree to find insertion point: if we are equal, we make ourselves parent, if we are less, we go left, if we are more, we go right.
        for ( ; ;)
        {
            int hd = (h_store[i].hash == h_store[index].hash ? 0 : h_store[i].hash < h_store[index].hash ? -1 : 1);

            if (!hd)
            {
                int sd = strncmp(h_store[i].token, h_store[index].token, WIDTHOF(h_store[i].token));
                if (!sd)
                {
                    // exact match (or a hash collision beyond the token width, but we cannot see that)
                    //
                    // so we PULL this reference forward: we make ourselves the parent of [i], so that
                    // the search will find us before the older [i] node.
                    //
                    // As an added bonus, we can complete REMOVE the old node then!
                    h_nodes[index].right = i;
                    h_nodes[index].parent = h_nodes[i].parent;
                    h_nodes[i].parent = index;

                    if (i == hash_checkcache_tree_root)
                    {
                        hash_checkcache_tree_root = index;
                    }

                    remove_hash_checkcache_node(i);

                    // we're done!
                    return 2;
                }
                else
                {
                    // only difference in token. hm... this is a TRUE hash collision!
                    if (sd < 0)
                    {
                        if (h_nodes[i].left == -1)
                        {
                            // add this node to the LEFT branch there:
                            h_nodes[i].left = index;
                            h_nodes[index].parent = i;

                            // we're done
                            return 1;
                        }
                        else if (h_store[h_nodes[i].left].hash != h_store[index].hash)
                        {
                            // left node differs in hash already, so place new node
                            // between this [i] and [left].
                            h_nodes[index].left = h_nodes[i].left;
                            h_nodes[h_nodes[i].left].parent = index;
                            h_nodes[i].left = index;
                            h_nodes[index].parent = i;

                            // we're done
                            return 1;
                        }
                        else
                        {
                            // LEFT node has same hash: check against that token in the next round
                            i = h_nodes[i].left;
                        }
                    }
                    else
                    {
                        if (h_nodes[i].right == -1)
                        {
                            // add this node to the RIGHT branch there:
                            h_nodes[i].right = index;
                            h_nodes[index].parent = i;

                            // we're done
                            return 1;
                        }
                        else if (h_store[h_nodes[i].right].hash != h_store[index].hash)
                        {
                            // right node differs in hash already, so place new node
                            // between this [i] and [right].
                            h_nodes[index].right = h_nodes[i].right;
                            h_nodes[h_nodes[i].right].parent = index;
                            h_nodes[i].right = index;
                            h_nodes[index].parent = i;

                            // we're done
                            return 1;
                        }
                        else
                        {
                            // LEFT node has same hash: check against that token in the next round
                            i = h_nodes[i].right;
                        }
                    }
                }
            }
            else
            {
                // difference in hash
                if (hd < 0)
                {
                    if (h_nodes[i].left == -1)
                    {
                        // add this node to the LEFT branch there:
                        h_nodes[i].left = index;
                        h_nodes[index].parent = i;

                        // we're done
                        break;
                    }
                    else
                    {
                        // check against left node in the next round
                        i = h_nodes[i].left;
                    }
                }
                else
                {
                    if (h_nodes[i].right == -1)
                    {
                        // add this node to the RIGHT branch there:
                        h_nodes[i].right = index;
                        h_nodes[index].parent = i;

                        // we're done
                        break;
                    }
                    else
                    {
                        // check against RIGHT node in the next round
                        i = h_nodes[i].right;
                    }
                }
            }
        }
    }
    return 0;
}



int find_hash_checkcache_node(crmhash_t hash, const char *str)
{
    int i;

    i = hash_checkcache_tree_root;
    if (i == -1)
    {
        // easy: empty tree, so we cannot be found.
        return -1;
    }
    else
    {
        // traverse tree to find matching node, if we are less, we go left, if we are more, we go right.
        for ( ; ;)
        {
            int hd = (h_store[i].hash == hash ? 0 : h_store[i].hash < hash ? -1 : 1);
            int sd = strncmp(h_store[i].token, str, WIDTHOF(h_store[i].token));

            if (!hd && !sd)
            {
                // exact match
                return i;
            }
            else if (!hd)
            {
                // only difference in token. hm... this is a TRUE hash collision!
                if (sd < 0)
                {
                    if (h_nodes[i].left == -1)
                    {
                        return -2;                         // signal partial collision, while not found
                    }
                    else
                    {
                        i = h_nodes[i].left;
                    }
                }
                else                 // if (sd > 0)
                {
                    if (h_nodes[i].right == -1)
                    {
                        return -2;                         // signal partial collision, while not found
                    }
                    else
                    {
                        i = h_nodes[i].right;
                    }
                }
            }
            else
            {
                if (hd < 0)
                {
                    if (h_nodes[i].left == -1)
                    {
                        return -1;
                    }
                    else
                    {
                        i = h_nodes[i].left;
                    }
                }
                else                 // if (hd > 0)
                {
                    if (h_nodes[i].right == -1)
                    {
                        return -1;
                    }
                    else
                    {
                        i = h_nodes[i].right;
                    }
                }
            }
        }
    }
}



static int hash_checkcache_fifo_pos = -1;



//
// return: 0: OK; 1: partial collision detected; insert OK; 2: full collision detected: insert OK
//
int add_hash_checkcache_node(crmhash_t hash, const char *str)
{
    HASH_CHECKCACHE_ELEM *sn;
    int i;

    hash_checkcache_fifo_pos++;
    hash_checkcache_fifo_pos %= hash_checkcache_size;

    // first: remove overwritten element from the tree.
    //
    // then: insert new element into the search tree.
    remove_hash_checkcache_node(hash_checkcache_fifo_pos);

    sn = h_store + hash_checkcache_fifo_pos;
    sn->hash = hash;

    for (i = 0; i < WIDTHOF(sn->token); i++)
    {
        if (*str)
        {
            sn->token[i] = *str++;
        }
        else
        {
            // pad with NUL bytes
            for ( ; i < WIDTHOF(sn->token); i++)
            {
                sn->token[i] = 0;
            }
        }
    }

    // insert element into tree:
    return insert_hash_checkcache_node(hash_checkcache_fifo_pos);
}


