/* Synaesthesia - program to display sound graphically
   Copyright (C) 1997  Paul Francis Harrison

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

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#if !defined (__FreeBSD__) && !defined(__FreeBSD_kernel__)
#include <linux/soundcard.h>
#include <linux/cdrom.h>
//#include <linux/ucdrom.h>
#else
#include <sys/soundcard.h>
#include <sys/cdio.h>
#define CDROM_LEADOUT 0xAA
#define CD_FRAMES 75 /* frames per second */
#define CDROM_DATA_TRACK 0x4
#endif
#include <time.h>

#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "syna.h"

#if HAVE_LIBESD
#include <esd.h>
#endif

static int cdDevice;

static int trackCount = 0, *trackFrame = 0;

void cdOpen(char *cdromName) {
  //6/7/2000 Some CDs seem to need to be opened nonblocking.
  //attempt(cdDevice = open(cdromName,O_RDONLY),"talking to CD device",true);
  attempt(cdDevice = open(cdromName,O_RDONLY|O_NONBLOCK),"talking to CD device",true);
}

void cdClose(void) {
  close(cdDevice);

  delete[] trackFrame;
}

//void cdConfigure(void) {
//#ifdef HAVE_UCDROM
//  if (!cdOpen()) return;
//  int options = (CDO_AUTO_EJECT|CDO_AUTO_CLOSE);
//  attemptNoDie(ioctl(cdDevice, CDROM_CLEAR_OPTIONS, options),
//    "configuring CD behaviour");
//  cdClose();
//#endif
//}

void getTrackInfo(void) {
  delete trackFrame;
  trackFrame = 0;
  trackCount  = 0;

#if !defined (__FreeBSD__) && !defined(__FreeBSD_kernel__)
  cdrom_tochdr cdTochdr;
  if (-1 == ioctl(cdDevice, CDROMREADTOCHDR, &cdTochdr))
#else
  ioc_toc_header cdTochdr;
  if (-1 == ioctl(cdDevice, CDIOREADTOCHEADER, (char *)&cdTochdr))
#endif
     return;
#if !defined (__FreeBSD__) && !defined(__FreeBSD_kernel__)
  trackCount = cdTochdr.cdth_trk1;
#else
  trackCount = cdTochdr.ending_track - cdTochdr.starting_track + 1;
#endif

  int i;
  trackFrame = new int[trackCount+1];
  for(i=trackCount;i>=0;i--) {
#if !defined (__FreeBSD__) && !defined(__FreeBSD_kernel__)
    cdrom_tocentry cdTocentry;
    cdTocentry.cdte_format = CDROM_MSF;
    cdTocentry.cdte_track  = (i == trackCount ? CDROM_LEADOUT : i+1);
#else
    cd_toc_entry cdTocentry;
    struct ioc_read_toc_entry t;
    t.address_format = CD_MSF_FORMAT;
    t.starting_track = (i == trackCount ? CDROM_LEADOUT : i+1);
    t.data_len = sizeof(struct cd_toc_entry);
    t.data = &cdTocentry;
#endif

    //Bug fix: thanks to Ben Gertzfield  (9/7/98)
    //Leadout track is sometimes reported as data.
    //Added check for this.
#if !defined (__FreeBSD__) && !defined(__FreeBSD_kernel__)
    if (-1 == ioctl(cdDevice, CDROMREADTOCENTRY, & cdTocentry) ||
        (i != trackCount && (cdTocentry.cdte_ctrl & CDROM_DATA_TRACK)))
      trackFrame[i] = (i==trackCount?0:trackFrame[i+1]);
    else
      trackFrame[i] = cdTocentry.cdte_addr.msf.minute*60*CD_FRAMES+
                      cdTocentry.cdte_addr.msf.second*CD_FRAMES+
                      cdTocentry.cdte_addr.msf.frame;
#else
    if ((ioctl(cdDevice, CDIOREADTOCENTRYS, (char *) &t) == -1) ||
     (i != trackCount && (cdTocentry.control & CDROM_DATA_TRACK)))
	     trackFrame[i] = (i==trackCount?0:trackFrame[i+1]);
    else
	trackFrame[i] = cdTocentry.addr.msf.minute*60*CD_FRAMES+
			    cdTocentry.addr.msf.second*CD_FRAMES+
			    cdTocentry.addr.msf.frame;
#endif
  }
}

