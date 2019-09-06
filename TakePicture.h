#ifndef APP_TAKEPICTURE_H
#define APP_TAKEPICTURE_H

#include <string>
#include <sstream>
#include <camera/Camera.h>
#include <camera/CameraParameters.h>

using std::string;
using std::ostringstream;

#define CAMERA_ERROR_UNKNOWN                1
#define CAMERA_ERROR_PREVIEW                2
#define CAMERA_ERROR_TAKEPICTURE            3
#define CAMERA_ERROR_RECORD                 4
#define CAMERA_ERROR_HAL_STORE              5
#define CAMERA_ERROR_SERVER_DIED            100

namespace android {

class TakePicture : public CameraListener {

public:
    TakePicture();
    status_t start();
    void stop();
    int writePipe(const char* buf);
    ~TakePicture();
    string           mFileName;
    virtual void notify(int32_t msgType, int32_t ext1, int32_t ext2);
    virtual void postData(int32_t msgType, const sp<IMemory>& dataPtr, camera_frame_metadata_t *metadata);
    virtual void postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr);

private:
    int               mNumberOfCameras;
    sp<Camera>        mCameraDevice;
    CameraParameters  mParameters;
    Vector<Size>      mSupportedPreviewSizes;
    Vector<Size>      mSupportedPictureSizes;


    unsigned int     mPreviewFrameCount;
    bool             mHardStore;
    bool             mHardStoreSuccess;

    void createFileName(string &filename);
    void dumpPreviewData(const sp<IMemory>& dataPtr);
    void writePictureFile(const sp<IMemory>& dataPtr);
    void Exit();
};

}; // namespace android

#endif
