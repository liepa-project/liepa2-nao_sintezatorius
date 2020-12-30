/**
 * Žr. licenzijos failą LICENSE source medžio viršuje. (LICENSE file at the top of the source tree.)
 * 
 * Projektas LIEPA-2, 2017 - 2020 m. (https://liepa2.raštija.lt)
 * Vilniaus universitetas (https://www.vu.lt)
 * 
 * @file liepaTTS.cpp
 * 
 * @author dr. Margarita Beniušė (margarita.beniuse@mif.vu.lt), dr. Pijus Kasparaitis (pkasparaitis@yahoo.com)
 * 2020 12 28
 */

#include "liepaTTS.h"
#include "LithUSS.h"
#include <qi/log.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <alsa/asoundlib.h>
#include <alsa/control.h>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <signal.h>
#include <iconv.h>

#define MIN_EVARR_SIZE 1000

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API

#define VOICE_VYR "vyr" // male (default voice)
#define VOICE_MOT "mot" // female

// LiepaTTS voice speed range [30(fast), 300(slow)], default 100 
#define MIN_SPEED 30
#define MAX_SPEED 300
#define DEFAULT_SPEED 100

// NAO speed range
#define MIN_SPEED_NAO 50
#define MAX_SPEED_NAO 400
#define DEFAULT_SPEED_NAO 100

// Volume range [0, 100], default 100
#define MIN_VOLUME 0
#define MAX_VOLUME 100
#define DEFAULT_VOLUME 100

// LiepaTTS pitch range [75(low), 133(high)], default=100 // tono aukštis
#define MIN_PITCH 75
#define MAX_PITCH 133
#define DEFAULT_PITCH 100

char projectDir[] = "/home/nao/naoqi/lib/LiepaTTSResources/";
char garsuDbDirVyr[] = "/home/nao/naoqi/lib/LiepaTTSResources/Edvardas/";
char garsuDbDirMot[] = "/home/nao/naoqi/lib/LiepaTTSResources/Aiste/";
char logFile[] = "/home/nao/naoqi/lib/LiepaTTSResources/liepaTTS.log";
char logFileLocal[] = "liepaTTS.log";

std::string voice = VOICE_VYR;
int speed = DEFAULT_SPEED; // speed range [MIN_SPEED, MAX_SPEED]
int defaultVoiceSpeed = DEFAULT_SPEED; // speed range [MIN_SPEED, MAX_SPEED]
int volume = DEFAULT_VOLUME; // volume range [MIN_VOLUME, MAX_VOLUME]
int pitch = DEFAULT_PITCH; // pitch range [MIN_PITCH, MAX_PITCH]
    
iconv_t conv_;
char defaultKoduote[] = "UTF-8"; // default text coding

// playback
static const char        *device      = "default";
static snd_pcm_format_t   format      = SND_PCM_FORMAT_S16_LE;
static unsigned int       rate        = 22050;
static unsigned int       channels    = 1;
static snd_output_t      *output      = NULL;
static snd_pcm_uframes_t  buffer_size = 0;
static snd_pcm_uframes_t  period_size = 0;

static int in_aborting = 0;

int evarrsize;
int evarrsizet;
struct event* evarr;
unsigned int signbufsize;
unsigned int signbufsizet;
short* signbuf;

short* hData[2];
char tekstokopija[10000];
char zodis[200];
char sakinys[200];

qi::AnyObject almemory;

/**
 * Print to log file.
 * @param message message text.
 * @param mode a+ append; w+ write.
 */
void printToLog(const char* message, char* mode) 
{
    FILE* rf;
    rf = fopen(logFile, mode);
	if(rf == NULL) rf = fopen(logFileLocal, mode);
	if(rf != NULL)
	{ 
        fprintf(rf, message); 
        fclose(rf); 
    }
	fprintf(stdout, message);
}

/**
 * wav file header.
 */
