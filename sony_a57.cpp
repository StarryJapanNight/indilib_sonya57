/*
 //SET_SOURCE_FILES_PROPERTIES(gphoto_readimage.cpp PROPERTIES COMPILE_FLAGS "-Wno-deprecated-declarations")
Sony A57 + FlashAir
*/

#include <memory>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <experimental/filesystem>

#include <fcntl.h>   	 /* File Control Definitions           */
#include <termios.h>	 /* POSIX Terminal Control Definitions */
#include <unistd.h> 	 /* UNIX Standard Definitions 	       */ 
#include <errno.h>   	 /* ERROR Number Definitions           */
#include <sys/ioctl.h>   /* ioctl()                            */

#include "config.h"
#include "indidevapi.h"
#include "eventloop.h"
#include "sony_a57.h"
#include "jpeg.h"




#define MAX_CCD_TEMP   45   /* Max CCD temperature */
#define MIN_CCD_TEMP   -55  /* Min CCD temperature */
#define MAX_X_BIN     1   /* Max Horizontal binning */
#define MAX_Y_BIN      1   /* Max Vertical binning */
#define MAX_PIXELS     4912 /* Max number of pixels in one dimension */
#define TEMP_THRESHOLD .25  /* Differential temperature threshold (C)*/
#define MAX_DEVICES    2   /* Max device cameraCount */

static int cameraCount;
static SonyA57 *cameras[MAX_DEVICES];

/**********************************************************
 *
 *  IMPORRANT: List supported camera models in initializer of deviceTypes structure
 *
 **********************************************************/

static struct
{
    int vid;
    int pid;
    const char *name;
} deviceTypes[] = { { 0x0001, 0x0001, "Sony A57" }, { 0, 0, nullptr } };

static void cleanup()
{
    for (int i = 0; i < cameraCount; i++)
    {
        delete cameras[i];
    }
}

void ISInit()
{
    static bool isInit = false;
    if (!isInit)
    {

        cameraCount            = 1;
        struct usb_device *dev = nullptr;
        cameras[0]             = new SonyA57(dev, deviceTypes[0].name);
        //cameras[1]             = new SonyA57(dev, deviceTypes[1].name);

        atexit(cleanup);
        isInit = true;
    }
}

void ISGetProperties(const char *dev)
{
    ISInit();
    for (int i = 0; i < cameraCount; i++)
    {
        SonyA57 *camera = cameras[i];
        if (dev == nullptr || !strcmp(dev, camera->name))
        {
            camera->ISGetProperties(dev);
            if (dev != nullptr)
                break;
        }
    }
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    ISInit();
    for (int i = 0; i < cameraCount; i++)
    {
        SonyA57 *camera = cameras[i];
        if (dev == nullptr || !strcmp(dev, camera->name))
        {
            camera->ISNewSwitch(dev, name, states, names, num);
            if (dev != nullptr)
                break;
        }
    }
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    ISInit();
    for (int i = 0; i < cameraCount; i++)
    {
        SonyA57 *camera = cameras[i];
        if (dev == nullptr || !strcmp(dev, camera->name))
        {
            camera->ISNewText(dev, name, texts, names, num);
            if (dev != nullptr)
                break;
        }
    }
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    ISInit();
    for (int i = 0; i < cameraCount; i++)
    {
        SonyA57 *camera = cameras[i];
        if (dev == nullptr || !strcmp(dev, camera->name))
        {
            camera->ISNewNumber(dev, name, values, names, num);
            if (dev != nullptr)
                break;
        }
    }
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    INDI_UNUSED(dev);
    INDI_UNUSED(name);
    INDI_UNUSED(sizes);
    INDI_UNUSED(blobsizes);
    INDI_UNUSED(blobs);
    INDI_UNUSED(formats);
    INDI_UNUSED(names);
    INDI_UNUSED(n);
}
void ISSnoopDevice(XMLEle *root)
{
    ISInit();

    for (int i = 0; i < cameraCount; i++)
    {
        SonyA57 *camera = cameras[i];
        camera->ISSnoopDevice(root);
    }
}

SonyA57::SonyA57(DEVICE device, const char *name)
{
    this->device = device;
    snprintf(this->name, 32, "Generic CCD %s", name);
    setDeviceName(this->name);

    setVersion(GENERIC_VERSION_MAJOR, GENERIC_VERSION_MINOR);
}

SonyA57::~SonyA57()
{
}

const char *SonyA57::getDefaultName()
{
    return "Generic CCD";
}

