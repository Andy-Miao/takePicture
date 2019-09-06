//============================================================================
// Name        : UploadImage.cpp
// Author      : guoyx
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================
#define LOG_NDEBUG 0
#define LOG_TAG "UploadImageAI"
#include <iostream>

#include <sys/types.h>

#include <string.h>
#include <sys/stat.h>
#include "UploadImageAI.h"
#include <stdio.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "cJSON.h"
#include <cstdlib>
#include <utils/Log.h>



namespace android {
char header[1024]={0};
char send_request[1024]={0};
 char send_end[1024]={0};
 char http_boundary[64]={0};

unsigned long UploadImageAI::get_file_size(const char *path)  //获取文件大小
{
    unsigned long filesize = -1;
    struct stat statbuff;
    if(stat(path, &statbuff) < 0){
        return filesize;
    }else{
        filesize = statbuff.st_size;
    }
    return filesize;
}

//  解析json

void UploadImageAI::parseJson(char line[]){
	//char *out;
	//char line[1024] = {0};
	cJSON  *json;
	json=cJSON_Parse(line); //获取整个大的句柄
	//out=cJSON_Print(json);  //这个是可以输出的。为获取的整个json的值
	cJSON *code = cJSON_GetObjectItem(json,"code");
	//printf("code:%d\n",code->valueint);
	ALOGV("gyx--------------img json code:%d", code->valueint);
	cJSON *data = cJSON_GetObjectItem(json,"data");

	cJSON *txt = cJSON_GetObjectItem(data,"txt");
		//printf("txt:%s\n",txt->valuestring);
	ALOGV("gyx--------------img txt code:%s", txt->valuestring);

	cJSON *url = cJSON_GetObjectItem(data,"url");
		//printf("url:%s\n",url->valuestring);
	ALOGV("gyx--------------img url code:%s", url->valuestring);
	cJSON *fileName = cJSON_GetObjectItem(data,"fileName");
			//printf("fileName:%s\n",fileName->valuestring);
	// 播放 在线WAV
	char playMsc[100];
	strcpy(playMsc,"http://server.sycglass.com");
	strcat(playMsc,url->valuestring);
	//char* play = "/usr/bin/MediaPlayer ";
	ALOGV("gyx------------download wav url :%s", playMsc);

	string folderPath = "/storage/music";

//	    if (0 != access(folderPath.c_str(), 0))
//	    {
//	        // if this folder not exist, create a new one.
//	        mkdir(folderPath.c_str());   // 返回 0 表示创建成功，-1 表示失败
//	        //换成 ::_mkdir  ::_access 也行，不知道什么意思
//	    }
	    if(access("/storage/music",F_OK) != 0){

	    	 string command;
	    	 command = "mkdir -p " + folderPath;
	    	 system(command.c_str());

	    }else{
	    	// 清空文件夹里边的内容
	    }
	    char downUrl[100];
	    strcpy(downUrl,"wget -P ");
	    strcat(downUrl,folderPath.c_str());
	    strcat(downUrl," ");
	    strcat(downUrl,playMsc);
	    ALOGV("gyx------------download command  url :%s", downUrl);
	    system(downUrl);

	    char playLocalMsc[100];
	    strcpy(playLocalMsc,"/usr/bin/MediaPlayer ");
	    strcat(playLocalMsc,folderPath.c_str());
	    strcat(playLocalMsc,"/");
	    strcat(playLocalMsc,fileName->valuestring);
	    ALOGV("gyx------------local img path:%s", playLocalMsc);

	    system(playLocalMsc);

	// todo 下载到文件夹播放 播放完后统一删除？

}
int UploadImageAI::http_post_upload_pic(const  char *IP,  unsigned int port, const char *URL,  const  char *filepath,
									char *ack_json, int ack_len) //Post方式上传图片
{
	ALOGV("gyx--------------http_post_upload_pic!!");
	int cfd = -1;
    int recbytes = -1;
    int sin_size = -1;
	char buffer[1024*10]={0};
    struct sockaddr_in s_add,c_add;

	cfd = socket(AF_INET, SOCK_STREAM, 0);  //创建socket套接字
    if(-1 == cfd)
    {
        //printf("socket fail ! \r\n");
        ALOGV("gyx--------------socket fail ! \r\n!!");
        return -1;
    }

	bzero(&s_add,sizeof(struct sockaddr_in));
    s_add.sin_family=AF_INET; //IPV4
    s_add.sin_addr.s_addr= inet_addr(IP);
    s_add.sin_port=htons(port);
    //printf("s_addr = %#x ,port : %#x\r\n",s_add.sin_addr.s_addr,s_add.sin_port);

	if(-1 == connect(cfd,(struct sockaddr *)(&s_add), sizeof(struct sockaddr)))  //建立TCP连接
    {
        //printf("connect fail !\r\n");
		ALOGV("gyx--------------tcp connect fail !!");
        return -1;
    }

	//获取毫秒级的时间戳用于boundary的值
	long long int timestamp;
	struct timeval tv;
	gettimeofday(&tv,NULL);
	timestamp = (long long int)tv.tv_sec * 1000 + tv.tv_usec;
	snprintf(http_boundary,64,"---------------------------%lld",timestamp);

	unsigned long totalsize = 0;
	unsigned long filesize = get_file_size(filepath); //文件大小
	unsigned long request_len = snprintf(send_request,1024,UPLOAD_REQUEST,http_boundary,filepath); //请求信息
	unsigned long end_len = snprintf(send_end,1024,"\r\n--%s--\r\n",http_boundary); //结束信息
	totalsize = filesize + request_len + end_len;

	unsigned long head_len = snprintf(header,1024,HTTP_HEAD,SERVER_PATH,URL,http_boundary,totalsize); //头信息
	totalsize += head_len;

    char* request = (char*)malloc(totalsize);	//申请内存用于存放要发送的数据
    if (request == NULL){
        //printf("malloc request fail !\r\n");
    	ALOGV("gyx--------------malloc request fail !!");
        return -1;
    }
    request[0] = '\0';

	/******* 拼接http字节流信息 *********/
    strcat(request,header);  									//http头信息
    strcat(request,send_request);    							//文件图片请求信息
	FILE* fp = fopen(filepath, "rb+");							//打开要上传的图片
    if (fp == NULL){
        //printf("open file fail!\r\n");
    	ALOGV("gyx--------------open file fail!");
        return -1;
    }
	int readbyte = fread(request+head_len+request_len, 1, filesize, fp);//读取上传的图片信息
    if(readbyte < 1024) //小于1024个字节 则认为图片有问题
	{
		//printf("Read picture data fail!\r\n");
    	ALOGV("gyx--------------Read picture data fail!");
        return -1;
	}
    memcpy(request+head_len+request_len+filesize,send_end,end_len);  //http结束信息

	/*********  发送http 请求 ***********/
	if(-1 == write(cfd,request,totalsize))
    {
        //printf("send http package fail!\r\n");
		ALOGV("gyx--------------send http package fail!");
        return -1;
    }

	/*********  接受http post 回复的json信息 ***********/
	if(-1 == (recbytes = read(cfd,buffer,102400)))
    {
        //printf("read http ACK fail !\r\n");
		ALOGV("gyx--------------read http ACK fail!");
        return -1;
    }

	//printf("back data  %s",buffer);
	ALOGV("gyx--------------back data  :%s", buffer);
   	buffer[recbytes]='\0';
	int index = 0,start_flag = 0;
	int ack_json_len = 0;
	for(index = 0; index<recbytes; index++)
	{
		if(buffer[index] == '{')
		{
			start_flag = 1;
		}
		if(start_flag)
			ack_json[ack_json_len++] = buffer[index];  //遇到左大括号则开始拷贝

		if(index == recbytes-1)		//遇到右大括号则停止拷贝
		{
			ack_json[ack_json_len] = '\0';
			break;
		}
	}
	if(ack_json_len > 0 && ack_json[ack_json_len-1] == '}')  //遇到花括号且有json字符串
	{
		//printf("Receive:%s\n",ack_json);
		ALOGV("gyx--------------img ack_json:%s", ack_json);
		// 解析json
		parseJson(ack_json);
	}else{
		ack_json[0] = '\0';
		//printf("Receive http ACK fail!!\n %s",ack_json);
		//printf("--- ack_json_len = %d\n",ack_json_len);
		ALOGV("gyx--------------last  Receive http ACK fail!%s",ack_json);
	}

    free(request);
    fclose(fp);
    close(cfd);
	return 0;
  }
};

//int main(int argc, char *argv[]){
//	//cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!
//	int ack_len = 256;
//		char ack_json[256]={0};
//		int ret = http_post_upload_pic(SERVER_ADDR, SERVER_PORT,SERVER_URL,"bc.jpg",ack_json,ack_len); //Post方式上传图片
//		if(ret == -1)
//		{
//			printf("\n\n----------- Post picture Fail!!\n");
//		}
//
//	return 0;
//}