void formuoti_wav_antraste(unsigned long dLen, FILE *stream)
{
    unsigned long rID=0x46464952, rLen, wID=0x45564157, fID=0x20746D66;
    unsigned short wFormatTag=0x0001, nChannels=0x0001;
    unsigned long fLen=0x00000010, nSamplesPerSec=22050, nAvgBytesPerSec;
    unsigned short nBlockAlighn=0x0002, wBitsPerSample=0x0010;
    unsigned long dID=0x61746164;

    fwrite(&rID,4,1,stream);
    rLen=dLen+36;
    fwrite(&rLen,4,1,stream);
    fwrite(&wID,4,1,stream);
    fwrite(&fID,4,1,stream);
    fwrite(&fLen,4,1,stream);
    fwrite(&wFormatTag,2,1,stream);
    fwrite(&nChannels,2,1,stream);
    fwrite(&nSamplesPerSec,4,1,stream);
    nAvgBytesPerSec=nSamplesPerSec*2;
    fwrite(&nAvgBytesPerSec,4,1,stream);
    fwrite(&nBlockAlighn,2,1,stream);
    fwrite(&wBitsPerSample,2,1,stream);
    fwrite(&dID,4,1,stream);
    fwrite(&dLen,4,1,stream);
}

/**
 * Close PCM handle preserving pending frames.
 */
int closeAlsa(snd_pcm_t *_soundDevice) 
{
    if (_soundDevice != NULL) 
    {  
        snd_pcm_drain(_soundDevice);
        snd_pcm_close(_soundDevice);
        printToLog("LiepaTTS: Audio close DONE.\n", "a+");
    }
    return 0;
}

/**
 * Close PCM handle dropping pending frames.
 */
int closeAlsaWithDrop(snd_pcm_t *_soundDevice) 
{
    if (_soundDevice != NULL) 
    {  
        snd_pcm_drop(_soundDevice);
        snd_pcm_close(_soundDevice);
        printToLog("LiepaTTS: Audio stop DONE.\n", "a+");
    }
    return 0;
}

/**
 * Close text codec converter.
 */
int closeConverter() 
{
    if(conv_ == NULL) return 0;
    int ret = 0;
    if (ret = iconv_close(conv_) != 0)  
    {
        printToLog("LiepaTTS: ERROR: iconv_close() failed to close text code converter.\n", "a+");
        return ret;
    }
    return ret;
}

LiepaTTS::LiepaTTS(qi::SessionPtr session)
   : _session(session)
{
    init();
}

LiepaTTS::~LiepaTTS()
{
    unloadLibraries();
    
	if (evarr != NULL) free(evarr);
	evarr = NULL;
	if (signbuf != NULL) free(signbuf);
	signbuf = NULL;

    if (hData[0] != NULL) free(hData[0]);
	if (hData[1] != NULL) free(hData[1]);

    printToLog("LiepaTTS: Baigtas.\n", "a+");
}

/**
 * Initialize
 */
void LiepaTTS::init()
{
    std::string str;
    
	int hr = 0;
	int textsize = 500;
	evarrsize = textsize * 2;
	if (evarrsize < MIN_EVARR_SIZE) evarrsize = MIN_EVARR_SIZE;
	evarr = NULL;
	evarr = (struct event*)malloc(evarrsize * sizeof(struct event));
	if (evarr == NULL)
	{
		printToLog("LiepaTTS ERROR: Inicializavimas -104\n", "a+");
		return;
	}

	signbufsize = textsize * 1250;
	if	(signbufsize < MIN_EVARR_SIZE * 1250) signbufsize = MIN_EVARR_SIZE * 1250;
	signbuf = NULL;
	signbuf = (short*)calloc(signbufsize, sizeof(short));
	if(signbuf == NULL)
	{
        printToLog("LiepaTTS ERROR: Inicializavimas -105\n", "a+");
		return;
	}
	if(voice == VOICE_MOT)
        hr = initLUSS(projectDir, garsuDbDirMot);
    else
        hr = initLUSS(projectDir, garsuDbDirVyr);
	
	if(hr < 0)
	{
        str = "LiepaTTS ERROR: Inicializavimas "; str += std::to_string(hr); str += "\n";
        printToLog(str.c_str(), "a+");
		return;
	}

    hData[0] = (short*)calloc(100000, sizeof(short));
    hData[1] = (short*)calloc(100000, sizeof(short));
    
    printToLog("LiepaTTS: Inicializavimas atliktas.\n", "a+");
}