bool SonyA57::initProperties()
{
    // Init parent properties first
    INDI::CCD::initProperties();

    uint32_t cap = CCD_CAN_ABORT | CCD_HAS_SHUTTER;
    SetCCDCapability(cap);

    addConfigurationControl();
    addDebugControl();
    return true;
}

void SonyA57::ISGetProperties(const char *dev)
{
    INDI::CCD::ISGetProperties(dev);
}

bool SonyA57::updateProperties()
{
    INDI::CCD::updateProperties();

    if (isConnected())
    {
        // Let's get parameters now from CCD
        setupParams();

        timerID = SetTimer(POLLMS);
    }
    else
    {
        rmTimer(timerID);
    }

    return true;
}

bool SonyA57::Connect()
{
    LOG_INFO("Attempting to find the Generic CCD...");

    /**********************************************************
   *
   *
   *
   *  IMPORRANT: Put here your CCD Connect function
   *  If you encameraCounter an error, send the client a message
   *  e.g.
   *  LOG_INFO( "Error, unable to connect due to ...");
   *  return false;
   *
   *
   **********************************************************/

    /* Success! */
    LOG_INFO("CCD is online. Retrieving basic data.");

    return true;
}

bool SonyA57::Disconnect()
{
    /**********************************************************
   *
   *
   *
   *  IMPORRANT: Put here your CCD disonnect function
   *  If you encameraCounter an error, send the client a message
   *  e.g.
   *  LOG_INFO( "Error, unable to disconnect due to ...");
   *  return false;
   *
   *
   **********************************************************/

    LOG_INFO("CCD is offline.");
    return true;
}

bool SonyA57::setupParams()
{
    float x_pixel_size, y_pixel_size;
    int bit_depth = 8;
    int x_1, y_1, x_2, y_2;

    /**********************************************************
   *
   *
   *
   *  IMPORRANT: Get basic CCD parameters here such as
   *  + Pixel Size X
   *  + Pixel Size Y
   *  + Bit Depth?
   *  + X, Y, W, H of frame
   *  + Temperature
   *  + ...etc
   *
   *
   *
   **********************************************************/

    ///////////////////////////
    // 1. Get Pixel size
    ///////////////////////////
    // Actucal CALL to CCD to get pixel size here
    x_pixel_size = 4.82;
    y_pixel_size = 4.82;

    ///////////////////////////
    // 2. Get Frame
    ///////////////////////////

    // Actucal CALL to CCD to get frame information here
    x_1 = y_1 = 0;
    x_2       = 4912;
    y_2       = 3264;

    ///////////////////////////
    // 3. Get temperature
    ///////////////////////////
    // Setting sample temperature -- MAKE CALL TO API FUNCTION TO GET TEMPERATURE IN REAL DRIVER
    TemperatureN[0].value = 25.0;
    LOGF_INFO("The CCD Temperature is %f", TemperatureN[0].value);
    IDSetNumber(&TemperatureNP, nullptr);

    ///////////////////////////
    // 4. Get temperature
    ///////////////////////////
    SetCCDParams(x_2 - x_1, y_2 - y_1, bit_depth, x_pixel_size, y_pixel_size);

    // Now we usually do the following in the hardware
    // Set Frame to LIGHT or NORMAL
    // Set Binning to 1x1
    /* Default frame type is NORMAL */

    // Let's calculate required buffer
    int nbuf;
    nbuf = PrimaryCCD.getXRes() * PrimaryCCD.getYRes() * PrimaryCCD.getBPP() / 8; //  this is pixel cameraCount
    nbuf += 512;                                                                  //  leave a little extra at the end
    PrimaryCCD.setFrameBufferSize(nbuf);

    return true;
}

bool SonyA57::StartExposure(float duration)
{
    minDuration = 1;
    if (duration < minDuration)
    {
        DEBUGF(INDI::Logger::DBG_WARNING,
               "Exposure shorter than minimum duration %g s requested. \n Setting exposure time to %g s.", duration,
               minDuration);
        duration = minDuration;
    }


		
    /*------------------ Opening the Serial port ------------------*/

    /* Change /dev/ttyUSB0 to the one corresponding to your system */

    camerafd = open("/dev/ttyUSB0",O_RDWR | O_NOCTTY );	        /* ttyUSB0 is the FT232 based USB2SERIAL Converter   */
                            /* O_RDWR Read/Write access to serial port           */
                            /* O_NOCTTY - No terminal will control the process   */
                            /* Blocking Mode  */                       
                            
    if(camerafd == -1) {
         printf("\n    Error! in Opening ttyUSB0  ");
    }						/* Error Checking */
    else
    {
            printf("\n    ttyUSB0 Opened Successfully \n");

    }
    /*--------- Controlling the RTS pins  of Serial Port --------*/

    int RTS_flag;
    RTS_flag = TIOCM_RTS;

    // Open shutter
    ioctl(camerafd,TIOCMBIS,&RTS_flag);
                    
    PrimaryCCD.setExposureDuration(duration);
    ExposureRequest = duration;

    gettimeofday(&ExpStart, nullptr);
    LOGF_INFO("Taking a %g seconds frame...", ExposureRequest);

    InExposure = true;

    return true;
}

