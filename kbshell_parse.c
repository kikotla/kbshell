/********************************************************************
 *
 *      File: kbshell_parse.c
 *
 *      Description: KB Shell Output Display implementation
 *
 *      2017 MIT License
 *
 *       Author: Kiran Kotla
 ********************************************************************/
#include "kbshell.h"
#include <stdlib.h>


/* Tree structure for command nodes,
 * is in the below format
 *                   _
 *                  |_|
 *               _ /  _       _    _    _
 *              |_|->|_|->...|_|->|_|->|_|->...
 *            _/   _                  _/
 *           |_|->|_|->..            |_|->...
 */
                     
typedef enum
{
    KBSHELL_NODE_TYPE_ROOT = 0,
    KBSHELL_NODE_TYPE_KEYWORD,
    KBSHELL_NODE_TYPE_INT,
    KBSHELL_NODE_TYPE_HEX,
    KBSHELL_NODE_TYPE_STR,
    KBSHELL_NODE_TYPE_MAX
}kbshell_node_type_t;

typedef struct kbshell_cmd_node_s
{
    kbshell_node_type_t node_type;
    char str[64];
    char cmd_str[256]; // complete command string
    uint32_t value;
    kbshell_cli_type_t cli_type; // valid for the last node
    struct kbshell_cmd_node_s *next;
    struct kbshell_cmd_node_s *left;
    struct kbshell_cmd_node_s *parent;
}kbshell_cmd_node_t;

extern kbshell_cmd_t kbshell_cmds[];

kbshell_cmd_node_t *cmd_root = NULL;

void kbshell_cmd_list_print()
{
    int i;

    for(i=0; i<KBSHELL_NUM_CMDS; i++)
    {
        printf("%s\n", kbshell_cmds[i].cmd_str);
    }
}

static int kbshell_parse_cmd_data(kbshell_cmd_t *cmd, kbshell_cmd_node_t *root)
{
    char copy_str[256];
    char *p; 
    kbshell_cmd_node_t *node = root;
    kbshell_cmd_node_t **nodep = &(root->left);

    strncpy(copy_str, cmd->cmd_str, 256);
    p = strtok(copy_str, " ");

    while (p) {
        if (*nodep) {
            /* see if we can find a match as we move right */
            uint8_t match_flag = 0;
            while (*nodep) {
                node = *nodep;
                if (strncmp((*nodep)->str, p, 64) == 0) {
                    /* found a match */
                    nodep = &((*nodep)->left);
                    match_flag = 1;
                    break;
                }
                nodep = &((*nodep)->next);
            }
            if (match_flag) {
                /* dig deeper */         
                p = strtok(NULL, " ");
                continue;
            }
        }

        *nodep = (kbshell_cmd_node_t *)malloc(sizeof(kbshell_cmd_node_t));
        (*nodep)->parent = node; // set the parent first
        node = *nodep;
        bzero(node, sizeof(kbshell_cmd_node_t));
        if (strstr(p, "<") == NULL) {
            node->node_type = KBSHELL_NODE_TYPE_KEYWORD;
        } else if (strstr(p, "num")) {
            node->node_type = KBSHELL_NODE_TYPE_INT;
        } else if (strstr(p, "hex")) {
            node->node_type = KBSHELL_NODE_TYPE_HEX;
        } else if (strstr(p, "str")) {
            node->node_type = KBSHELL_NODE_TYPE_STR;
        }
        strncpy(node->str, p, 64);
        nodep = &node->left;
        
        p = strtok(NULL, " ");
    }

    if (node->node_type != KBSHELL_NODE_TYPE_ROOT) {
        node->cli_type = cmd->cli_type;
        strncpy(node->cmd_str, cmd->cmd_str, 256); // for display purposes
    }

    return 0;
}

int kbshell_parse_cons_cmd_tree()
{
    int i;

    cmd_root = (kbshell_cmd_node_t *)malloc(sizeof(kbshell_cmd_node_t));
    bzero(cmd_root, sizeof(kbshell_cmd_node_t));
    cmd_root->node_type = KBSHELL_NODE_TYPE_ROOT;

    for(i=0; i<KBSHELL_NUM_CMDS; i++)
    {
        kbshell_parse_cmd_data(&kbshell_cmds[i], cmd_root);
    }
    return 0;
}

