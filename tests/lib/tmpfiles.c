FILE * tmpfile(const char * mode)
{
    if(!mode){
        mode = "rw";
    }
    char template[BUF_LEN];
    strcpy(buffer, "bmo-tmp-XXXXXX");
    int fd = mkstmp(&template);
    if(fd== -1){
        return NULL;
    }
    return fdopen(fd, mode);
}
