
#define LOG_NDEBUG 0
#define LOG_TAG "TakePicture"

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <utils/Log.h>
#include <utils/DiskSpaceUtil.h>
#include <binder/IPCThreadState.h>
#include <SettingsDB.h>
#include <utils/BluetoothPipeUtil.h>
#include "TakePicture.h"
#include <PowerManager.h>

#define DUMP_DEBUG 0

#define PHOTO_NAME_PREFIX "/PIC_"

namespace android {
#define APP_NAME "com.ingenic.glass.camera"
#define SOUND_CAMERA_CLICK "/usr/bin/MediaPlayer /etc/sound/camera_wav.wav"// add syc 20190711 去掉&
//#define SOUND_CAMERA_CLICK "/usr/bin/MediaPlayer /etc/sound/camera_click.mp3 &" syc 20190711
#define SOUND_LOW_STORAGE "/usr/bin/MediaPlayer /etc/sound/noSpace.mp3 &"
TakePicture::TakePicture()
	: mNumberOfCameras(0),
	  mCameraDevice(NULL),
	  mPreviewFrameCount(0),
	  mHardStore(false),
	  mHardStoreSuccess(true)
{
    ALOGV("--%s, %p", __FUNCTION__, this);
    PowerManager::LockSleep();
}

status_t TakePicture::start()
{
    int diskReserveMB = 0;
    diskReserveMB = SettingsDB::getInt(DB_KEY_DISK_RESERVE_MB);
    if(DiskSpaceUtil::dataDiskFull(diskReserveMB)){
	system(SOUND_LOW_STORAGE);
	ALOGE("no enough space!!");
	return -1;
    }

    system(SOUND_CAMERA_CLICK); //syc 20190711

    String16 clientName(APP_NAME, strlen(APP_NAME));
    int pictureW = 0, pictureH = 0;
    int pictureWatermark = 0;
    status_t res = 0;
    bool isValid = false;

    SettingsDB::getInt(DB_KEY_PICTURE_WIDTH, pictureW);
    SettingsDB::getInt(DB_KEY_PICTURE_HEIGHT, pictureH);
    SettingsDB::getInt(DB_KEY_WATERMARK_PICTURE, pictureWatermark);

    mNumberOfCameras = Camera::getNumberOfCameras();
    if(mNumberOfCameras == 0){
	ALOGE("can not get camera device!");
	return -1;
    }
    struct CameraInfo cameraInfo;
    for (int i = 0; i < mNumberOfCameras; i++) {
    	Camera::getCameraInfo(i, &cameraInfo);
    	if (cameraInfo.facing == CAMERA_FACING_BACK) {
	    mCameraDevice = Camera::connect(i, clientName, Camera::USE_CALLING_UID);
    	}
    }
    if (mCameraDevice == NULL) {
	ALOGE("Fail to connect to camera service");
	return -1;	
    }
    // make sure camera hardware is alive
    if (mCameraDevice->getStatus() != NO_ERROR) {
	ALOGE("Camera initialization failed");
        return -1;
    }
    mCameraDevice->setListener(this);
    mParameters.unflatten(mCameraDevice->getParameters());
    mParameters.getSupportedPictureSizes(mSupportedPictureSizes);
ALOGE("getSupportedPictureSizes pictureW is %d pictureH is:%d\n",pictureW,pictureH);

	for (Vector<Size>::iterator it = mSupportedPictureSizes.begin(); 
    	 it != mSupportedPictureSizes.end(); it++) {
    	if (it->width == pictureW && it->height == pictureH) {
    	    isValid = true;
    	}
    }
    if (isValid) {
    	mParameters.setPictureSize(pictureW, pictureH);
    } else {
    	ALOGW("Warning : picture size is invalid, please refence to /etc/camera.conf");
    }
    /* JPEG Hal Store */
    createFileName(mFileName);
    mParameters.set("jpeg_halstore_path", mFileName.c_str());
    mHardStore = true;
    /*
     * Picture and Video Mark:
     * 1. Picture
     *      mParameters.set("pic_mark_time", "false");
     *           参数："true", "flase"  
     *           功能：设置是否添加时间水印，格式为：2015-12-20 18:45:29
     *      mParameters.set("pic_mark_user", "null");
     *           参数：字符串，例如："MyGlass010"
     *           功能：设置用户自定义字符串，主要为添加蓝牙序列
     *      mParameters.set("pic_mark_location", "left-up");
     *           参数："left-up", "left-down", "right-up", "right-down"
     *           功能：设置时间水印的位置，如左上角，左下角，右上角，右下角
     *      mParameters.set("pic_mark_size", "middle");
     *           参数："big", "middle", "small"
     *           功能：设置水印的大小，目前提供3中选项：大，中，小
     *      mParameters.set("pic_mark_rows", "1");
     *           参数："1", "2"
     *           功能：设置一共有几行水印，可以分两行显示，也可以只显示在一行
     * 2. Video
     *      参数和功能和照片类似
     */
    if(pictureWatermark) {
	    mParameters.set("pic_mark_time", "true");
	    mParameters.set("pic_mark_location", "right-down");
	    mParameters.set("pic_mark_size", "small");
	    mParameters.set("pic_mark_rows", "1");
    }else {
	    mParameters.set("pic_mark_time", "false");
    }

    res = mCameraDevice->setParameters(mParameters.flatten());
    if (res != NO_ERROR) {
	ALOGE("Err : Set parameters failed!!");
	return -1;
    }
    res = mCameraDevice->startPreview();
    if (res != NO_ERROR) {
	ALOGE("Err : Start preview failed!!");
	return -1;	
    }
    // system(SOUND_CAMERA_CLICK);
    res = mCameraDevice->takePicture(CAMERA_MSG_COMPRESSED_IMAGE);
    if (res != NO_ERROR) {
	ALOGE("Err : takePicture failed!!");
	return -1;	
    }
    return 0;
}

void TakePicture::stop()
{
    ALOGV("stop in");
    // Exit();
    if (mCameraDevice != NULL) {
        mCameraDevice->stopPreview();
        mCameraDevice->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_NOOP);
        mCameraDevice->disconnect();
    }
    PowerManager::UnLockSleep();
    ALOGV("stop out");

}

