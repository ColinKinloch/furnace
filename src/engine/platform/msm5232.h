/**
 * Furnace Tracker - multi-system chiptune tracker
 * Copyright (C) 2021-2022 tildearrow and contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _MSM5232_H
#define _MSM5232_H

#include "../dispatch.h"
#include <queue>
#include "../macroInt.h"
#include "sound/oki/msm5232.h"

class DivPlatformMSM5232: public DivDispatch {
  struct Channel {
    int freq, baseFreq, pitch, pitch2, note;
    int ins;
    bool active, insChanged, freqChanged, keyOn, keyOff, inPorta, noise;
    signed char vol, outVol;
    DivMacroInt std;
    void macroInit(DivInstrument* which) {
      std.init(which);
      pitch2=0;
    }
    Channel():
      freq(0),
      baseFreq(0),
      pitch(0),
      pitch2(0),
      note(0),
      ins(-1),
      active(false),
      insChanged(true),
      freqChanged(false),
      keyOn(false),
      keyOff(false),
      inPorta(false),
      noise(false),
      vol(127),
      outVol(127) {}
  };
  Channel chan[8];
  DivDispatchOscBuffer* oscBuf[8];
  int partVolume[8];
  int initPartVolume[8];
  double capacitance[8];
  bool isMuted[8];
  bool updateGroup[2];
  bool updateGroupAR[2];
  bool updateGroupDR[2];
  bool groupEnv[2];
  unsigned char groupControl[2];
  unsigned char groupAR[2];
  unsigned char groupDR[2];
  struct QueuedWrite {
      unsigned char addr;
      unsigned char val;
      QueuedWrite(unsigned char a, unsigned char v): addr(a), val(v) {}
  };
  std::queue<QueuedWrite> writes;

  int cycles, curChan, delay, detune;
  short temp[16];
  msm5232_device* msm;
  unsigned char regPool[128];
  friend void putDispatchChip(void*,int);
  friend void putDispatchChan(void*,int,int);
  public:
    void acquire(short* bufL, short* bufR, size_t start, size_t len);
    int dispatch(DivCommand c);
    void* getChanState(int chan);
    DivMacroInt* getChanMacroInt(int ch);
    DivDispatchOscBuffer* getOscBuffer(int chan);
    unsigned char* getRegisterPool();
    int getRegisterPoolSize();
    void reset();
    void forceIns();
    void tick(bool sysTick=true);
    void muteChannel(int ch, bool mute);
    bool isStereo();
    bool keyOffAffectsArp(int ch);
    void setFlags(const DivConfig& flags);
    void notifyInsDeletion(void* ins);
    void poke(unsigned int addr, unsigned short val);
    void poke(std::vector<DivRegWrite>& wlist);
    const char** getRegisterSheet();
    int init(DivEngine* parent, int channels, int sugRate, const DivConfig& flags);
    void quit();
    ~DivPlatformMSM5232();
};

#endif