void LiepaTTS::printHello()
{
    std::cout << "Laba diena." << std::endl;
}

void LiepaTTS::printWord(char* word)
{
    std::cout << word << std::endl;
}

bool LiepaTTS::returnTrue()
{
    return true;
}

void LiepaTTS::sayHello()
{
    sayText("Laba diena.");
}

/**
 * Convert text coding.
 */
bool openConverter(const char *fromcode) 
{
    conv_ = iconv_open("WINDOWS-1257", fromcode);
    
    if (conv_ == (iconv_t) - 1) 
    {
      
        printToLog("LiepaTTS ERROR: Failed to open text encoding converter\n", "a+");
      
        if(strcmp (fromcode, "UTF-8") == 0 || strcmp (fromcode, "UTF8") == 0) 
        {
            printToLog("LiepaTTS ERROR: Koduotės keitimas nebus vykdomas.\n", "a+");
            return false;

        } else {
            printToLog("LiepaTTS WARNING: Trying to apply text code converter UTF-8 -> Windows1257, result may be unpredictable.", "a+");
            conv_ = iconv_open("WINDOWS-1257", "UTF-8");
            if (conv_ == (iconv_t) - 1) 
            {
                printToLog("LiepaTTS ERROR: Failed to open text code converter UTF-8 -> Windows1257.\n", "a+");
                return false;
            }
        }
    }
    return true;
}

/**
 * Set PCM hardware parameters.
 */
