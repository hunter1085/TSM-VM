#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "./common/fm_Base.h"
#include "./common/fm_Mem.h"
#include "./common/fm_Util.h"
#include "tsm_Err.h"
#include "tsm_ccb.h"
#include "tsm_scloader.h"



LOCAL fmBool is_parameterless(char *line,int len)
{
    int i;
	fmBool result = fm_true;

	for(i = 0; i < len; i++){
		if(line[i] == ','){
			result = fm_false;
			break;
		}
	}
	return result;
}
/*
the code entry is in the form of:
byte-code,parm0:parm1:parmn
*/
PUBLIC script_entry_t *scloader_load(char *path)
{
    FILE *fhead;
	char line[1024];
	int line_len;
	code_entry_t *ce;
	script_entry_t *se;
	
	
    fhead = fopen(path, "r");
    if(fhead == NULL){
    	//printf("Open h file faile\n");
    	return NULL;
    }

	se = (script_entry_t *)fm_calloc(1,sizeof(script_entry_t));
	list_init(&se->c_list);

    while(fgets(line,sizeof(line),fhead) != NULL){

    	if((line[0] == '#')||(line[0] == '\n')) continue;
    	line_len = strlen(line);
    	if(line[line_len - 1] == '\n'){
    		line[line_len - 1] = '\0';
    	}

        ce = (code_entry_t *)fm_calloc(1,sizeof(code_entry_t));
		if(is_parameterless(line,line_len)){
			sscanf(line,"%d",&ce->code);
			ce->parameters.size = 0;
			ce->parameters.array=NULL;
		}else{
		    char *delim = ",:\0";
			char *p[MAX_PARM_NUM];
			fmBytes *parm;
			int i = 0;
			
			ce->code = atoi(strtok(line,delim));
			while((p[i++] = strtok(NULL, delim))){
				if(i >= MAX_PARM_NUM){
					fm_free(se);
					return NULL;
				}
			}
			ce->parameters.size = i;
			ce->parameters.array = fm_calloc(ce->parameters.size,sizeof(fmBytes *));
			for(i = 0; i < ce->parameters.size; i++){
				parm = fmBytes_alloc(strlen(p[i])/2);
				str_2_hexstr(p[i],fmBytes_get_buf(parm));
				fmBytes_set_length(parm,strlen(p[i])/2);
				ce->parameters.array[i] = parm;
			}
		}
		
        list_add_entry(se->c_list,ce->node);
		se->code_num++;
    }
	return se;
}
