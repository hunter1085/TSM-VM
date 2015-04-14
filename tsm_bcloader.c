#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include "./common/fm_Base.h"
#include "./common/fm_Mem.h"
#include "./common/fm_Util.h"
#include "tsm_Err.h"
#include "tsm_ccb.h"
#include "tsm_vm.h"
#include "tsm_bcloader.h"

#define DEF_BYCODE_SIZE     256
#define DEF_BYCODE_SET_SIZE 32



typedef struct{
	int size;//in case cnt>=size,we should resize
	int base;
	int cnt;
	char **array;
}func_array_t,func_class_t;




LOCAL void func_array_resize(func_array_t *p)
{
	char **new_array;
	int i,size;

    size = p->size<<1;
	new_array = (char **)fm_calloc(size,sizeof(char *));
	for(i = 0; i < p->cnt; i++){
		new_array[i] = p->array[i];
	}
	fm_free(p->array);
	p->array = new_array;
	p->size  = size;
}
LOCAL func_array_t *func_array_alloc(int array_size)
{
    func_array_t *p;

	p = (func_array_t *)fm_calloc(1,sizeof(func_array_t));
	if(p == NULL) return NULL;

	p->array = (char **)fm_calloc(array_size,sizeof(char *));
	if(p->array == NULL){
		fm_free(p);
		return NULL;
	}

	p->size = array_size;
	p->cnt  = 0;
	return p;
}
LOCAL void func_array_add_entry(func_array_t *p,char *e)
{
    if(p->cnt >= p->size){
		func_array_resize(p);
    }
	p->array[p->cnt] = e;
	p->cnt++;
}
LOCAL void func_array_free(func_array_t *p)
{
    int i;

	if(!p) return;

	for(i = 0; i < p->cnt; i++){
		if(p->array[i])
		    fm_free(p->array[i]);
	}

    if(p->array == NULL)
	    fm_free(p->array);
	fm_free(p);
}
LOCAL bc_set_t *byte_code_set_alloc(char *name,int base,int cnt)
{
    bc_set_t *bc_set;

    bc_set = (bc_set_t *)fm_calloc(1,sizeof(bc_set_t));
	if(!bc_set) return NULL;
	bc_set->name = (char *)fm_calloc(strlen(name)+1,sizeof(char));
	if(!bc_set->name){
		fm_free(bc_set->name);
		return NULL;
	}
	memcpy(bc_set->name,name,strlen(name));
	bc_set->base = base;
	bc_set->cnt  = cnt;
	bc_set->bc = (int *)fm_calloc(cnt,sizeof(tsm_bytecode));
	if(bc_set->bc == NULL){
		fm_free(bc_set->name);
		fm_free(bc_set);
		return NULL;
	}
	return bc_set;
}
LOCAL void byte_code_set_free(bc_set_t *bc_set)
{
    if(!bc_set) return;

	if(bc_set->bc)fm_free(bc_set->bc);
	fm_free(bc_set);
}

LOCAL int bcloader_get_libfile_name(char *path,char *name,int name_len)
{
    int index;
	char *start,*end;

    if((path == NULL) || (name == NULL)) return -1;
	
    index = last_index_of(path,'/');
	if(index == -1){
		if(strlen(path)>name_len) return -1;
		strcpy(name,path);
		return 0;
	}
	start = path+index+1;
	end = path + strlen(path);
	if((end-start)>name_len) return -1;
	memcpy(name,start,end-start);
	return 0;
}
/*Note:we get hfile name form libfile name,not from path*/
LOCAL int bcloader_get_hfile_name(char *path,char *name)
{
    int ret,index;
    char *start,*end;
	char libfile_name[128],hfile_name[128];

	if((path == NULL) || (name == NULL)) return -1;

	index = last_index_of(path,'/');

	ret = bcloader_get_libfile_name(path,libfile_name,sizeof(libfile_name));
	if(ret == -1) return -1;

	start = libfile_name;
	end   = libfile_name + strlen(libfile_name)-2;

	if(memcmp(libfile_name,"lib",3) == 0){
		start += 3;
	}
	//hfile ends with .h,so add 1 more byte
	if((end-start+1)>sizeof(hfile_name)) return -1;

    memset(hfile_name,0,sizeof(hfile_name));
	memcpy(hfile_name,start,end-start);
	hfile_name[end-start] = 'h';

	memcpy(name,path,index);
	memcpy(name+index,hfile_name,strlen(hfile_name));
	return 0;
}

