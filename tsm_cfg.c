#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "./common/fm_Util.h"
#include "./common/fm_Mem.h"
#include "tsm_cfg.h"
#include "tsm_Err.h"


LOCAL int cfg_parse_line(tsm_cfg_t *cfg,char *line,int *section)
{
    int nscan,nread;
	char name[1024],value[128],word_1[128],word_2[128],word_3[128];
	agent_info_t *agent;
	
	if (line[0] == '[') {
		int last_section = *section;
		nscan = sscanf(line, "[ %[^] \t]", name);
		if (STRCMP(name, ==, "global")) {
			*section = 1;
		} else if (STRCMP(name, ==, "agent")) {
			*section = 2;
		} else {
			//zc_error("wrong section name[%s]", name);
			return -1;
		}
		/* check the sequence of section, must increase */
		if (last_section >= *section) {
			//zc_error("wrong sequence of section, must follow global->levels->formats->rules");
			return -1;
		}

		return 0;
	}
	
	memset(word_1, 0x00, sizeof(word_1));
	memset(word_2, 0x00, sizeof(word_2));
	memset(word_3, 0x00, sizeof(word_3));
	switch(*section){
		case 1:
    		memset(name, 0x00, sizeof(name));
    		memset(value, 0x00, sizeof(value));
			nscan = sscanf(line, " %[^=]= %s ", name, value);
    		if (nscan != 2) {
    			//zc_error("sscanf [%s] fail, name or value is null", line);
    			return -1;
    		}
    		nread = 0;
    		nscan = sscanf(name, "%s%n%s%s", word_1, &nread, word_2, word_3);
			str_trim(value);
			
    		if (STRCMP(word_1, ==, "bytecode")){
                if(STRCMP(word_2, ==, "parameter") && STRCMP(word_3, ==, "max")){
					cfg->bc_parm_max = atoi(value);
                }else if(STRCMP(word_2, ==, "class") && STRCMP(word_3, ==, "max")){
                    cfg->bc_class_num = atoi(value);
                }else if(STRCMP(word_2, ==, "class") && STRCMP(word_3, ==, "room")){
                    cfg->bc_class_room = atoi(value);
                }else{
                    return -1;
                }
    		} else {
    			//zc_error("name[%s] is not any one of global options", name);
    			return -1;
    		}

			break;
		case 2:
            nscan = sscanf(name, "%d,%s,%s,%s",&nread, word_1, word_2, word_3);
			if(nscan != 4){
				return -1;
			}
			agent = (agent_info_t *)fm_calloc(1,sizeof(agent_info_t));
			agent->agent_id = nread;
			agent->name = (char *)fm_malloc(strlen(word_1));
			memcpy(agent->name,word_1,strlen(word_1));
			agent->agent_r_fifo = (char *)fm_malloc(strlen(word_2));
			memcpy(agent->agent_r_fifo,word_2,strlen(word_2));
			agent->agent_w_fifo = (char *)fm_malloc(strlen(word_3));
			memcpy(agent->agent_w_fifo,word_3,strlen(word_3));
			list_add_entry(&cfg->ag_list,&agent->entry);
			break;
		default:
			return -1;
			break;
	}
	return 0;
}

/* section [global:1] [agent:2] */
PUBLIC tsm_cfg_t *cfg_load(char *cfg_path)
{
    FILE *fcfg;
	char line[1024];
	int line_len,section;
	tsm_cfg_t *cfg;
	
	cfg = fm_calloc(1,sizeof(tsm_cfg_t));
	if(!cfg){
		return NULL;
	}
	
    fcfg = fopen(cfg_path, "r");
    if(fcfg == NULL){
    	//printf("Open h file faile\n");
    	return NULL;
    }

    section = 0;
	while(fgets(line,sizeof(line),fcfg) != NULL){
    	if((line[0] == '#')||(line[0] == '\n')) continue;
    	line_len = strlen(line);
    	if(line[line_len - 1] == '\n'){
    		line[line_len - 1] = '\0';
    	}
		trim_lr(line);
		cfg_parse_line(cfg,line,&section);
	}
	return cfg;
}
