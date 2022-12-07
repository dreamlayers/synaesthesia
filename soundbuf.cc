/* Synaesthesia - program to display sound graphically
   Copyright (C) 1997  Paul Francis Harrison
   Copyright (C) 2019  Boris Gjenero <boris.gjenero@gmail.com>

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

  The original author may be contacted at:
    pfh@yoyo.cc.monash.edu.au
  or
    27 Bond St., Mt. Waverley, 3149, Melbourne, Australia
*/

#include <SDL.h>
#include "syna.h"

#define WITH_SDL
#define RING_SIZE (NumSamples*2)
/* #define CONVERT_SAMPLES(x) ((x) / 32768.0) */
#undef CONVERT_SAMPLES
typedef int16_t sndbufInType;
typedef int16_t sndbufOutType;
#define SNDBUF_DEST data
#define SNDBUF_FUNC int getNextFragment(void)

/*
 * Incoming samples go into a ring buffer protected by a mutex.
 * Next incoming sample goes to ring[ring_write], and ring_write wraps.
 * When enough samples have arrived sound_retrieve() copies from the ring
 * buffer, protected by that mutex, which prevents new samples from
 * overwriting. It uses ring_write to know how data is wrapped in the
 * ring buffer.
 */
static sndbufOutType ring[RING_SIZE];
static unsigned int ring_write = 0;
#ifdef WITH_SDL
static SDL_mutex *mutex = NULL;
static SDL_cond *cond = NULL;
#else /* Use pthreads */
static pthread_mutex_t mutex;
static pthread_cond_t cond;
#endif
static int signalled = 0;
static unsigned int min_new = 0;

/* Number of samples put into ring buffer since last sound_retrieve() call is
 * counted by ring_has. If visualizer is slower than input, ring_has could be
 * more than RING_SIZE, which is useful for timing.
 */
static unsigned int ring_has = 0;

void sndbuf_init(unsigned int min_new_samples)
{
    min_new = min_new_samples;
#ifdef WITH_SDL
    mutex = SDL_CreateMutex();
    if (mutex == NULL) error("creating SDL_mutex for sound buffer");
    cond = SDL_CreateCond();
    if (cond == NULL) error("creating SDL_cond for sound buffer");
#else /* Use pthreads */
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
#endif
}

void sndbuf_quit(void)
{
#ifdef WITH_SDL
    SDL_DestroyMutex(mutex);
    mutex = NULL;
    SDL_DestroyCond(cond);
    cond = NULL;
#else /* Use pthreads */
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
#endif
}

static inline void sndbuf_lock(void)
{
#ifdef WITH_SDL
    SDL_LockMutex(mutex);
#else /* Use pthreads */
    pthread_mutex_lock(&mutex);
#endif
}

static inline void sndbuf_unlock(void)
{
#ifdef WITH_SDL
    SDL_UnlockMutex(mutex);
#else /* Use pthreads */
    pthread_mutex_unlock(&mutex);
#endif
}

void sndbuf_store(const int16_t *input, unsigned int len)
{
#ifdef CONVERT_SAMPLES
    int i;
#endif
    const sndbufInType *ip;
    sndbufOutType *op;
    unsigned int remain;

    sndbuf_lock();

    if (len >= RING_SIZE) {
        /* If there are too many samples, fill buffer with latest ones */
        ip = input + len - RING_SIZE;
        op = &(ring[0]);
        remain = RING_SIZE;

        ring_write = 0;
    } else {
        int new_ring_write;
        remain = RING_SIZE - ring_write;
        if (remain > len) {
            remain = len;
            new_ring_write = ring_write + len;
        } else if (remain == len) {
            new_ring_write = 0;
        } else {
            /* Do part of store after wrapping around ring */
            new_ring_write = len - remain;
            ip = input + remain;
            op = &(ring[0]);
#ifdef CONVERT_SAMPLES
            for (i = 0; i < new_ring_write; i++) {
                *(op++) = CONVERT_SAMPLES(*(ip++));
            }
#else
            memcpy(op, ip, new_ring_write * sizeof(sndbufInType));
#endif
        }

        ip = input;
        op = &(ring[ring_write]);

        /* Advance write pointer */
        ring_write = new_ring_write;
    }

    /* Do part of store before wrapping point, maybe whole store */
#ifdef CONVERT_SAMPLES
    for (i = 0; i < remain; i++) {
        *(op++) = CONVERT_SAMPLES(*(ip++));
    }
#else
    memcpy(op, ip, remain * sizeof(sndbufInType));
#endif

    /* This could theoretically overflow, but is needed for timing */
    ring_has += len;

    if (ring_has >= min_new && !signalled) {
        /* Next buffer is full */
        signalled = 1;
#ifdef WITH_SDL
        SDL_CondSignal(cond);
#else /* Use pthreads */
        pthread_cond_signal(&cond);
#endif
    }
    sndbuf_unlock();
}

SNDBUF_FUNC
{
    unsigned int chunk1, ring_had;

    sndbuf_lock();

    /* Wait for next buffer to be full */
    while (!signalled) {
#ifdef WITH_SDL
        SDL_CondWait(cond, mutex);
#else /* Use pthreads */
        pthread_cond_wait(&cond, &mutex);
#endif
    }
    signalled = 0;

    /* Copy first part, up to wrapping point */
    chunk1 = RING_SIZE - ring_write;
    memcpy(&SNDBUF_DEST[0], &ring[ring_write],
           sizeof(sndbufOutType) * chunk1);
    if (ring_write > 0) {
        /* Samples are wrapped around the ring. Copy the second part. */
        memcpy(&SNDBUF_DEST[chunk1], &ring[0],
               sizeof(sndbufOutType) * ring_write);
    }

    ring_had = ring_has;
    ring_has = 0;

    /* After swap the buffer shouldn't be touched anymore by interrupts */
    sndbuf_unlock();

    return ring_had;
}
