#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/wait.h>
#include "kbshell.h"
#include <readline/readline.h>
#include <readline/history.h>


#define KBSHELL_CMD_STR_MAX 256

typedef unsigned int u32;
typedef unsigned int uint32;


int main(int argc, char *argv[])
{
    int i = 0;
    char cmd_str[KBSHELL_CMD_STR_MAX];
    kbshell_cli_msg_t msg = {0};
    int ret_val = 0;
    uint32_t set_asic = 0;
    char *rl_str;


    /* Parse the list of commands supported */
    kbshell_parse_cons_cmd_tree();

    while(1) {
        sprintf(cmd_str, "\nKBshell#");
        rl_str = readline((const char *)cmd_str);
        add_history(rl_str);

        ret_val = kbshell_parse(rl_str, &msg);
        if (ret_val < 0) {
            fprintf(stderr, "syntax error %s\n", cmd_str);
            continue;
        } 
        
        switch(msg.cli_type) {
        case KBSHELL_CLI_HELP:
            kbshell_cmd_list_print();
            break;        
        default:
		 kbshell_cmd_list_print();
            break; 
        }
        fflush(stdout);
        fflush(stdin);
        free(rl_str); // free the string allocated by readline
    }

    return 0;
}