TakePicture::~TakePicture()
{
    ALOGV("--%s", __FUNCTION__);
}

void TakePicture::notify(int32_t msgType, int32_t ext1, int32_t ext2)
{
    ALOGV("msgType : %d", msgType);
    switch (msgType) {
    case CAMERA_ERROR_UNKNOWN:
    case CAMERA_ERROR_PREVIEW:
    case CAMERA_ERROR_TAKEPICTURE:
	ALOGE("Err : Camera subSystem has Errors!");
	writePipe(PIPE_BUF_CAPTURE_ERROR);
	system("killall -2 TakePicture");
	break;
    case CAMERA_ERROR_HAL_STORE:
	mHardStoreSuccess = false;
	break;
    case CAMERA_MSG_RAW_IMAGE_NOTIFY:
	break;
    default:
	break;
    }
}

void TakePicture::postData(int32_t msgType, const sp<IMemory>& dataPtr, camera_frame_metadata_t *metadata)
{
    ALOGV("msgType : %d", msgType);
    switch (msgType) {
    case CAMERA_MSG_PREVIEW_FRAME: {
#if DUMP_DEBUG
        dumpPreviewData(dataPtr);
#endif
        mPreviewFrameCount++;
        break;
    }
    case CAMERA_MSG_COMPRESSED_IMAGE: {
	if (!(mHardStore && mHardStoreSuccess)) {
	    /* Hal Store Failed, Write file*/
	    writePictureFile(dataPtr);
	}
	ALOGV("compressed image!");
	writePipe(PIPE_BUF_CAPTURE_OK);
	  /*hal jpeg ok*/
	Exit();
	break;
    }
    case CAMERA_MSG_RAW_IMAGE:
	break;
    default:
        break;
    }
}

void TakePicture::postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr)
{
    ALOGV("msgType : %d", msgType);
}

/*---------------------------------------------------------------------------------*/

void TakePicture::dumpPreviewData(const sp<IMemory>& dataPtr)
{
    if (dataPtr != NULL) {
        ssize_t offset;
        size_t size;
        sp<IMemoryHeap> heap = dataPtr->getMemory(&offset, &size);
        ALOGV("dumpPreviewData: off=%ld, size=%d", offset, size);
        uint8_t *heapBase = (uint8_t*)heap->base();

        if (heapBase != NULL) {
            const uint8_t* data = reinterpret_cast<const uint8_t*>(heapBase + offset);

	    FILE* fp = NULL;
	    char fileName[64];
	    snprintf(fileName, sizeof(fileName), "/home/img/camera-%d", mPreviewFrameCount);
	    if ((fp=fopen(fileName, "w+")) != NULL) {
		fwrite(data, size, 1, fp);
		fclose(fp);
	    }
	 }
    }
}

void TakePicture::writePictureFile(const sp<IMemory>& dataPtr)
{
    if (dataPtr != NULL) {
        ssize_t offset;
        size_t size;
        sp<IMemoryHeap> heap = dataPtr->getMemory(&offset, &size);
        uint8_t *heapBase = (uint8_t*)heap->base();
        if (heapBase != NULL) {
            const uint8_t* data = reinterpret_cast<const uint8_t*>(heapBase + offset);
	    FILE* fp = NULL;
	    if ((fp=fopen(mFileName.c_str(), "w+")) != NULL) {
		fwrite(data, size, 1, fp);
		fclose(fp);
	    }
	  }
    }
}

void TakePicture::createFileName(string& filename)
{
    char teeTime[16];
    struct tm tm; 
    struct timeval tv;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm);
    strftime(teeTime, sizeof(teeTime), "%Y%m%d%H%M%S", &tm);
    string photoPath = "";
    SettingsDB::getString(DB_KEY_PHOTO_PATH, photoPath);
    filename.append(photoPath);
    filename.append(PHOTO_NAME_PREFIX);
    filename.append(teeTime);
    filename.append(".jpg");

}

void TakePicture::Exit()
{
    pid_t pid = gettid(); 
    kill(pid, SIGINT);
}

int TakePicture::writePipe(const char* buf){
    int pipe_fd = -1;
    int res = -1;
    if(buf == NULL || strcmp(buf, "") == 0) {
	ALOGE("pipe buf is empty");	
	return res;
    }
    ALOGV("pipeName: %s buf: %s", PIPE_NAME, buf);
    //以只写阻塞方式打开FIFO文件
    pipe_fd = open(PIPE_NAME, O_WRONLY);
    
    if (pipe_fd < 0) {
	ALOGE("Open bt pipe file failed");	
	return res;
    }
    int size = strlen(buf);
    //向FIFO文件写数据
    res = write(pipe_fd, buf, size);

    if(res == -1) {
	ALOGE("write error on bt pipe");
    }
    close(pipe_fd);
    return res;
}

}; // namespace android