static int set_hwparams(snd_pcm_t *_soundDevice, snd_pcm_hw_params_t *hw_params) 
{
    int err, dir;
    unsigned int rrate;
    std::string str;
  
    if ((err = snd_pcm_hw_params_any (_soundDevice, hw_params)) < 0) 
    {
        str = "LiepaTTS ERROR: AUDIO PCM HW params: cannot initialize hardware parameter structure: ";
        str += snd_strerror (err); str += "\n";
        printToLog(str.c_str(), "a+");
        return err;
    }
  
    if ((err = snd_pcm_hw_params_set_access (_soundDevice, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) 
    {
        str = "LiepaTTS ERROR: AUDIO PCM HW params: Access type not available for playback: ";
        str += snd_strerror (err); str += "\n";
        printToLog(str.c_str(), "a+");
        return err;
    }
  
    if ((err = snd_pcm_hw_params_set_format (_soundDevice, hw_params, format)) < 0) 
    {
        str = "LiepaTTS ERROR: AUDIO PCM HW params: Sample format not available for playback: ";
        str += snd_strerror (err); str += "\n";
        printToLog(str.c_str(), "a+");
        return err;
    }
  
    if ((err = snd_pcm_hw_params_set_channels (_soundDevice, hw_params, channels)) < 0) 
    {
        str = "LiepaTTS ERROR: AUDIO PCM HW params: Channels count ";
        str += std::to_string(channels);
        str += " not available for playbacks: ";
        str += snd_strerror (err); str += "\n";
        printToLog(str.c_str(), "a+");  
        return err;
    }

    rrate = rate;
    err = snd_pcm_hw_params_set_rate_near (_soundDevice, hw_params, &rrate, 0);
    if (err < 0) 
    {			
        str = "LiepaTTS ERROR: AUDIO PCM HW params: Rate ";
        str += std::to_string(rate);
        str += " Hz not available for playback: ";
        str += snd_strerror (err); str += "\n";
        printToLog(str.c_str(), "a+");
        return err;
    }
    if ((int)rrate != rate) 
    {
        printf("ERROR: Rate doesn't match (requested %iHz, get %iHz, err %d)\n", rate, rrate, err);
        return -EINVAL;
    }

    err = snd_pcm_hw_params (_soundDevice, hw_params);

    snd_pcm_hw_params_get_period_size(hw_params, &period_size, &dir);

    snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size );
  
    if (2*period_size > buffer_size) 
    {
        printToLog("LiepaTTS ERROR: AUDIO PCM HW params: Buffer too small, could not use.\n", "a+");
        return -EINVAL;
    }
  
    snd_pcm_hw_params_free (hw_params);
  
    if (err < 0) 
    {
        str = "LiepaTTS ERROR: AUDIO PCM HW params: Unable to set hw params for playback: ";
        str += snd_strerror (err); str += "\n";
        printToLog(str.c_str(), "a+");
        return err;
    }
    return 0;
}

/**
 * Set PCM software parameters.
 */
static int set_swparams(snd_pcm_t *_soundDevice, snd_pcm_sw_params_t *sw_params) 
{
    std::string str;
    int err;

    err = snd_pcm_sw_params_current(_soundDevice, sw_params);
    if (err < 0) 
    {
        str = "LiepaTTS ERROR: AUDIO PCM SW params: Unable to determine current sw params for playback: ";
        str += snd_strerror (err); str += "\n";
        printToLog(str.c_str(), "a+");
        return err;
    }

    err = snd_pcm_sw_params_set_start_threshold(_soundDevice, sw_params, buffer_size - period_size);
    if (err < 0) 
    {
        str = "LiepaTTS ERROR: AUDIO PCM SW params: Unable to set start threshold mode for playback: ";
        str += snd_strerror (err); str += "\n";
        printToLog(str.c_str(), "a+");
        return err;
    }
    err = snd_pcm_sw_params_set_avail_min(_soundDevice, sw_params, period_size);
    if (err < 0) 
    {
        str = "LiepaTTS ERROR: AUDIO PCM SW params: Unable to set avail min for playback: ";
        str += snd_strerror (err); str += "\n";
        printToLog(str.c_str(), "a+");
        return err;
    }

    err = snd_pcm_sw_params(_soundDevice, sw_params);
  
    snd_pcm_sw_params_free (sw_params);
  
    if (err < 0) 
    {
        str = "LiepaTTS ERROR: AUDIO PCM SW params: Unable to set sw params for playback: ";
        str += snd_strerror (err); str += "\n";
        printToLog(str.c_str(), "a+");
        return err;
    }
    return 0;
}

static int xrun_recovery(snd_pcm_t *_soundDevice, int err) 
{
    std::string str;
    
    if (err == -EPIPE)
    {
        printToLog("LiepaTTS: ERROR xrun_recovery: Audio Underrun occurred. Trying to recover.\n", "a+");
        err = snd_pcm_prepare(_soundDevice);
        if (err < 0) {
        str = "LiepaTTS: ERROR xrun_recovery: Can't recovery from audio underrun, prepare failed: ";
        str += snd_strerror (err); str += "\n";
        printToLog(str.c_str(), "a+");
        } else {
            printToLog("LiepaTTS: SUCCESS xrun_recovery: Recovered from audio underrun.\n", "a+"); 
        }
        return 0;
  
  } else if (err == -ESTRPIPE) {

        while ((err = snd_pcm_resume(_soundDevice)) == -EAGAIN)
            sleep(1);

        if (err < 0) 
        {
            err = snd_pcm_prepare(_soundDevice);
            if (err < 0) 
            {
                str = "LiepaTTS: ERROR xrun_recovery: Can't recovery from audio suspend, prepare failed: ";
                str += snd_strerror (err); str += "\n";
                printToLog(str.c_str(), "a+");
            }
        }
        return 0;

    }  else if (err < 0) {
        str = "LiepaTTS: ERROR xrun_recovery: ";
        str += snd_strerror (err); str += "\n";
        printToLog(str.c_str(), "a+");
    }

  return err;
}

static int prg_exit(snd_pcm_t *_soundDevice, int code) 
{
     unloadLibraries();
     closeConverter();
     if (_soundDevice)  
     {
       snd_pcm_close(_soundDevice);
       _soundDevice = NULL;
     }
     exit(code);
     return code;
}

static void signal_handler(int sig) 
{
    std::string str;
    str = "LiepaTTS: Aborted by signal: ";
    str += strsignal(sig); str += "\n";
    printToLog(str.c_str(), "a+");
    
    unloadLibraries();
    closeConverter();
    signal(sig, signal_handler);
}

/**
 * Prepare audio device for use.
 */
int initAlsa(snd_pcm_t *&_soundDevice, snd_pcm_hw_params_t *hw_params, snd_pcm_sw_params_t *sw_params) 
{
    std::string str;
    int err;

    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) 
    {
        str = "LiepaTTS ERROR initAlsa(): cannot allocate hardware parameter structure: ";
        str += snd_strerror (err);
        str += "\n";
        printToLog(str.c_str(), "a+");
        return err;
    }
  
    if ((err = snd_pcm_sw_params_malloc (&sw_params)) < 0) 
    {
        str = "LiepaTTS ERROR initAlsa: cannot allocate software parameter structure: ";
        str += snd_strerror (err);
        str += "\n";
        printToLog(str.c_str(), "a+");
        return err;
    }

    err = snd_output_stdio_attach(&output, stdout, 0);
    if (err < 0) 
    {
        str = "LiepaTTS ERROR initAlsa: snd_output_stdio_attach failed: ";
        str += snd_strerror (err); str += "\n";
        printToLog(str.c_str(), "a+");
        return err;
    }

    if ((err = snd_pcm_open(&_soundDevice, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) 
    {
        str = "LiepaTTS: Audio playback open error: ";
        str += snd_strerror (err); str += "\n";
        printToLog(str.c_str(), "a+");
        return err;
    }

    if ((err = set_hwparams(_soundDevice, hw_params)) < 0) 
    {
        str = "LiepaTTS ERROR initAlsa: Setting of hwparams failed: ";
        str += snd_strerror (err); str += "\n";
        printToLog(str.c_str(), "a+");
        return err;
    }
  
    if ((err = set_swparams(_soundDevice, sw_params)) < 0) 
    {
        str = "LiepaTTS ERROR initAlsa: Setting of swparams failed: ";
        str += snd_strerror(err); str += "\n";
        printToLog(str.c_str(), "a+");
        return err;
    }
  
    if ((err = snd_pcm_prepare (_soundDevice)) < 0) 
    {
        str = "LiepaTTS ERROR: Cannot prepare audio interface for use: ";
        str += snd_strerror(err); str += "\n";
        printToLog(str.c_str(), "a+");
        return err;
    }
    printf("LiepaTTS: Audio device has been prepared for use.\n");
    return 0;
}

/**
 * Playback
 */
int playAlsa(snd_pcm_t *_soundDevice, const short *soundBuffer, size_t soundSize, size_t soundCount) {

    int rc = 0;
    std::string str;
    int audioSize = snd_pcm_bytes_to_frames(_soundDevice, soundSize * soundCount);		
  
    if(buffer_size > 0) 
    {
        while (audioSize > 0 && !in_aborting) 
        {
            rc = snd_pcm_writei(_soundDevice, soundBuffer, audioSize);
   
            if (rc == -EAGAIN)
                continue;
   
            if (rc < 0) 
            {
                str = "LiepaTTS: ERROR: PLAYBACK Write error ";
                str += std::to_string(rc); str += ": "; str += snd_strerror(rc); str += "\n";
                printToLog(str.c_str(), "a+");
                rc = xrun_recovery(_soundDevice, rc);
                if (rc < 0) 
                {
                    str = "LiepaTTS: Audio playback xrun_recovery failed: ";
                    str += snd_strerror(rc);
                    str += "\n";
                    printToLog(str.c_str(), "a+");
                    return rc;
                }
                break;
            }
            soundBuffer += snd_pcm_frames_to_bytes(_soundDevice, rc);
            audioSize -= rc;
        }
        xrun_recovery(_soundDevice, rc);
        snd_pcm_drain(_soundDevice);
        snd_pcm_prepare(_soundDevice);
    } else {
        str = "LiepaTTS ERROR PLAYBACK: neteisingas Alsa buferio dydis: buffer_size(frames)="; 
        str += std::to_string(buffer_size); str += "\n";
        printToLog(str.c_str(), "a+");
        return 0;
    }
    printToLog("LiepaTTS: Audio playback DONE.\n", "a+");
    return rc;
}

int speedInRange(int val) 
{
    if( val < MIN_SPEED ) val = MIN_SPEED;
    if( val > MAX_SPEED ) val = MAX_SPEED;
    return val;
}

bool isSpeedInRangeNao(int val) 
{
    if( val < MIN_SPEED_NAO ) val = MIN_SPEED_NAO;
    if( val > MAX_SPEED_NAO ) val = MAX_SPEED_NAO;
    return val;

}

int naoSpeedToLiepaSpeed(int n) 
{
    int ret;
    ret = (MAX_SPEED*(n-MIN_SPEED_NAO)+MIN_SPEED*(MAX_SPEED_NAO-n)) / (MAX_SPEED_NAO - MIN_SPEED_NAO);
    return static_cast<int>(ret);
}

int volumeInRange(int val) 
{
    if( val < MIN_VOLUME ) val = MIN_VOLUME;
    if( val > MAX_VOLUME ) val = MAX_VOLUME;
    return val;
}

int pitchInRange(int val) 
{
    if( val < MIN_PITCH ) val = MIN_PITCH;
    if( val > MAX_PITCH ) val = MAX_PITCH;
    return val;
}

/**
 * Synthesize text and either output result to playback device (speakers) or save to file.
 * @param toSay Text to synthesize.
 * @param _speed Speed of synthesized speech, speed range [30(fast), 300(slow)], default 100.
 * @param sayToOutputFile If false then output synthesized result to playback device, if true then save synthesized result to file.
 * @param outputFileName Name of file where synthesized result may be saved.
 */
void LiepaTTS::sayTextWithSpeedToFile(char *toSay, int _speed, bool sayToOutputFile=false, const std::string& outputFileName="") 
{

    if(toSay == NULL || strlen(toSay) < 1) return;
    if(sayToOutputFile && outputFileName == "") return;
    
    speed = speedInRange(_speed);
    
    int err;
    std::string str;
    
    str = "LiepaTTS: sayText() START:\n";
    str += "LiepaTTS: tekstas="; str += toSay; str += "\n";
    printToLog(str.c_str(), "a+");

    char dest_str[strlen(toSay)+1];
    strcpy(dest_str, toSay);
    
	char *koduote = defaultKoduote;

    if( strcmp(koduote, "WINDOWS-1257") != 0 && strcmp(koduote, "CP1257") != 0) 
    {
        if(openConverter(koduote)) 
        {
            size_t inbytes = strlen(toSay);
            size_t outbytes = sizeof dest_str;

            char *out = dest_str;
    
            size_t nconv = iconv(conv_, &toSay, &inbytes, &out, &outbytes);

            if (nconv == (size_t) -1) {
                printToLog("LiepaTTS: WARNING Nepavyko teisingai pakeisti teksto koduotės, rezultatas gali būti netikslus.\n", "a+");
            } else {
                dest_str[sizeof dest_str - outbytes] = 0;
            }
            closeConverter();
        }
    }
	
	strcpy(tekstokopija, dest_str);
   
	int hr = 0;

	signbufsizet = signbufsize;
	evarrsizet = evarrsize;
	memset(signbuf, 0, signbufsize * sizeof(short));

	hr = synthesizeWholeText(dest_str, signbuf, &signbufsizet, evarr, &evarrsizet, speed, pitch);
	if(hr < 0)
	{
        str = "LiepaTTS ERROR: synthesizeWholeText()="; str += std::to_string(hr); str += "\n";
        printToLog(str.c_str(), "a+");
		return;
	}

    printToLog("LiepaTTS: synthesizeWholeText DONE.\n", "a+");
    if(signbufsizet < 1) 
    {
        printToLog("LiepaTTS: Sudaryta 0-linio ilgio garsų seka, neturiu ką įgarsinti.\n", "a+");
        return;
    }

    volume = volumeInRange(volume);
    if (volume < MAX_VOLUME) 
    {
        for(int i = 0; i < signbufsizet; i++) {
            signbuf[i] = signbuf[i] * volume / 100;
        }
    }

    
    if(!sayToOutputFile) 
    {
        almemory = _session->service("ALMemory");
        almemory.call<void>("raiseEvent", "ALTextToSpeech/TextStarted", true);
        printToLog("LiepaTTS: raiseEvent eventName=ALTextToSpeech/TextStarted.\n", "a+");

        snd_pcm_hw_params_t *hw_params;
        snd_pcm_sw_params_t *sw_params;
        snd_pcm_t * _soundDevice;

        err = initAlsa(_soundDevice, hw_params, sw_params);
        if (err < 0) 
        {
            printToLog("LiepaTTS: Audio init error.\n", "a+"); 
            return;
        }

        printToLog("LiepaTTS: Audio playback start.\n", "a+");
        err = playAlsa(_soundDevice, signbuf, sizeof(short), signbufsizet);

        closeAlsa(_soundDevice);
     
        almemory.call<void>("raiseEvent", "ALTextToSpeech/TextDone", true);
        printToLog("LiepaTTS: raiseEvent eventName=ALTextToSpeech/TextDone.\n", "a+");
    } else {
        FILE *outputfile;
        if(outputFileName!="")
            outputfile = fopen(outputFileName.c_str(), "wb");
        if(outputfile == 0 || outputfile == NULL) return;
        printToLog("LiepaTTS: Output to audio file *.wav start.\n", "a+");
        formuoti_wav_antraste(signbufsizet*sizeof(short), outputfile);
        fwrite(signbuf, sizeof(short), signbufsizet, outputfile);
        if(outputfile != NULL) fclose(outputfile);
        printToLog("LiepaTTS: Output to audio file *.wav done.\n", "a+");
    }
    return;
}

/** 
 * Say given text with speed specified by second parameter.
 * @param toSay Text to say, encoded in UTF-8.
 * @param _speed speed range [30(fast), 300(slow)], default 100.
 */
void LiepaTTS::sayTextWithSpeed(char *toSay, int _speed) 
{
    sayTextWithSpeedToFile(toSay, _speed, false, "");
}

/** 
 * Say given text.
 * @param toSay Text to say, encoded in UTF-8.
 */
void LiepaTTS::sayText(char *toSay) 
{
    sayTextWithSpeed(toSay, speed);
}

/** 
 * Say given text.
 * @param toSay Text to say, encoded in UTF-8.
 */
void LiepaTTS::say(char *toSay) 
{
    sayTextWithSpeed(toSay, speed);
}

/** 
 * Say given text and return the length of text.
 * @param toSay Text to say, encoded in UTF-8.
 * @return Length of text.
 */
int LiepaTTS::sayTextAndReturnLength(char* toSay) 
{
  sayTextWithSpeed(toSay, speed);
  return strlen(toSay);
}

/** 
 * Works similarly to LiepaTTS::say but the synthesized signal is recorded into the specified file instead of being sent to the robot’s loudspeakers. 
 * The signal is encoded with a sample rate of 22050 Hz, format S16_LE, 1 channel.
 * @param toSay Text to be synthesized, encoded in UTF-8.
 * @param fileName .wav file where the synthesized signal should be saved.
 */
void LiepaTTS::sayToFile(char *toSay, const std::string& fileName) 
{
    sayTextWithSpeedToFile(toSay, speed, true, fileName);
}

/**
 * Returns the list of the Lithuanian text to speech engine LiepaTTS voices installed on the system.
 * Available voices: vyr, mot (male and female).
 * @return Array of available voices.
 */
std::vector<std::string> LiepaTTS::getAvailableVoices() 
{
    std::vector<std::string> ret = {VOICE_VYR, VOICE_MOT};
    return ret;
}

/**
 * Returns the value of specified parameter of the Lithuanian text to speech engine LiepaTTS. 
 * Parameter names: speed, defaultVoiceSpeed, volume, pitch.
 * speed and defaultVoiceSpeed range [30(fast), 300(slow)], default value 100.
 * volume range [0, 100], default value 100.
 * pitch range [75(low), 133(high)], default=100.
 * @param parameter Name of the parameter (available names: speed, defaultVoiceSpeed, volume, pitch).
 * @return Value of the specified parameter.
 */
float LiepaTTS::getParameter(const std::string& parameter) 
{

    if (parameter == "speed") {
        return speed;
    }
    if (parameter == "defaultVoiceSpeed") {
        return defaultVoiceSpeed;
    }
    if (parameter == "volume") {
        return volume;
    }
    if (parameter == "pitch") {
        return pitch;
    }
    return 0;
}

/**
 * Returns the voice currently used by the Lithuanian text to speech engine LiepaTTS.
 * Available voices: vyr, mot (male and female).
 * @return	Name of the current voice.
 */
std::string LiepaTTS::getVoice() 
{
    return voice;
}

/**
 * Gets the current gain applied to the signal synthesized by the Lithuanian text to speech engine LiepaTTS. 
 * @return	Volume range [0, 100], default value 100.
 */
float LiepaTTS::getVolume() 
{
    return volume;
}

/**
 * Reset the speed of the speech to the value stored in the parameter defaultVoiceSpeed.
 */
void LiepaTTS::resetSpeed() 
{
    speed = defaultVoiceSpeed;
}

/**
 * Sets parameter value of the Lithuanian text to speech engine LiepaTTS.
 * Parameter names: speed, defaultVoiceSpeed, volume, pitch.
 * speed and defaultVoiceSpeed range [30(fast), 300(slow)], default value 100.
 * volume range [0, 100], default value 100.
 * pitch range [75(low), 133(high)], default value 100.
 * @param parameter – Name of the parameter (available names: speed, defaultVoiceSpeed, volume, pitch)
 * @param value – Value of the parameter
 */
void LiepaTTS::setParameter(const std::string& parameter, const float& value) 
{
    int value_i = static_cast<int>(value);
    if (parameter == "speed") 
        speed = speedInRange(value_i);
    
    if (parameter == "defaultVoiceSpeed") 
        defaultVoiceSpeed = speedInRange(value_i);
    
    if (parameter == "volume") 
        volume = volumeInRange(value_i);

    if (parameter == "pitch") 
        pitch = pitchInRange(value_i);
}

/**
 * Changes the voice used by the Lithuanian text-to-speech engine LiepaTTS. The voice identifier must belong to the installed voices, that can be listed using the LiepaTTS::getAvailableVoices method.
 * @param  voiceID Name of the voice. Available voices: vyr, mot (male and female).
 */
void LiepaTTS::setVoice(const std::string& voiceID) 
{
    if(voiceID == voice) return;
    
    if(voiceID == VOICE_VYR || voiceID == VOICE_MOT) {
        voice = voiceID;
        int hr;
        if(voice == VOICE_MOT)
            hr = initLUSS(projectDir, garsuDbDirMot); 
        else
            hr = initLUSS(projectDir, garsuDbDirVyr);
    }
}

/**
 * Sets the current gain applied to the signal synthesized by the Lithuanian text-to-speech engine LiepaTTS.
 * @param value Volume range [0, 100], default value 100.
 */
void LiepaTTS::setVolume(const float& value) 
{
    int value_i = static_cast<int>(value);
    volume = volumeInRange(value_i);
}

/**
 * Stop the current and pending tasks.
 */
void LiepaTTS::stopAll() 
{
    snd_pcm_t * _soundDevice;
    closeAlsaWithDrop(_soundDevice);
}

