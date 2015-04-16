#include <stdio.h>
#include <dlfcn.h>
#include "./common/fm_Base.h"
#include "./common/fm_Bytes.h"
#include "tsm_ccb.h"

int main(int argc,char *argv[])
{
    void *dl = NULL;
	tsm_bytecode *func;
	char *error;
	
    dl = dlopen("libbc_data.so", RTLD_NOW);

    if(dl == NULL)
    {
        error = dlerror();
		printf("dlopen err %s\n",error);
		return -1;
    }

	func = dlsym(dl, "data_set");
	printf("data_set=%p\n",func);
	
    return 0;
}
