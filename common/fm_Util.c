#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h> 

#include "fm_Base.h"
#include "fm_Log.h"
#include "fm_Bytes.h"
#include "fm_Util.h"


/*find sub_str in str,if found,return the start of the addr in the str,
  if not found, return NULL*/
char *fm_strstr(u8 *str,int len, u8 *sub_str,int sub_len)
{  
    int tem,i, j = 0;  

	//FM_LOGD("str len=%d,%s",len,HexStr(str,len));
	//FM_LOGD("sub_str len=%d,%s",sub_len,HexStr(sub_str,sub_len));

    for(i = 0; i<len; i++)  
    {  
	j = 0;
	tem = i;
        while(str[i++] == sub_str[j++])  
        {  
            if(j == sub_len)  
            {  
                return &str[tem];  
            }  
        }  
        i = tem;  
    }  
  
    return NULL;  
}  

/*Returns the index within this string of the last occurrence of the 
specified character,-1 if the character does not occur*/
int last_index_of(char *str,char delimiter)
{
    int i,len;
	char *start,*end,*pchar;

   	len = strlen(str);
	start = str;
    end = str + len;
    for(pchar = end; pchar != start; pchar--){
		if(*pchar == delimiter){
			return (pchar - start);
		}
    }
	return -1;
}
/*trim all space in the str*/
void str_trim(char *pStr)  
{  
    char *pTmp = pStr;  
      
    while (*pStr != '\0')   
    {  
        if (*pStr != ' ')  
        {  
            *pTmp++ = *pStr;  
        }  
        ++pStr;  
    }  
    *pTmp = '\0';  
}
/*trim all space/tab//n both in the left and right*/
void trim_lr(char *ptr)
{
    char *p,*q;
    if(ptr==NULL)
        return;
    for(p=ptr; *p==' ' || *p=='\t'|| *p=='\n' ; ++p);
    if( *p==0 )
    {
        *ptr=0;
        return;
    }
    for(q=ptr; *p; ++p,++q)
    {
        *q=*p;
    }
    for(p=q-1; *p==' '||*p=='\t'||*p=='\n'; --p);
    *(++p)='\0';
}
int sub_str(char *str,int start,int len,char *sub_str)
{
    int length;

	length = strlen(str);
	if((start + len)>length) return -1;

	memcpy(sub_str,str+start,len);
	return 0;
}

int str_2_hexstr(char *in,char *out)
{
    int i,in_len = strlen(in);

	for(i = 0; i < in_len; i+=2){
		out[i/2] = ((in[i]-0x30)<<4)|(in[i+1]-0x30);
	}
	return 0;
}

int set_nonblocking(int fd)
{
    int op;
 
    op=fcntl(fd,F_GETFL,0);
    fcntl(fd,F_SETFL,op|O_NONBLOCK);
 
    return op;
}
void clear_buffer(int fd)
{
    u8 temp_buf[100];
    while(read(fd,temp_buf,100) != 0){
    }
}

int init_thread_pool(int num,void *(*func)(void *))
{
    int i,ret;
	pthread_t threadId;

	for(i = 0; i < num; i ++){
		ret = pthread_create(&threadId, 0, func, (void *)0);
		if(ret != 0){
			FM_LOGE("Create thread pool failed!");
			return -1;
		}
	}
	return 0;
}
/*
 * make des = len + conten, and return len+1
*/
int fmbytes_concat_with_len(u8 *des,fmBytes *src)
{
    if(!src){
		des[0] = 0;
    }else{
        des[0] = fmBytes_get_length(src);
    	memcpy(des+1,fmBytes_get_buf(src),fmBytes_get_length(src));
    }

	return des[0]+1;
}

int fmbytes_concat_with_nlen(u8 *des,u8 len_size,fmBytes *src)
{
    int i;
	int len = fmBytes_get_length(src);

	for(i = 0; i < len_size; i++){
		des[i] = (len>>((len_size-i-1)*8))&0xFF;
	}

    memcpy(des+len_size,fmBytes_get_buf(src),len);
	return (len+len_size);
}

int get_BERLen(u32 len,u8 *berlen)
{
    u8 i,LenFirst;
	u32 length;
    if(len < 128){
		berlen[0] = len;
		LenFirst =  0;
    }else{
        LenFirst = 0x00;
		length = len;
		do{
			length = length >> 8;
			LenFirst++;
		}while(length != 0);

		berlen[0] = LenFirst+128;
		length = len;
		for(i = LenFirst; i > 0; i--){
		    berlen[i] = length & 0xFF;
			length = length >> 8;
		}
    }
	return (LenFirst+1);
}

fmBool is_all_zero(u8 *buf,int len)
{
    int i;

	for(i = 0;i < len; i++){
		if(buf[i] != 0)
			return fm_false;
	}
	return fm_true;
}
int int2byteBE(int v,u8 *buf)
{
    buf[0] = (v>>24)&0xFF;
	buf[1] = (v>>16)&0xFF;
	buf[2] = (v>>8)&0xFF;
	buf[3] = (v)&0xFF;
	return 4;
}

int hexstr_to_hex(fmBytes *hexstr,fmBytes *out)
{
    int i,len_in,size_out;
    u8 *ibuf,*obuf;
	
	len_in   = fmBytes_get_length(hexstr);
	size_out = fmBytes_get_size(out);
	ibuf     = fmBytes_get_buf(hexstr);
	obuf     = fmBytes_get_buf(out);

	if((!hexstr)||(!out)||(size_out<(len_in/2))) return -1;

	for(i = 0; i < len_in; i+=2){
		obuf[i/2] = ((ibuf[i]-0x30)<<4)|(ibuf[i+1]-0x30);
	}
	fmBytes_set_length(out,len_in/2);
	return 0;
}
void memset_int(void *mem,int val,int len)
{
    int i;

	for(i = 0; i < len; i++){
		*(int *)(mem+i) = val;
	}
}
void memcpy_int(void *src,void *des,int len)
{
    int i;

	for(i = 0; i < len; i++){
		*(int *)(src+i) = *(int *)(des+i);
	}
}