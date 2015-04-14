int caploader_load(cap_info *cap)
{
    struct zip *cap_zip;
	struct zip_stat state;
	struct zip_file *file;
	int errorp;
	int subcap_cnt,index,totol_len = 0,sub_cap_size;
	int i,j;
	char *sub_cap_name[] = {
		"Header.cap",
		"Directory.cap",
		"Import.cap",
		"Applet.cap",
		"Class.cap",
		"Method.cap",
		"StaticField.cap",
		//"Export.cap",
		"ConstantPool.cap",
		"RefLocation.cap",
		//"Descriptor.cap",
	};
	subcap_cnt = sizeof(sub_cap_name)/sizeof(sub_cap_name[0]);

    cap->content.buffer = (u8 *)fmsh_malloc(36*1024);
	cap->content.bufSize=36*1024;
	cap->content.dataLen=0;

	cap_zip = zip_open(cap->path, 0, &errorp);
	if(cap_zip == NULL){
		FM_LOGE("zip_open failed,err=%d",errorp);
		return FM_ZIP_FILE_OPEN_FAILED;
	}

    for(i = 0; i < subcap_cnt; i++){
		index = zip_name_locate(cap_zip, sub_cap_name[i],ZIP_FL_NODIR|ZIP_FL_NOCASE);
		if(index == -1){
			FM_LOGE("no entry match with %s",sub_cap_name[i]);
			return FM_ZIP_FILE_NO_ENTRY_MATCH;
		}
		
		if(zip_stat_index(cap_zip,index,0,&state) == -1){
			FM_LOGE("can't stat %s",sub_cap_name[i]);
			return FM_ZIP_FILE_STATE_FAILED;
		}

		if(state.valid & ZIP_STAT_SIZE){
			totol_len += state.size;
		}else{
		    FM_LOGE("cap %s size is not available",sub_cap_name[i]);
			return FM_ZIP_FILE_SIZE_NOT_AVAIL;
		}

		file = zip_fopen_index(cap_zip,index, ZIP_FL_UNCHANGED);
		if(file == NULL){
		    FM_LOGE("can't open entry %s",sub_cap_name[i]);
			return FM_ZIP_FILE_ENTRY_NOT_OPEN;
		}

	    /*if buff is not big enough,resize*/
        if((cap->content.bufSize - cap->content.dataLen) < state.size){
			cap->content.bufSize += (state.size/4+1)*1024;    /*in 4k unit*/
			u8 *temp;
			temp = fmsh_malloc(cap->content.bufSize);
			memcpy(temp,cap->content.buffer,cap->content.dataLen);
			fmsh_free(cap->content.buffer);
			cap->content.buffer = temp;
        }
		zip_fread(file,cap->content.buffer+cap->content.dataLen,state.size);
		
        /*get pkg-aid from header.cap*/
		if(i == 0){
			u8 aid_len;
			aid_len = cap->content.buffer[12];
			cap->pkg_aid.buffer = (u8 *)fmsh_malloc(aid_len);
			cap->pkg_aid.bufSize = aid_len;
			memcpy(cap->pkg_aid.buffer,cap->content.buffer+13,aid_len);
			cap->pkg_aid.dataLen = aid_len;
			FM_LOGD("packet aid=%s",HexStr(cap->pkg_aid.buffer,cap->pkg_aid.dataLen));
		}

		/*get applet-aid from Applet.cap*/
		if(i == 3){
			u8 app_cnt;
			u8 adi_len = 0;
			u32 offset,pos;

			app_cnt = *(cap->content.buffer+cap->content.dataLen + 3);
			
			offset = 4;
			for(j = 0; j < app_cnt; j++){
				adi_len += *(cap->content.buffer+cap->content.dataLen + offset);
				offset += adi_len+3;
			}
			cap->applet_aid.buffer= fmsh_malloc(adi_len);
			offset = 4;
			pos = 0;
			for(j = 0; j < app_cnt; j++){
				adi_len = *(cap->content.buffer+cap->content.dataLen + offset);
				memcpy(cap->applet_aid.buffer+pos,cap->content.buffer+cap->content.dataLen + offset+1,adi_len);
				offset += adi_len+3;
				pos += adi_len;
			}
			cap->applet_aid.dataLen = adi_len;
			FM_LOGD("applet aid:%s",HexStr(cap->applet_aid.buffer,cap->applet_aid.dataLen));
		}

		cap->content.dataLen += state.size;

		zip_fclose(file);
    }


	HASH(&cap->content,&cap->hash,0,0);
	FM_LOGD("cap size=%d",cap->content.dataLen);
	FM_LOGD("Cap hash:%s",HexStr(cap->hash.buffer,cap->hash.dataLen));

	zip_close(cap_zip);
	return FM_OK;
}