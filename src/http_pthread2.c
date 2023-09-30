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

#include <pthread.h>

#define PTHREAD_NUM         10

struct fileinfo{
    int id;
    char *ptr;
    long offset;//每一段起始位置
    long end;//每一段终止位置
    double data_sum;
    // pthread_t pid;//线程
};

char *url;
long filelen;
long filepart;

//一个线程中下载文件中的每次的偏移量
//int offset;



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


void show_schedule(double schedule){

    int sum=100;
    char ch[103];
    memset(ch, 0, 103);
    ch[0]='[';
    ch[40]=']';
    int num=schedule*sum;
    memset(&ch[1],'#',num);
   // printf("\rdownload schedule %.3f%%",schedule*100);
    printf("download : %s\n",ch);

    fflush(stdout);
}



size_t writeFunc(void *ptr,size_t size,size_t memb,void *userdata){

    struct fileinfo *info =(struct fileinfo*)userdata;

    // printf("info->id%d",info->id);
    // printf("info->offset-info->end=%ld-%ld\n",info->offset,info->end);
    
    //printf("size%ldmenb%ld",size,memb);
    memcpy(info->ptr+info->offset,ptr,size*memb);
    info->offset+=size*memb;
    info->data_sum+=size*memb;
    double schedule=info->data_sum/(filelen/PTHREAD_NUM);
    // printf("id=%d",info->id);
    // // show_schedule(schedule);


    // printf("schedule:%f\n",schedule);

    printf("id:%d  writeFun:%ld\n",info->id,size*memb);
    //printf("memb%ld",memb);
    return size*memb;
}

char *getfilename(char *url,int size){


    for(int i=size-1;i>=0;i--){

        if(url[i]=='/')
            return &url[i+1];
    }
    
    return NULL;
}

void *work(void *arg){

    struct fileinfo *info=(struct fileinfo*)arg;
    // info->pid=getpid();
    // printf("pid=%ld\n",info->pid);
    printf("id=%d\n",info->id);
    char range[64]={0};
    snprintf(range,64,"%ld-%ld",info->offset,info->end);
    CURL *curl =curl_easy_init();
    printf("range=%s\n",range);

    
    curl_easy_setopt(curl,CURLOPT_URL,url);
    curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,writeFunc);
    curl_easy_setopt(curl,CURLOPT_WRITEDATA,info);
    // curl_easy_setopt(curl,CURLOPT_WRITEDATA,ptr+info->offset);

    curl_easy_setopt(curl,CURLOPT_RANGE,range);

    CURLcode res=curl_easy_perform(curl);
    if(res==CURLE_OK){
        // perror("%d res",info->id);
        printf("%d pthread Success!\n",info->id);
        
    }


    curl_easy_cleanup(curl);



}



int download(const char *url,const char *filename){

    struct fileinfo *info[PTHREAD_NUM];




    int fd=open(filename,O_RDWR|O_CREAT,0660);
    if(fd<0){
        perror("fd");
        exit(1);
    }
 
    //printf("filelen=%ld",filelen);
    lseek(fd,filelen-1,SEEK_SET);
    write(fd,"",1);




    char *ptr=(char *)mmap(NULL,filelen,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    if(ptr==MAP_FAILED){
        perror("mmap");
        exit(1);
    }

    pthread_t pd[PTHREAD_NUM];

    filepart=filelen/PTHREAD_NUM;

    //创建每一段的文件属性
    for(int i=0;i<PTHREAD_NUM;i++){
        
        info[i]=malloc(sizeof(struct fileinfo));
        
        // info[i]->offset=i*filepart;
        info[i]->id=i;
        info[i]->offset=i*filepart;
        info[i]->ptr=ptr;
        info[i]->data_sum=0;
        if(i==PTHREAD_NUM-1){
            info[i]->end=filelen-1;
        }else{
            info[i]->end=info[i]->offset+filepart-1;
        }

    }
    
    //创建线程
    for(int i=0;i<PTHREAD_NUM;i++){
        
        pthread_create(&pd[i],NULL,work,info[i]);


    }

    for(int i=0;i<PTHREAD_NUM;i++){
        
        pthread_join(pd[i],NULL);

    }



    // pthread_mutex_lock(&mutex);
    // int fd=open(filename,O_RDWR|O_CREAT,0660);
    // if(fd<0){
    //     perror("fd");
    //     exit(1);
    // }
    // pthread_mutex_unlock(&mutex);
    // double filelen=getfilelength(url);
    // printf("filelen=%f",filelen);
    // lseek(fd,filelen-1,SEEK_SET);


    // write(fd,"",1);

    // char *ptr=(char *)mmap(NULL,filelen,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    // if(ptr==MAP_FAILED){
    //     perror("mmap");
    //     exit(1);
    // }

    munmap(ptr,filelen);
    close(fd);

    return 0;
}





int main(int argc, char const *argv[])
{

//初始化
    
    filelen=getfilelength(argv[1]);
    curl_global_init(CURL_GLOBAL_ALL);

    url=(char *)argv[1];
//判断命令行参数
    char *filename=NULL;
    if(argc==2){
        filename=getfilename(url, strlen(argv[1]));
        if(filename==NULL){
            perror("filename error");
            exit(1);
        }
    }else if(argc==3){
        filename=(char *)argv[2];
    }else{
        printf("arg error\n");
    }



    download(url,filename);



    return 0;
}