bool SonyA57::AbortExposure()
{
    /**********************************************************
   *
   *
   *
   *  IMPORRANT: Put here your CCD abort exposure here
   *  If there is an error, report it back to client
   *  e.g.
   *  LOG_INFO( "Error, unable to abort exposure due to ...");
   *  return false;
   *
   *
   **********************************************************/
	int RTS_flag;
   
    RTS_flag = TIOCM_RTS;	/* Modem Constant for RTS pin */

 
    ioctl(camerafd,TIOCMBIC,&RTS_flag); 		 /* fd -file descriptor pointing to the opened Serial port */
    
    close(camerafd); /* Close the Opened Serial Port */

    InExposure = false;
    return true;
}

bool SonyA57::UpdateCCDFrameType(INDI::CCDChip::CCD_FRAME fType)
{
    INDI::CCDChip::CCD_FRAME imageFrameType = PrimaryCCD.getFrameType();

    if (fType == imageFrameType)
        return true;

    switch (imageFrameType)
    {
        case INDI::CCDChip::BIAS_FRAME:
        case INDI::CCDChip::DARK_FRAME:
            /**********************************************************
     *
     *
     *
     *  IMPORRANT: Put here your CCD Frame type here
     *  BIAS and DARK are taken with shutter closed, so _usually_
     *  most CCD this is a call to let the CCD know next exposure shutter
     *  must be closed. Customize as appropiate for the hardware
     *  If there is an error, report it back to client
     *  e.g.
     *  LOG_INFO( "Error, unable to set frame type to ...");
     *  return false;
     *
     *
     **********************************************************/
            break;

        case INDI::CCDChip::LIGHT_FRAME:
        case INDI::CCDChip::FLAT_FRAME:
            /**********************************************************
     *
     *
     *
     *  IMPORRANT: Put here your CCD Frame type here
     *  LIGHT and FLAT are taken with shutter open, so _usually_
     *  most CCD this is a call to let the CCD know next exposure shutter
     *  must be open. Customize as appropiate for the hardware
     *  If there is an error, report it back to client
     *  e.g.
     *  LOG_INFO( "Error, unable to set frame type to ...");
     *  return false;
     *
     *
     **********************************************************/
            break;
    }

    PrimaryCCD.setFrameType(fType);

    return true;
}

bool SonyA57::UpdateCCDFrame(int x, int y, int w, int h)
{
    /* Add the X and Y offsets */
    long x_1 = x;
    long y_1 = y;

    long bin_width  = x_1 + (w / PrimaryCCD.getBinX());
    long bin_height = y_1 + (h / PrimaryCCD.getBinY());

    if (bin_width > PrimaryCCD.getXRes() / PrimaryCCD.getBinX())
    {
        LOGF_INFO("Error: invalid width requested %d", w);
        return false;
    }
    else if (bin_height > PrimaryCCD.getYRes() / PrimaryCCD.getBinY())
    {
        LOGF_INFO("Error: invalid height request %d", h);
        return false;
    }

    /**********************************************************
   *
   *
   *
   *  IMPORRANT: Put here your CCD Frame dimension call
   *  The values calculated above are BINNED width and height
   *  which is what most CCD APIs require, but in case your
   *  CCD API implementation is different, don't forget to change
   *  the above calculations.
   *  If there is an error, report it back to client
   *  e.g.
   *  LOG_INFO( "Error, unable to set frame to ...");
   *  return false;
   *
   *
   **********************************************************/

    // Set UNBINNED coords
    PrimaryCCD.setFrame(x_1, y_1, w, h);

    int nbuf;
    nbuf = (bin_width * bin_height * PrimaryCCD.getBPP() / 8); //  this is pixel count
    nbuf += 512;                                               //  leave a little extra at the end
    PrimaryCCD.setFrameBufferSize(nbuf);

    LOGF_DEBUG("Setting frame buffer size to %d bytes.", nbuf);

    return true;
}