LOCAL int bcloader_get_funcs_name(char *path,func_array_t *funcs)
{
    FILE *fhead;
	char line[1024],fun_name[1024];
	char *name;
	int line_len;
	
    if((funcs == NULL)||(funcs->cnt != 0)||
		(funcs->array == NULL)) return FM_BCLOADER_INVALID_PARM;

    fhead = fopen(path, "r");
    if(fhead == NULL){
    	//printf("Open h file faile\n");
    	return FM_SYS_ERR_OPEN_FILE_FAIL;
    }

    while(fgets(line,sizeof(line),fhead) != NULL){

    	if(line[0] == '\n') continue;
		if(line[0] == '#'){
			if(strstr(line,"BC_BASE") != NULL){
				sscanf(line,"%* ",fun_name);
				str_trim(fun_name);
				funcs->base = atoi(fun_name);
			}
			continue;
		}
		
    	line_len = strlen(line);
    	if(line[line_len - 1] == '\n'){
    		line[line_len - 1] = '\0';
    	}
		
    	memset(fun_name,0,sizeof(fun_name));
    	sscanf(line,"%*[^ ]%[^(]",fun_name);
    	str_trim(fun_name);
		name = (char *)fm_calloc(strlen(fun_name),sizeof(char));
		strcpy(name,fun_name);
		func_array_add_entry(funcs,name);
    }

	return 0;
}

/*path should comply with linux convention,that is,begin with "lib",end with ".so",
e.g /home/hzh/tsm/dlls/libtsm_bc_v1.so, and the header file is assume to be under 
the same directory with so-file.
newly loaded dll will be add in the cache list,named bc_list
*/
PUBLIC bc_set_t *bcloader_load(char *path,list_t *bc_list)
{
    int i,j,ret;
	char libfile_name[128],hfile[512];
	char *error,*name;
	void *dl;
    tsm_bytecode *func;
	func_array_t *fa = NULL;
	bc_set_t *bc_set,*res;

    ret = bcloader_get_libfile_name(path,libfile_name,sizeof(libfile_name));
	if(ret != 0) return NULL;
	ret = bcloader_get_hfile_name(libfile_name,hfile);
	if(ret != 0) return NULL;

    fa = func_array_alloc(DEF_BYCODE_SIZE);
	if(fa == NULL) return NULL;

	ret = bcloader_get_funcs_name(hfile,fa);
	if(ret != 0) {
		res = NULL;
		goto err_handling;
	}

	bc_set = byte_code_set_alloc(libfile_name,fa->base,fa->cnt);

    dl = dlopen(path, RTLD_NOW);
    if(dl == NULL)
    {
        //printf("Failed load libary\n");
        error = dlerror();
    	//printf("%s\n", error);
    	res = NULL;
    	goto err_handling;
    }
	
	for(i = 0; i < fa->cnt; i++){
		name = fa->array[i];
    	func = dlsym(dl, name);
    	error = dlerror();
    	if(error != NULL){
    	    //printf("%s\n", error);
    	    res =  NULL;
		    goto err_handling;
    	}

		bc_set->bc[i] = func;
		
	}
	list_add_entry(bc_list,&bc_set->entry);
	return bc_set;
	
	err_handling:
	if(fa != NULL){
		func_array_free(fa);
	}
	if(bc_set != NULL){
		byte_code_set_free(bc_set);
	}
	if(dl != NULL){
		dlclose(dl);
	}
	return res;
}