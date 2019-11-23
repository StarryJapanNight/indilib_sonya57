/*
Sony A57 + FlashAir
*/

#ifndef GENERIC_CCD_H
#define GENERIC_CCD_H

#include <indiccd.h>
#include <iostream>

using namespace std;

#define DEVICE struct usb_device *

class SonyA57 : public INDI::CCD
{
  public:
    SonyA57(DEVICE device, const char *name);
    virtual ~SonyA57();

    const char *getDefaultName();

    bool initProperties();
    void ISGetProperties(const char *dev);
    bool updateProperties();

    bool Connect();
    bool Disconnect();

    bool StartExposure(float duration);
    bool AbortExposure();

  protected:
    void TimerHit();
    virtual bool UpdateCCDFrame(int x, int y, int w, int h);
    virtual bool UpdateCCDFrameType(INDI::CCDChip::CCD_FRAME fType);

  private:
    DEVICE device;
    char name[32];

    double ccdTemp;
    double minDuration;
    int camerafd;	/*File Descriptor*/
    unsigned short *imageBuffer;

    int timerID;

    INDI::CCDChip::CCD_FRAME imageFrameType;

    struct timeval ExpStart;

    float ExposureRequest;
    float TemperatureRequest;

    float CalcTimeLeft();
    int grabImage();
    bool setupParams();
    bool sim;

    friend void ::ISGetProperties(const char *dev);
    friend void ::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num);
    friend void ::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num);
    friend void ::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num);
    friend void ::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                            char *formats[], char *names[], int n);
};

#endif // GENERIC_CCD_H
