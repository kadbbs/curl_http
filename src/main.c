#include <stddef.h>
#include <stdio.h>
#include <curl/curl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>



double filelen;


double getfilelength(const char *url){

    double len=0;
    CURL *curl =curl_easy_init();
    curl_easy_setopt(curl,CURLOPT_URL,url);
    curl_easy_setopt(curl,CURLOPT_HEADER,1);

    curl_easy_setopt(curl,CURLOPT_NOBODY,1);



    CURLcode res=curl_easy_perform(curl);
    if(res==CURLE_OK){
        curl_easy_getinfo(curl,CURLINFO_CONTENT_LENGTH_DOWNLOAD, &len);
    }
    curl_easy_cleanup(curl);
    return len;

}
int offset;
void show_schedule(double schedule){

    int sum=100;
    char ch[103];
    memset(ch, 0, 103);
    ch[0]='[';
    ch[40]=']';
    int num=schedule*sum;
    memset(&ch[1],'#',num);
   // printf("\rdownload schedule %.3f%%",schedule*100);
    printf("download : %s\r",ch);

    fflush(stdout);
}
size_t writeFunc(void *ptr,size_t size,size_t memb,void *userdata){

    //printf("size%ldmenb%ld",size,memb);
    memcpy((char *)userdata+offset,ptr,size*memb);
    offset+=size*memb;
    double schedule=(double)offset/filelen;
    
    show_schedule(schedule);


    printf("memb%ld",memb);
    return size*memb;
}


int download(const char *url,const char *filename){




    int fd=open(filename,O_RDWR|O_CREAT,0660);
    if(fd<0){
        perror("fd");
        exit(1);
    }

    double filelen=getfilelength(url);
    printf("filelen=%f",filelen);
    lseek(fd,filelen-1,SEEK_SET);


    write(fd,"",1);

    char *ptr=(char *)mmap(NULL,filelen,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    if(ptr==MAP_FAILED){
        perror("mmap");
        exit(1);
    }


    CURL *curl =curl_easy_init();

    
    curl_easy_setopt(curl,CURLOPT_URL,url);
    curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,writeFunc);
    curl_easy_setopt(curl,CURLOPT_WRITEDATA,ptr);

    CURLcode res=curl_easy_perform(curl);
    if(res==CURLE_OK){
        perror("res");
        exit(1);
    }


    curl_easy_cleanup(curl);


    munmap(ptr,filelen);
    close(fd);



    return 0;
}




char *getfilename(char *url,int size){


    for(int i=size-1;i>=0;i--){

        if(url[i]=='/')
            return &url[i+1];
    }
    
    return NULL;
}



int main (int argc, char *argv[])
{

    char *filename;
    if(argc==2){
        filename=getfilename(argv[1], strlen(argv[1]));
        if(filename==NULL){
            perror("filename error");
            exit(1);
        }
    }else if(argc==3){
        filename=argv[2];
    }else{
        printf("arg error\n");
    }

    filelen=getfilelength(argv[1]);
    offset=0;
    
    download(argv[1], filename);
    




    return 0;
}