static int kbshell_help_print(kbshell_cmd_node_t *node)
{
    while (node) {
        if (!node->left) {
            // print help string
            printf("%s\n", node->cmd_str);
        } else {
            kbshell_help_print(node->left);
        }
        node = node->next;
    }

    return 0;
}

static int kbshell_parse_cmd_runtime(char *cmd_str, kbshell_cli_msg_t *msg,
                                kbshell_cmd_node_t *root)
{
    char *p = strtok(cmd_str, " ");
    kbshell_cmd_node_t *node = root;
    kbshell_cmd_node_t **nodep = &(root->left);
    kbshell_cmd_node_t **nodep2 = &(root->left);
    int int_val;
    int hex_val;
    int pos = 0;
    uint8_t *data = msg->data;

    while (p) {
        uint8_t match_flag = 0;
        nodep2 = nodep; // for second pass
        while (*nodep) {
            node = *nodep;
            if (strncmp((*nodep)->str, p, 64) == 0) {
                /* found a match */
                nodep = &((*nodep)->left);
                match_flag = 1;
                break;
            }
            nodep = &((*nodep)->next);
        }
            
        if (match_flag) {
            /* dig deeper */         
            p = strtok(NULL, " ");
            continue;
        } else {
            /* second pass, to check for partial strings */
            nodep = nodep2;
            while (*nodep) {
                node = *nodep;
                /* there is no match for sure, just compare
                 * as many characters are there in 'p'
                 */
                if (strncmp((*nodep)->str, p, strlen(p)) == 0) {
                    /* found a match */
                    nodep = &((*nodep)->left);
                    match_flag += 1;
                    break;
                }
                nodep = &((*nodep)->next);
            }
        }

        if (match_flag == 1) {
            /* dig deeper */         
            p = strtok(NULL, " ");
        } else if (match_flag > 1) {
            /* multiple matches, parse error */
            return -1;
        } else {
            /* third pass to check for params */
            match_flag = 0;
            nodep = nodep2;
            while (*nodep) {
                node = *nodep;
                switch(node->node_type) {
                case KBSHELL_NODE_TYPE_INT:
                    int_val = atoi(p);
                    if (int_val == 0 && p[0] != '0') {
                        /* not a match */
                    } else {
                        data[pos++] = sizeof(int);
                        *((int *)(data+pos)) = int_val;
                        pos += sizeof(int);
                        match_flag = 1;
                    }
                    break;
                case KBSHELL_NODE_TYPE_HEX:
                    sscanf(p, "%x", &hex_val);
                    if (hex_val == 0 && 
                        !( (p[0] == '0') && (p[1] == 'x'))) {
                        /* not a match */
                    } else {
                        data[pos++] = sizeof(int);
                        *((int *)(data+pos)) = hex_val;
                        pos += sizeof(int);
                        match_flag = 1;
                    }
                    break;
                case KBSHELL_NODE_TYPE_STR:
                    data[pos++] = strlen(p);
                    strncpy((char *)data+pos, p, strlen(p));
                    pos += strlen(p);
                    match_flag = 1;
                    break;
                default:
                    break;
                }
                if (match_flag) {
                    break;
                }
                nodep = &((*nodep)->next);
            }

            if (match_flag) {
                nodep = &((*nodep)->left);
                p = strtok(NULL, " ");
            } else {
                printf("Usage: \n"); 
                kbshell_help_print(*nodep2);
                return -1;
            }
        }
    }

    if (node->node_type != KBSHELL_NODE_TYPE_ROOT) {
        msg->cli_type = node->cli_type;
        return 0;
    } else {
        return -1;
    }
}

int kbshell_parse(char *str, kbshell_cli_msg_t *msg)
{
    return kbshell_parse_cmd_runtime(str, msg, cmd_root);
}
