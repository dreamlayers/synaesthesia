/* Synaesthesia - program to display sound graphically
   Copyright (C) 1997  Paul Francis Harrison
   Copyright (C) 2017  Boris Gjenero

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  675 Mass Ave, Cambridge, MA 02139, USA.

  The author may be contacted at:
    pfh@yoyo.cc.monash.edu.au
  or
    27 Bond St., Mt. Waverley, 3149, Melbourne, Australia
*/

#include <sys/time.h>
#include <pthread.h>
#include <glib.h>
#include <libaudcore/i18n.h>
#include <libaudcore/plugin.h>
#include <libaudcore/drct.h>
#include "syna.h"

void *synaesthesiaMain(void *arg);

// Buffer for real samples and zeroes
static sampleType realdata[2*NumSamples], fakedata[2*NumSamples];

bool quitFlag;

pthread_t synthread;

bool gotdata;
pthread_mutex_t gotdata_mtx;
pthread_cond_t gotdata_cond;

bool havedata;
pthread_mutex_t havedata_mtx;
pthread_cond_t havedata_cond;

class Synaesthesia : public VisPlugin
{
public:
    static constexpr PluginInfo info = {
        N_("Synaesthesia"),
        N_("standalone")
    };

    constexpr Synaesthesia () : VisPlugin (info, Visualizer::MultiPCM) {}

    bool init ();
    void cleanup ();

    void render_multi_pcm (const float * pcm, int channels);
    void clear ();
};


__attribute__((visibility("default"))) Synaesthesia aud_plugin_instance;

bool Synaesthesia::init(void)
{
    gotdata = false;
    havedata = false;
    quitFlag = false;

    pthread_mutex_init(&gotdata_mtx, NULL);
    pthread_cond_init(&gotdata_cond, NULL);
    pthread_mutex_init(&havedata_mtx, NULL);
    pthread_cond_init(&havedata_cond, NULL);

    if (pthread_create(&synthread, NULL, &synaesthesiaMain, NULL)) {
        quitFlag = true;
        cleanup();
        return false;
    }

    return true;
}

void Synaesthesia::cleanup(void)
{

    if (!quitFlag) {
        // Tell thread to quit and wait for it
        pthread_mutex_lock(&havedata_mtx);
        quitFlag = true;
        havedata = true;
        pthread_cond_signal(&havedata_cond);
        pthread_mutex_unlock(&havedata_mtx);
    }

    pthread_join(synthread, NULL);

    pthread_mutex_destroy(&gotdata_mtx);
    pthread_cond_destroy(&gotdata_cond);
    pthread_mutex_destroy(&havedata_mtx);
    pthread_cond_destroy(&havedata_cond);
}

void Synaesthesia::render_multi_pcm(const float * pcm, int channels)
{
    pthread_mutex_lock(&gotdata_mtx);
    while (!gotdata && !quitFlag) {
        pthread_cond_wait(&gotdata_cond, &gotdata_mtx);
    }
    gotdata = false;
    pthread_mutex_unlock(&gotdata_mtx);

    if (quitFlag) {
        /* There should be a better way to get current plugin handle. */
        /* These functions are not thread safe, so must be called from here. */
        PluginHandle *ph = aud_plugin_lookup_basename("synaesthesia");
        if (ph) aud_plugin_enable(ph, false);
        return;
    }

    if (channels >= 2) {
        for(int i=0; i<NumSamples; i++) {
            realdata[i*2] = pcm[i * channels] * 32768.0;
            realdata[i*2+1] = pcm[i * channels + 1] * 32768.0;
        }
    } else if (channels == 1) {
        // Synaesthesia is not designed for mono
        for(int i=0; i<NumSamples; i++) {
            realdata[i*2+1] = realdata[i*2] = pcm[i]  * 32768.0;
        }
    } else {
        return;
    }

    pthread_mutex_lock(&havedata_mtx);
    havedata = true;
    pthread_cond_signal(&havedata_cond);
    pthread_mutex_unlock(&havedata_mtx);
}

void Synaesthesia::clear(void)
{
}


void setupMixer(double &loudness) {
    /* TODO: Could use aud_drct_get_volume_main() / 100.0,
       but that is not thread safe. */
    loudness = 1.0;
}

void setVolume(double loudness) {
    /* TODO: Could use aud_drct_set_volume_main() / 100.0,
       but that is not thread safe. */
}

void openSound(SoundSource source, int inFrequency, char *dspName,
               char *mixerName) {
    memset(fakedata, 0, NumSamples * 2 * sizeof(sampleType));
    data = fakedata;
}

void closeSound() {
    pthread_mutex_lock(&gotdata_mtx);
    quitFlag = true;
    pthread_cond_signal(&gotdata_cond);
    pthread_mutex_unlock(&gotdata_mtx);

    data = NULL;
}

int getNextFragment(void) {
    if (quitFlag) return -1;

    // It's easiest to do this here.
    // It decreases lag but may also decrease frame rate
    pthread_mutex_lock(&gotdata_mtx);
    gotdata = true;
    pthread_cond_signal(&gotdata_cond);
    pthread_mutex_unlock(&gotdata_mtx);

    if (quitFlag) return -1;

    struct timeval tp;
    struct timespec tmo;

    gettimeofday(&tp, NULL);
    tmo.tv_nsec = tp.tv_usec * 1000 + 100000000;
    tmo.tv_sec = tp.tv_sec;
    if (tmo.tv_nsec >= 1000000000) {
        tmo.tv_nsec -= 1000000000;
        tmo.tv_sec++;
    }

    pthread_mutex_lock(&havedata_mtx);
    pthread_cond_timedwait(&havedata_cond, &havedata_mtx, &tmo);
    if (havedata) {
        havedata = false;
        data = realdata;
    } else {
        data = fakedata;
    }
    pthread_mutex_unlock(&havedata_mtx);

    return 0;
}
