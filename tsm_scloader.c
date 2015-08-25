#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "./common/fm_Base.h"
#include "./common/fm_Mem.h"
#include "./common/fm_Util.h"
#include "./common/fm_Log.h"
#include "tsm_Err.h"
#include "tsm_ccb.h"
#include "tsm_scloader.h"

#define DEFAULT_PARAM_NUM      5
#define DEFAULT_SC_ENTRY_NUM    128

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

LOCAL script_entry_t *se_alloc(char *name)
{
    int len = strlen(name);
    script_entry_t *se;

	se = (script_entry_t *)fm_calloc(1,sizeof(script_entry_t));
	if(!se) return NULL;

	se->name = (char *)fm_malloc(len+1);
	if(!se->name){
		fm_free(se);
		return NULL;
	}
    memcpy(se->name,name,len);
	se->name[len] = '\0';

	se->ces = (code_entry_t **)fm_calloc(DEFAULT_SC_ENTRY_NUM,sizeof(code_entry_t *));
	if(!se->ces){
		fm_free(se->name);
		fm_free(se);
		return NULL;
	}

    se->size = DEFAULT_SC_ENTRY_NUM;
	se->code_num = 0;
	return se;
}

LOCAL int se_resize(script_entry_t *se)
{
    code_entry_t **new_ces;

	new_ces = (code_entry_t **)fm_calloc(se->size<<1,sizeof(code_entry_t *));
	if(!new_ces) return -1;

	memcpy_int(new_ces,se->ces,se->code_num);
	fm_free(se->ces);
	se->ces = new_ces;
	return 0;
}
/*
the code entry is in the form of:
byte-code,parm0:parm1:parmn
*/
PUBLIC script_entry_t *scloader_load(char *path)
{
    FILE *fhead;
	char line[1024],file_name[512]={0};
	int line_len,addr = 0;
	code_entry_t *ce;
	script_entry_t *se;
	
	
    fhead = fopen(path, "r");
    if(fhead == NULL){
    	//printf("Open h file faile\n");
    	return NULL;
    }

    get_filename_from_path(path,file_name,sizeof(file_name));
	se = se_alloc(file_name);

	FM_LOGD("in scloader...");

    memset(line,0,sizeof(line));
    while(fgets(line,sizeof(line),fhead) != NULL){

    	if((line[0] == '#')||(line[0] == '\r')){ 
			memset(line,0,sizeof(line));
			continue;
    	}
    	line_len = strlen(line);
    	if((line[line_len - 1] == '\n')||(line[line_len - 1] == '\r')){
    		line[line_len - 1] = '\0';
    	}
		if(line[line_len - 2] == '\r'){
			line[line_len - 2] = '\0';
		}
        FM_LOGH(line,128);
		

        ce = (code_entry_t *)fm_calloc(1,sizeof(code_entry_t));
		ce->addr = addr++;
		if(is_parameterless(line,line_len)){
			sscanf(line,"%d",&ce->code);
			ce->parameters=NULL;
		}else{
		    char *delim = ",:\0";
			char *p;
			fmBytes *parm;
			int i = 0;
			
			ce->code = atoi(strtok(line,delim));
			FM_LOGD("ce->code=%d",ce->code);
			ce->parameters = fmBytes_array_alloc(DEFAULT_PARAM_NUM);
			while((p = strtok(NULL, delim))!=NULL){
				parm = fmBytes_alloc(strlen(p));
				fmBytes_copy_from(parm,0,p,0,strlen(p));
				FM_LOGH(parm->buf,parm->leng);
				if((ce->parameters->size-ce->parameters->cnt)<1){
					ce->parameters = fmBytes_array_resize(ce->parameters,DEFAULT_PARAM_NUM<<1);
				}
				ce->parameters->array[i++] = parm;
				ce->parameters->cnt = i;
			}
		}

		if((se->size-se->code_num)<1){
			se_resize(se);
		}
		se->ces[se->code_num++] = ce;
		memset(line,0,sizeof(line));
    }
	return se;
}