float SonyA57::CalcTimeLeft()
{
    double timesince;
    double timeleft;
    struct timeval now;
    gettimeofday(&now, nullptr);

    timesince = (double)(now.tv_sec * 1000.0 + now.tv_usec / 1000) -
                (double)(ExpStart.tv_sec * 1000.0 + ExpStart.tv_usec / 1000);
    timesince = timesince / 1000;

    timeleft = ExposureRequest - timesince;
    return timeleft;
}



/* Downloads the image from the CCD.
 N.B. No processing is done on the image */
int SonyA57::grabImage()
{
    uint8_t *image = PrimaryCCD.getFrameBuffer();
    int width      = PrimaryCCD.getSubW() / PrimaryCCD.getBinX() * PrimaryCCD.getBPP() / 8;
    int height     = PrimaryCCD.getSubH() / PrimaryCCD.getBinY();
    int bitdepth   = PrimaryCCD.getBPP();

    using namespace marengo::jpeg;
    namespace fs = std::experimental::filesystem;
 
    // File extension to copy
    std::string ext(".JPG");
    
    try {
    
        // Source folder
        std::string src = "/mnt/dav/";

        // Destination folder
        const fs::path dst = "/home/phil/flashair/";

        // Copy all new JPGs from src to dst
        for (auto & p : fs::directory_iterator(src)) {
            std::string fileName = p.path();
                  
            if(p.path().extension() == ext) {
                
                // Target path
                auto target = dst / p.path().filename(); 

                if(fs::copy_file(p, target, fs::copy_options::skip_existing ) == true)
                {
                    DEBUG(INDI::Logger::DBG_SESSION,(std::string("Downloaded: ")+fileName).c_str());
                }
            }
        }

        /*
        // Select the most recent
        // This often gets the wrong file if two shots taken quickly (same minute)

        std::time_t latest_time = 0;
        std::string selected_path = "";
        for (auto & p : fs::directory_iterator(dst)) {
            std::string fileName = p.path();
                    
            auto fstime = fs::last_write_time(p);
            std::time_t cfstime = decltype(fstime)::clock::to_time_t(fstime); 
            if(cfstime > latest_time) {

                // Most recent file
                latest_time = cfstime;              
                selected_path = p.path();
                
            }
        }
        */

        // Select the highest filenumber
        int highest_fileNum = 0;
        std::string selected_path = "";
        for (auto & p : fs::directory_iterator(dst)) {
            std::string fileName = p.path().stem();
            std::string fileNum_str = fileName.substr(4,4);
                                
            int fileNum = std::stoi(fileNum_str);

            if(fileNum >= highest_fileNum) {

                // Most recent file
                highest_fileNum = fileNum;              
                selected_path = p.path();

                
            }
        }

        DEBUG(INDI::Logger::DBG_SESSION,(std::string("Reading: ")+selected_path).c_str());

        // Load jpeg using libjpg
        Image img(selected_path);

        // Store luminance into image array
        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                image[i * width + j] = img.getLuminance(j,i)%255;
            }
        }

    }
    catch (std::exception& e)
    {
        std::string error_tmp = e.what();
        DEBUG(INDI::Logger::DBG_ERROR,error_tmp.c_str());
    }

    ExposureComplete(&PrimaryCCD);

    return 0;

}

void SonyA57::TimerHit()
{
    int timerID = -1;
    long timeleft;

    if (isConnected() == false)
        return; //  No need to reset timer if we are not connected anymore

    if (InExposure)
    {
        timeleft = CalcTimeLeft();

        if (timeleft < 1.0)
        {
            if (timeleft > 0.25)
            {
                //  a quarter of a second or more
                //  just set a tighter timer
                timerID = SetTimer(250);
            }
            else
            {
                if (timeleft > 0.07)
                {
                    //  use an even tighter timer
                    timerID = SetTimer(50);
                }
                else
                {
                    //  it's real close now, so spin on it
                    while (timeleft > 0)
                    {

                        // Breaking in simulation, in real driver either loop until time left = 0 or use an API call to know if the image is ready for download
                        break;

                    }
 


                    PrimaryCCD.setExposureLeft(0);
                    InExposure = false;
        
                    // End exposure
                    AbortExposure();

                    /* We're done exposing */
                    LOG_INFO("Exposure done.");
        
                    // Wait for image to save to SDCard
                    sleep(5);

                    /* We're done exposing */
                    LOG_INFO("Waiting for file to write to SD card.");

                    // Download image from SDCard
                    grabImage();
                }
            }
        }
        else
        {
            if (isDebug())
            {
                IDLog("With time left %ld\n", timeleft);
                IDLog("image not yet ready....\n");
            }

            PrimaryCCD.setExposureLeft(timeleft);
        }
    }

    if (timerID == -1)
        SetTimer(POLLMS);
    return;
}