int cdGetTrackCount(void) {
  return trackCount;
}
int cdGetTrackFrame(int track) {
  if (!trackFrame)
    return 0; 
  else if (track <= 1 || track > trackCount+1)
    return trackFrame[0];
  else
    return trackFrame[track-1]; 
}

void cdPlay(int frame, int endFrame) {
#if !defined (__FreeBSD__) && !defined(__FreeBSD_kernel__)
  cdrom_msf msf;
#else
  struct ioc_play_msf msf;
#endif
  if (frame < 1) frame = 1;
  if (endFrame < 1) {
    if (!trackFrame) return; 
    endFrame = trackFrame[trackCount];
  }
  
  //Some CDs can't change tracks unless not playing.
  // (Sybren Stuvel)
  cdStop();
  
#if !defined (__FreeBSD__) && !defined(__FreeBSD_kernel__)
  msf.cdmsf_min0 = frame / (60*CD_FRAMES);
  msf.cdmsf_sec0 = frame / CD_FRAMES % 60;
  msf.cdmsf_frame0 = frame % CD_FRAMES;
#else
  msf.start_m = frame / (60*CD_FRAMES);
  msf.start_s = frame / CD_FRAMES % 60;
  msf.start_f = frame % CD_FRAMES;
#endif

  //Bug fix: thanks to Martin Mitchell
  //An out by one error that affects some CD players. 
  //Have to use endFrame-1 rather than endFrame (9/7/98)
#if !defined (__FreeBSD__) && !defined(__FreeBSD_kernel__)
  msf.cdmsf_min1 = (endFrame-1) / (60*CD_FRAMES);
  msf.cdmsf_sec1 = (endFrame-1) / CD_FRAMES % 60;
  msf.cdmsf_frame1 = (endFrame-1) % CD_FRAMES;
  attemptNoDie(ioctl(cdDevice, CDROMPLAYMSF, &msf),"playing CD",true);
#else
  msf.end_m = (endFrame-1) / (60*CD_FRAMES);
  msf.end_s = (endFrame-1) / CD_FRAMES % 60;
  msf.end_f = (endFrame-1) % CD_FRAMES;
  attemptNoDie(ioctl(cdDevice, CDIOCPLAYMSF, (char *) &msf),"playing CD",true);
#endif
}

