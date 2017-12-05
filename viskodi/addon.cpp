#ifdef __GNUC__
#define __cdecl
#endif

#include <kodi/xbmc_vis_types.h>
#include <kodi/xbmc_vis_dll.h>

#include <cstdlib>
#include <cmath>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#include "../syna.h"

bool initialized = false;

bool warnGiven = false;

float audio_data[513];
float audio_data_freq[513];

void *synaesthesiaMain(void *arg);
void synaesthesiaLoop(void);

extern "C" ADDON_STATUS ADDON_Create (void* hdl, void* props)
{
    if (!props)
        return ADDON_STATUS_UNKNOWN;

    //VIS_PROPS* visProps = (VIS_PROPS*) props;

    if (!initialized) {
//        if (!rgbm_init())
            //return ADDON_STATUS_UNKNOWN;
        synaesthesiaMain(NULL);
        initialized = true;
    }


    return ADDON_STATUS_NEED_SETTINGS;
}

extern "C" void Start (int, int, int, const char*)
{
}

static bool havedata = false;

extern "C" void AudioData (const float *pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
    if (initialized) {
        const unsigned int channels = 2;
        printf("ADL: %i\n", iAudioDataLength);
        for(int i=0; i<NumSamples; i++) {
            data[i*2] = pAudioData[i * channels] * 32768.0;
            data[i*2+1] = pAudioData[i * channels + 1] * 32768.0;
        }
        havedata = true;
    }
#if 0
    if ((unsigned int)iAudioDataLength > sizeof(audio_data)/sizeof(*audio_data)-1) {
        if (!warnGiven) {
            fprintf(stderr, "Unexpected audio data length received (%d), expect incorrect vis\n", iAudioDataLength);
            warnGiven = true;
        }
        return;
    }

    if ((unsigned int)iFreqDataLength > sizeof(audio_data_freq)/sizeof(*audio_data_freq)-1) {
        if (!warnGiven) {
            fprintf(stderr, "Unexpected freq data length received (%d), expect incorrect vis\n", iAudioDataLength);
            warnGiven = true;
        }
        return;
    }

    for (unsigned long i=0; i<sizeof(audio_data)/sizeof(*audio_data); i++) { audio_data[i] = 0; }
    for (unsigned long i=0; i<sizeof(audio_data_freq)/sizeof(*audio_data_freq); i++) { audio_data_freq[i] = 0; }

    memcpy(audio_data, pAudioData, iAudioDataLength*sizeof(float));
    memcpy(audio_data_freq, pFreqData, iFreqDataLength*sizeof(float));
#endif
}

extern "C" void Render()
{
    if (havedata) {
        synaesthesiaLoop();
        havedata = false;
    }
}

extern "C" void GetInfo (VIS_INFO* pInfo)
{
    pInfo->bWantsFreq = true;
    pInfo->iSyncDelay = 0;
}

extern "C" bool OnAction (long flags, const void *param)
{
    return false;
}

extern "C" unsigned int GetPresets (char ***presets)
{
    return 0;
}

extern "C" unsigned GetPreset()
{
    return 0;
}

extern "C" bool IsLocked()
{
    return false;
}

extern "C" unsigned int GetSubModules (char ***names)
{
    return 0;
}

extern "C" void ADDON_Stop()
{
    return;
}

extern "C" void ADDON_Destroy()
{
    if (initialized) {
// FIXME
        initialized = false;
    }
    return;
}

extern "C" bool ADDON_HasSettings()
{
    return false;
}

extern "C" ADDON_STATUS ADDON_GetStatus()
{
    //    return ADDON_STATUS_UNKNOWN;

    return ADDON_STATUS_OK;
}

extern "C" unsigned int ADDON_GetSettings (ADDON_StructSetting ***sSet)
{
    return 0;
}

extern "C" void ADDON_FreeSettings()
{
}

extern "C" ADDON_STATUS ADDON_SetSetting (const char *strSetting, const void* value)
{
    return ADDON_STATUS_OK;
}

extern "C" void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}

void setupMixer(double &loudness) {
    loudness = 1.0;
}

void setVolume(double loudness) {
}

void openSound(SoundSource source, int inFrequency, char *dspName,
               char *mixerName) {
//    memset(fakedata, 0, NumSamples * 2 * sizeof(sampleType));
//    data = fakedata;
    static sampleType realdata[2*NumSamples];
    data = realdata;
}

void closeSound() {
}

int getNextFragment(void) {
//    if (quitFlag) return -1;
    return 0;
}
