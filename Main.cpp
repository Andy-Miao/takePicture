
#define LOG_NDEBUG 0
#define LOG_TAG "TakePicture"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>
#include <utils/BluetoothPipeUtil.h>
#include <monitor/ProcessMonitor.h>
// #include <Led.h>
#include "TakePicture.h"
#include "UploadImageAI.h"
using namespace android;

static bool previewing = false;
void preview_thread_stop(int sig)
{
    previewing = false;
}

int main(int argc, char** argv)
{
    ALOGV("%s", __FUNCTION__);

      /*take picture must need a continuous space */
    system("echo 3 > /proc/sys/vm/drop_caches");

    TakePicture app;
    setProcessStatus(STATUS_TAKING_PIC);
    if (app.start() == -1) {
	ALOGE("start error");
	app.writePipe(PIPE_BUF_CAPTURE_ERROR);
	return -1;
    }
    // android::Led *led = android::Led::getInstance();
    // led->TurnRedLightOff();
    // led->TurnRedLightBlinkOff();
    previewing = true;
    // Register Signal function
    if (SIG_ERR == signal(SIGINT, preview_thread_stop)) {
	ALOGE("ERROR: signal()\n");
	return 0;
    }
    while (previewing) {
	usleep(100 * 1000);
    }
    app.stop();
    // led->TurnRedLightOn();
    setProcessStatus(STATUS_INVALID);
    // 上传图片
    UploadImageAI uploadImageAI;
        	int ack_len = 256;
        	char ack_json[256]={0};
            int ret = 0;
            ALOGV("gyx--------------cur img path:%s", app.mFileName.c_str());
            ret = uploadImageAI.http_post_upload_pic(SERVER_ADDR, SERVER_PORT,SERVER_URL,app.mFileName.c_str(),ack_json,ack_len); //Post方式上传图片

            if(ret == -1){
        				//printf("\n\n----------- Post picture Fail!!\n");
            	ALOGV("gyx--------------Post picture Fail!!");
        	}else{
        		ALOGV("gyx--------------Post picture ok!");
        	}
    int pid = getpid();
    removeProcess(pid);
    ALOGV("main out");
    return 0;
}