void cdGetStatus(int &track, int &frames, SymbolID &state) {
#if !defined (__FreeBSD__) && !defined(__FreeBSD_kernel__)
  cdrom_subchnl subchnl;
  subchnl.cdsc_format = CDROM_MSF;
  if (-1 == ioctl(cdDevice, CDROMSUBCHNL, &subchnl)) {
#else
  ioc_read_subchannel subchnl;
  struct cd_sub_channel_info info;

  subchnl.data = &info;
  subchnl.data_len = sizeof (info);
  subchnl.address_format = CD_MSF_FORMAT;
  subchnl.data_format = CD_CURRENT_POSITION;
  
  if (-1 == ioctl(cdDevice, CDIOCREADSUBCHANNEL, (char *) &subchnl)) {
#endif
    track = 1; 
    frames  = 0;
    state = (state == Open ? Open : NoCD); /* ? */
    return;
  }
#if !defined (__FreeBSD__) && !defined(__FreeBSD_kernel__)
  track = subchnl.cdsc_trk;
  frames  = subchnl.cdsc_reladdr.msf.minute*60*CD_FRAMES+
            subchnl.cdsc_reladdr.msf.second*CD_FRAMES+
            subchnl.cdsc_reladdr.msf.frame; 
#else
  track = subchnl.data->what.position.track_number;
  frames  = subchnl.data->what.position.reladdr.msf.minute*60*CD_FRAMES+
		subchnl.data->what.position.reladdr.msf.second*CD_FRAMES+
		subchnl.data->what.position.reladdr.msf.frame; 
#endif
  
  SymbolID oldState = state;
#if !defined (__FreeBSD__) && !defined(__FreeBSD_kernel__)
  switch(subchnl.cdsc_audiostatus) {
    case CDROM_AUDIO_PAUSED    : state = Pause; break;
    case CDROM_AUDIO_PLAY      : state = Play; break;
    case CDROM_AUDIO_COMPLETED : state = Stop; break;
    case CDROM_AUDIO_NO_STATUS : state = Stop; break;
    case CDROM_AUDIO_INVALID   : state = Stop; break;
#else
  switch(subchnl.data->header.audio_status) {
    case CD_AS_PLAY_PAUSED	     : state = Pause; break;
    case CD_AS_PLAY_IN_PROGRESS    : state = Play; break;
    case CD_AS_PLAY_COMPLETED 	     : state = Stop; break;
    case CD_AS_NO_STATUS	     : state = Stop; break;
    case CD_AS_AUDIO_INVALID       : state = Stop; break;
#endif
    default : state = NoCD; break;
  }

  if ((oldState == NoCD || oldState == Open) &&
      (state == Pause || state == Play || state == Stop)) {
    getTrackInfo();
  }
  
  if (state != Play && state != Pause) {
    track = 1;
    frames = 0;
  }
}

void cdStop(void) {
  //attemptNoDie(ioctl(cdDevice, CDROMSTOP),"stopping CD");
#if !defined (__FreeBSD__) && !defined(__FreeBSD_kernel__)
  ioctl(cdDevice, CDROMSTOP);
#else
  ioctl(cdDevice, CDIOCSTOP);
#endif
}
void cdPause(void) {
#if !defined (__FreeBSD__) && !defined(__FreeBSD_kernel__)
  attemptNoDie(ioctl(cdDevice, CDROMPAUSE),"pausing CD",true);
#else
  attemptNoDie(ioctl(cdDevice, CDIOCPAUSE),"pausing CD",true);
#endif
}
void cdResume(void) {
#if !defined (__FreeBSD__) && !defined(__FreeBSD_kernel__)
  attemptNoDie(ioctl(cdDevice, CDROMRESUME),"resuming CD",true);
#else
  attemptNoDie(ioctl(cdDevice, CDIOCRESUME),"resuming CD",true);
#endif
}
void cdEject(void) {
#if !defined (__FreeBSD__) && !defined(__FreeBSD_kernel__)
  attemptNoDie(ioctl(cdDevice, CDROMEJECT),"ejecting CD",true);
#else
  attemptNoDie(ioctl(cdDevice, CDIOCEJECT),"ejecting CD",true);
#endif
}
void cdCloseTray(void) {
#if !defined (__FreeBSD__) && !defined(__FreeBSD_kernel__)
  attemptNoDie(ioctl(cdDevice, CDROMCLOSETRAY),"ejecting CD",true);
#else
  attemptNoDie(ioctl(cdDevice, CDIOCCLOSE),"ejecting CD",true);
#endif
}

/* Sound Recording ================================================= */

#ifdef LITTLEENDIAN
#define SOUNDFORMAT AFMT_S16_LE
#else
#define SOUNDFORMAT AFMT_S16_BE
#endif

//If kernel starts running out of sound memory playing mp3s, this could
//be the problem. OTOH if it is too small, it will start ticking on slow
//computers
#define MAXWINDOWSIZE 32

//Used for various buffers
#define BUFFERSIZE (1<<18)

static SoundSource source;
static int inFrequency, downFactor, windowSize, pipeIn, device;

// pipe stuff
//static int16_t *dataIn;
//static int dataInAvailable; // current size of data available
//static int feederFd; // fd of pipe to feeded process
char pipeBuffer[BUFFERSIZE];
int pipeBufferUsed, pipeBufferDelay;

static char *mixer;

int readWholeBlock(int pipe,char *dest,int length) {
  while(length > 0) {
    int result = read(pipe,dest,length);
    if (result < 1)
      return -1;
    dest += result;
    length -= result;
  }
  return 0;
}

int writeWholeBlock(int pipe,char *src,int length) {
  while(length > 0) {
    int result = write(pipe,src,length);
    if (result < 1)
      return -1;
    src += result;
    length -= result;
  }
  return 0;
}

int makePiper(void) {
  int p[2];
  attempt(pipe(p), "making pipe");

  if (!fork()) {
    close(p[0]);

    static char buffer[BUFFERSIZE];
    int bufferUsed = 0;
    int device_p = 0;
    bool doneReading = false;

    while(!doneReading || bufferUsed >= 256) {
      fd_set readers, writers;
      FD_ZERO(&readers);
      if (bufferUsed < BUFFERSIZE-256) FD_SET(0, &readers);
      FD_ZERO(&writers);
      if (device_p >= 4) FD_SET(p[1], &writers);
      if (bufferUsed-device_p >= 256) FD_SET(device, &writers);

      select((p[1]<device?device:p[1])+1, &readers, &writers, 0, 0);
      
      if (FD_ISSET(device, &writers) && bufferUsed-device_p >= 256) {
        int n = write(device, buffer+device_p, 256);

        if (n < 1)
          break;

        device_p += n;
      }

      if (FD_ISSET(0, &readers) && bufferUsed < BUFFERSIZE-256) {
        int n = read(0, buffer+bufferUsed, 256);

        if (n < 1)
          doneReading = true;
        
        bufferUsed += n;
      }

      if (FD_ISSET(p[1], &writers) && device_p >= 4) {
        static char b2[BUFFERSIZE];
        int b2Used = 0;
        for(int i=0;i<device_p;i += downFactor*4) {
          *(uint32_t*)(b2+b2Used) = *(uint32_t*)(buffer+i);
          b2Used += 4;
        }
        
        int n = write(p[1], b2, b2Used);
          
        if (n < 1)
          break;

        bufferUsed -= n*downFactor;
        device_p -= n*downFactor;
        memmove(buffer, buffer+n*downFactor, bufferUsed);
      }
    }

    exit(0);
  }

  close(p[1]);
  return p[0];
}

void setupMixer(double &loudness) {
  int device = open(mixer,O_WRONLY);
  if (device == -1) 
    warning("opening mixer device");
  else {
    if (source != SourcePipe) {
      int blah = (source == SourceCD ? SOUND_MASK_CD : SOUND_MASK_LINE + SOUND_MASK_MIC);
      attemptNoDie(ioctl(device,SOUND_MIXER_WRITE_RECSRC,&blah),"writing to mixer",true);
    }

    if (source == SourceCD) {
      int volume;
      attemptNoDie(ioctl(device,SOUND_MIXER_READ_CD,&volume),"writing to mixer",true);
      loudness = ((volume&0xff) + volume/256) / 200.0; 
    } else if (source == SourceLine) {
      int volume = 100*256 + 100;
      attemptNoDie(ioctl(device,SOUND_MIXER_READ_LINE,&volume),"writing to mixer",true);
      //attemptNoDie(ioctl(device,SOUND_MIXER_READ_MIC,&volume),"writing to mixer");
      loudness = ((volume&0xff) + volume/256) / 200.0; 
    } else 
      loudness = 1.0;
    close(device);
  }
}

void setVolume(double loudness) {
  int device = open(mixer,O_WRONLY);
  if (device == -1) 
    warning("opening mixer device",true);
  else {
    int scaledLoudness = int(loudness * 100.0);
    if (scaledLoudness < 0) scaledLoudness = 0;
    if (scaledLoudness > 100) scaledLoudness = 100;
    scaledLoudness = scaledLoudness*256 + scaledLoudness;
    if (source == SourceCD) {
      attemptNoDie(ioctl(device,SOUND_MIXER_WRITE_CD,&scaledLoudness),"writing to mixer",true);
    } else if (source == SourceLine) {
      attemptNoDie(ioctl(device,SOUND_MIXER_WRITE_LINE,&scaledLoudness),"writing to mixer",true);
      attemptNoDie(ioctl(device,SOUND_MIXER_WRITE_MIC,&scaledLoudness),"writing to mixer",true);
    } else {
      attemptNoDie(ioctl(device,SOUND_MIXER_WRITE_PCM,&scaledLoudness),"writing to mixer",true);
    }
    close(device);
  }
} 

void openSound(SoundSource source, int inFrequency, char *dspName, 
               char *mixerName) {
  ::source = source;
  ::inFrequency = inFrequency; 
  ::windowSize = 1;
  pipeBufferUsed = 0;
  pipeBufferDelay = 0;
  mixer = mixerName;

  if (source == SourceESD) {
#if HAVE_LIBESD
    attempt(pipeIn = esd_monitor_stream(
	    ESD_BITS16|ESD_STEREO|ESD_STREAM|ESD_PLAY,Frequency,0,PACKAGE),
            "connecting to EsounD");

    int esd;
    attempt(esd = esd_open_sound(0), "querying esd latency");
    attempt(pipeBufferDelay = esd_get_latency(esd_open_sound(0)), "querying esd latency");
    esd_close(esd);

    pipeBufferDelay = int( double(pipeBufferDelay) * Frequency * 2 / 44100 );
    // there should be an extra factor of two here... but it looks wrong
    // if i put it in... hmm
    
#endif
  } else {
    downFactor = inFrequency / Frequency; 
    if (downFactor <= 0)
      downFactor = 1;
  
    int format, stereo, fragment, fqc;
  
#if defined(__FreeBSD__) || defined(_FreeBSD_kernel__)
    attempt(device = open(dspName,O_WRONLY),"opening dsp device",true);
    format = SOUNDFORMAT;
    attempt(ioctl(device,SNDCTL_DSP_SETFMT,&format),"setting format",true);
    if (format != SOUNDFORMAT) error("setting format (2)");
    close(device);
#endif
    if (source == SourcePipe)
      attempt(device = open(dspName,O_WRONLY),"opening dsp device",true);
    else
      attempt(device = open(dspName,O_RDONLY),"opening dsp device",true);
      
    format = SOUNDFORMAT;
    fqc = (source == SourcePipe ? inFrequency : Frequency);
    stereo = 1;
    
    //if (source == SourcePipe)
      //fragment = 0x00010000*(MAXWINDOWSIZE+1) + (LogSize-Overlap+1);
    //else
      //Added extra fragments to allow recording overrun (9/7/98)
      //8 fragments of size 2*(2^(LogSize-Overlap+1)) bytes
    //  fragment = 0x00080000 + (LogSize-Overlap+1); 
      
    fragment = 0x0003000e;
    attemptNoDie(ioctl(device,SNDCTL_DSP_SETFRAGMENT,&fragment),"setting fragment",true);
    
#if !defined (__FreeBSD__) && !defined(__FreeBSD_kernel__)
    attempt(ioctl(device,SNDCTL_DSP_SETFMT,&format),"setting format",true);
    if (format != SOUNDFORMAT) error("setting format (2)");
#endif
    attempt(ioctl(device,SNDCTL_DSP_STEREO,&stereo),"setting stereo",true);
    attemptNoDie(ioctl(device,SNDCTL_DSP_SPEED,&fqc),"setting frequency",true);
  
    if (source == SourcePipe) {
      //dataIn = new int16_t[NumSamples*2*downFactor*MAXWINDOWSIZE];
      //memset(dataIn,0,NumSamples*4*downFactor*MAXWINDOWSIZE);
      //dataInAvailable = NumSamples*4*downFactor*windowSize;
      //pipeIn = 0;//dup(0);

      pipeIn = makePiper();
    } else {
      pipeIn = device;
    }
  }
     
  fcntl(pipeIn, F_SETFL, O_NONBLOCK);

  data = new int16_t[NumSamples*2];  
  memset((char*)data,0,NumSamples*4);
}

void closeSound() {
  delete data;

  close(pipeIn);
}

int getNextFragment(void) {
  // Linux wasn't designed for real time piping
  // hence, this section is a bit hacked...
  static int step = 4096;

  bool end = false, any = false;

  // This seems to be necessary. Not sure why.
  usleep(1000);

  // Read all data in pipe
  while(pipeBufferUsed < BUFFERSIZE) {
    int result = read(pipeIn,pipeBuffer+pipeBufferUsed,BUFFERSIZE-pipeBufferUsed);
    if (result == 0)
      end = true;
    if (result < 1)
      break;
    pipeBufferUsed += result;
  }
  
  //printf("%d\n",pipeBufferUsed);
  //if (pipeBufferUsed == BUFFERSIZE) {
  //    delay += 10000;
  //}
    
  int eat = pipeBufferUsed - pipeBufferDelay;

  if (eat < step && step > 256) step -= 128;
  else if (eat > step * 16) step += 128;

  if (eat > 0) {
    if (eat > step) eat = step;

    if (eat > NumSamples*4)
      memcpy((char*)data, pipeBuffer+eat-NumSamples*4, NumSamples*4);
    else {
      memmove((char*)data,(char*)data+eat,NumSamples*4-eat);
      memcpy((char*)data+NumSamples*4-eat,pipeBuffer,eat);
    }

    pipeBufferUsed -= eat;
    memmove(pipeBuffer,pipeBuffer+eat,pipeBufferUsed);
  }

  return end && eat <= 0 ? -1 : 0;
}

