#include "nes.h"
#include "sound/nes/cpu_inline.h"
#include "../engine.h"
#include <cstddef>
#include <math.h>

#define CHIP_DIVIDER 16

#define rWrite(a,v) if (!skipRegisterWrites) {apu_wr_reg(nes,a,v); if (dumpWrites) {addWrite(a,v);} }

const char* regCheatSheetNES[]={
  "S0Volume", "4000",
  "S0Sweep", "4001",
  "S0PeriodL", "4002",
  "S0PeriodH", "4003",
  "S1Volume", "4004",
  "S1Sweep", "4005",
  "S1PeriodL", "4006",
  "S1PeriodH", "4007",
  "TRVolume", "4008",
  "TRPeriodL", "400A",
  "TRPeriodH", "400B",
  "NSVolume", "400C",
  "NSPeriod", "400E",
  "NSLength", "400F",
  "DMCControl", "4010",
  "DMCLoad", "4011",
  "DMCAddr", "4012",
  "DMCLength", "4013",
  "APUControl", "4015",
  "APUFrameCtl", "4017",
  NULL
};

const char** DivPlatformNES::getRegisterSheet() {
  return regCheatSheetNES;
}

void DivPlatformNES::acquire(short* bufL, short* bufR, size_t start, size_t len) {
  for (size_t i=start; i<start+len; i++) {
    if (dacSample!=-1) {
      dacPeriod+=dacRate;
      if (dacPeriod>=rate) {
        DivSample* s=parent->song.sample[dacSample];
        if (s->rendLength>0) {
          if (!isMuted[4]) {
            if (s->depth==8) {
              rWrite(0x4011,((unsigned char)s->rendData[dacPos]+0x80)>>1);
            } else {
              rWrite(0x4011,((unsigned short)s->rendData[dacPos]+0x8000)>>9);
            }
          }
          if (++dacPos>=s->rendLength) {
            if (s->loopStart>=0 && s->loopStart<=(int)s->rendLength) {
              dacPos=s->loopStart;
            } else {
              dacSample=-1;
            }
          }
          dacPeriod-=rate;
        } else {
          dacSample=-1;
        }
      }
    }
  
    apu_tick(nes,NULL);
    nes->apu.odd_cycle=!nes->apu.odd_cycle;
    if (nes->apu.clocked) {
      nes->apu.clocked=false;
    }
    bufL[i]=(pulse_output(nes)+tnd_output(nes))*30;
  }
}

static unsigned char noiseTable[256]={
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 5, 4,
  15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4,
  15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4,
  3, 2, 1, 0, 11, 10, 9, 8, 7, 6, 5, 4,
  15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4,
  3, 2, 1, 0, 11, 10, 9, 8, 7, 6, 5, 4,
  15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4,
  3, 2, 1, 0, 11, 10, 9, 8, 7, 6, 5, 4,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  15
};

void DivPlatformNES::tick() {
  for (int i=0; i<4; i++) {
    chan[i].std.next();
    if (chan[i].std.hadVol) {
      // ok, why are the volumes like that?
      chan[i].outVol=MIN(15,chan[i].std.vol)-(15-(chan[i].vol&15));
      if (chan[i].outVol<0) chan[i].outVol=0;
      if (i==2) { // triangle
        rWrite(0x4000+i*4,(chan[i].outVol==0)?0:255);
        chan[i].freqChanged=true;
      } else {
        rWrite(0x4000+i*4,0x30|chan[i].outVol|((chan[i].duty&3)<<6));
      }
    }
    if (chan[i].std.hadArp) {
      if (i==3) { // noise
        if (chan[i].std.arpMode) {
          chan[i].baseFreq=chan[i].std.arp;
        } else {
          chan[i].baseFreq=chan[i].note+chan[i].std.arp;
        }
        if (chan[i].baseFreq>255) chan[i].baseFreq=255;
        if (chan[i].baseFreq<0) chan[i].baseFreq=0;
      } else {
        if (!chan[i].inPorta) {
          if (chan[i].std.arpMode) {
            chan[i].baseFreq=NOTE_PERIODIC(chan[i].std.arp);
          } else {
            chan[i].baseFreq=NOTE_PERIODIC(chan[i].note+chan[i].std.arp);
          }
        }
      }
      chan[i].freqChanged=true;
    } else {
      if (chan[i].std.arpMode && chan[i].std.finishedArp) {
        chan[i].baseFreq=NOTE_PERIODIC(chan[i].note);
        chan[i].freqChanged=true;
      }
    }
    if (chan[i].std.hadDuty) {
      chan[i].duty=chan[i].std.duty;
      if (i==3 && chan[i].duty>1) chan[i].duty=1;
      if (i!=2) {
        rWrite(0x4000+i*4,0x30|chan[i].outVol|((chan[i].duty&3)<<6));
      }
      if (i==3) { // noise
        chan[i].freqChanged=true;
      }
    }
    if (chan[i].sweepChanged) {
      chan[i].sweepChanged=false;
      if (i==0) {
        //rWrite(16+i*5,chan[i].sweep);
      }
    }
    if (chan[i].freqChanged || chan[i].keyOn || chan[i].keyOff) {
      if (i==3) { // noise
        chan[i].freq=noiseTable[chan[i].baseFreq];
      } else {
        chan[i].freq=parent->calcFreq(chan[i].baseFreq,chan[i].pitch,true);
        if (chan[i].freq>2047) chan[i].freq=2047;
      }
      if (chan[i].keyOn) {
        //rWrite(16+i*5+1,((chan[i].duty&3)<<6)|(63-(ins->gb.soundLen&63)));
        //rWrite(16+i*5+2,((chan[i].vol<<4))|(ins->gb.envLen&7)|((ins->gb.envDir&1)<<3));
      }
      if (chan[i].keyOff) {
        //rWrite(16+i*5+2,8);
        if (i==2) { // triangle
          rWrite(0x4000+i*4,0x00);
        } else {
          rWrite(0x4000+i*4,0x30);
        }
      }
      if (i==3) { // noise
        rWrite(0x4002+i*4,(chan[i].duty<<7)|chan[i].freq);
        rWrite(0x4003+i*4,0xf0);
      } else {
        rWrite(0x4002+i*4,chan[i].freq&0xff);
        if ((chan[i].prevFreq>>8)!=(chan[i].freq>>8) || i==2) {
          rWrite(0x4003+i*4,0xf8|(chan[i].freq>>8));
        }
        if (chan[i].freq!=65535 && chan[i].freq!=0) {
          chan[i].prevFreq=chan[i].freq;
        }
      }
      if (chan[i].keyOn) chan[i].keyOn=false;
      if (chan[i].keyOff) chan[i].keyOff=false;
      chan[i].freqChanged=false;
    }
  }

  // PCM
  if (chan[4].freqChanged) {
    chan[4].freq=parent->calcFreq(chan[4].baseFreq,chan[4].pitch,false);
    if (chan[4].furnaceDac) {
      double off=1.0;
      if (dacSample>=0 && dacSample<parent->song.sampleLen) {
        DivSample* s=parent->song.sample[dacSample];
        off=(double)s->centerRate/8363.0;
      }
      dacRate=MIN(chan[4].freq*off,32000);
      if (dumpWrites) addWrite(0xffff0001,dacRate);
    }
    chan[4].freqChanged=false;
  }
}

int DivPlatformNES::dispatch(DivCommand c) {
  switch (c.cmd) {
    case DIV_CMD_NOTE_ON:
      if (c.chan==4) { // PCM
        DivInstrument* ins=parent->getIns(chan[c.chan].ins);
        if (ins->type==DIV_INS_AMIGA) {
          dacSample=ins->amiga.initSample;
          if (dacSample<0 || dacSample>=parent->song.sampleLen) {
            dacSample=-1;
            if (dumpWrites) addWrite(0xffff0002,0);
            break;
          } else {
            if (dumpWrites) addWrite(0xffff0000,dacSample);
          }
          dacPos=0;
          dacPeriod=0;
          chan[c.chan].baseFreq=parent->song.tuning*pow(2.0f,((float)(c.value+3)/12.0f));
          if (c.value!=DIV_NOTE_NULL) {
            chan[c.chan].freqChanged=true;
            chan[c.chan].note=c.value;
          }
          chan[c.chan].active=true;
          chan[c.chan].keyOn=true;
          chan[c.chan].furnaceDac=true;
        } else {
          if (c.value!=DIV_NOTE_NULL) {
            chan[c.chan].note=c.value;
          }
          dacSample=12*sampleBank+chan[c.chan].note%12;
          if (dacSample>=parent->song.sampleLen) {
            dacSample=-1;
            if (dumpWrites) addWrite(0xffff0002,0);
            break;
          } else {
            if (dumpWrites) addWrite(0xffff0000,dacSample);
          }
          dacPos=0;
          dacPeriod=0;
          dacRate=parent->song.sample[dacSample]->rate;
          if (dumpWrites) addWrite(0xffff0001,dacRate);
          chan[c.chan].furnaceDac=false;
        }
        break;
      } else if (c.chan==3) { // noise
        if (c.value!=DIV_NOTE_NULL) {
          chan[c.chan].baseFreq=c.value;
        }
      } else {
        if (c.value!=DIV_NOTE_NULL) {
          chan[c.chan].baseFreq=NOTE_PERIODIC(c.value);
        }
      }
      if (c.value!=DIV_NOTE_NULL) {
        chan[c.chan].freqChanged=true;
        chan[c.chan].note=c.value;
      }
      chan[c.chan].active=true;
      chan[c.chan].keyOn=true;
      chan[c.chan].std.init(parent->getIns(chan[c.chan].ins));
      if (c.chan==2) {
        rWrite(0x4000+c.chan*4,0xff);
      } else {
        rWrite(0x4000+c.chan*4,0x30|chan[c.chan].vol|((chan[c.chan].duty&3)<<6));
      }
      break;
    case DIV_CMD_NOTE_OFF:
      if (c.chan==4) {
        dacSample=-1;
        if (dumpWrites) addWrite(0xffff0002,0);
      }
      chan[c.chan].active=false;
      chan[c.chan].keyOff=true;
      chan[c.chan].std.init(NULL);
      break;
    case DIV_CMD_INSTRUMENT:
      if (chan[c.chan].ins!=c.value || c.value2==1) {
        chan[c.chan].ins=c.value;
      }
      break;
    case DIV_CMD_VOLUME:
      if (chan[c.chan].vol!=c.value) {
        chan[c.chan].vol=c.value;
        if (!chan[c.chan].std.hasVol) {
          chan[c.chan].outVol=c.value;
        }
        if (chan[c.chan].active) {
          if (c.chan==2) {
            rWrite(0x4000+c.chan*4,0xff);
          } else {
            rWrite(0x4000+c.chan*4,0x30|chan[c.chan].vol|((chan[c.chan].duty&3)<<6));
          }
        }
      }
      break;
    case DIV_CMD_GET_VOLUME:
      return chan[c.chan].vol;
      break;
    case DIV_CMD_PITCH:
      chan[c.chan].pitch=c.value;
      chan[c.chan].freqChanged=true;
      break;
    case DIV_CMD_NOTE_PORTA: {
      int destFreq=NOTE_PERIODIC(c.value2);
      bool return2=false;
      if (destFreq>chan[c.chan].baseFreq) {
        chan[c.chan].baseFreq+=c.value;
        if (chan[c.chan].baseFreq>=destFreq) {
          chan[c.chan].baseFreq=destFreq;
          return2=true;
        }
      } else {
        chan[c.chan].baseFreq-=c.value;
        if (chan[c.chan].baseFreq<=destFreq) {
          chan[c.chan].baseFreq=destFreq;
          return2=true;
        }
      }
      chan[c.chan].freqChanged=true;
      if (return2) {
        chan[c.chan].inPorta=false;
        return 2;
      }
      break;
    }
    case DIV_CMD_STD_NOISE_MODE:
      chan[c.chan].duty=c.value;
      if (c.chan==3) { // noise
        chan[c.chan].freqChanged=true;
      }
      break;
    case DIV_CMD_NES_SWEEP:
      if (c.chan>1) break;
      if (c.value2==0) {
        chan[c.chan].sweep=0x08;
      } else {
        if (!c.value) { // down
          chan[c.chan].sweep=0x88|(c.value2&0x77);
        } else { // up
          chan[c.chan].sweep=0x80|(c.value2&0x77);
        }
      }
      rWrite(0x4001+(c.chan*4),chan[c.chan].sweep);
      break;
    case DIV_CMD_SAMPLE_BANK:
      sampleBank=c.value;
      if (sampleBank>(parent->song.sample.size()/12)) {
        sampleBank=parent->song.sample.size()/12;
      }
      break;
    case DIV_CMD_LEGATO:
      if (c.chan==3) break;
      chan[c.chan].baseFreq=NOTE_PERIODIC(c.value+((chan[c.chan].std.willArp && !chan[c.chan].std.arpMode)?(chan[c.chan].std.arp):(0)));
      chan[c.chan].freqChanged=true;
      chan[c.chan].note=c.value;
      break;
    case DIV_CMD_PRE_PORTA:
      chan[c.chan].std.init(parent->getIns(chan[c.chan].ins));
      chan[c.chan].inPorta=c.value;
      break;
    case DIV_CMD_GET_VOLMAX:
      return 15;
      break;
    case DIV_ALWAYS_SET_VOLUME:
      return 1;
      break;
    default:
      break;
  }
  return 1;
}

void DivPlatformNES::muteChannel(int ch, bool mute) {
  isMuted[ch]=mute;
  rWrite(0x4015,(!isMuted[0])|((!isMuted[1])<<1)|((!isMuted[2])<<2)|((!isMuted[3])<<3)|((!isMuted[4])<<4));
  if (isMuted[4]) {
    rWrite(0x4011,0);
  }
}

void DivPlatformNES::forceIns() {
  for (int i=0; i<5; i++) {
    chan[i].insChanged=true;
    chan[i].prevFreq=65535;
  }
  rWrite(0x4001,chan[0].sweep);
  rWrite(0x4005,chan[1].sweep);
}

void* DivPlatformNES::getChanState(int ch) {
  return &chan[ch];
}

void DivPlatformNES::reset() {
  for (int i=0; i<5; i++) {
    chan[i]=DivPlatformNES::Channel();
  }
  if (dumpWrites) {
    addWrite(0xffffffff,0);
  }

  dacPeriod=0;
  dacPos=0;
  dacRate=0;
  dacSample=-1;
  sampleBank=0;

  apu_turn_on(nes,apuType);
  nes->apu.cpu_cycles=0;
  nes->apu.cpu_opcode_cycle=0;

  rWrite(0x4015,(!isMuted[0])|((!isMuted[1])<<1)|((!isMuted[2])<<2)|((!isMuted[3])<<3)|((!isMuted[4])<<4));
  rWrite(0x4001,chan[0].sweep);
  rWrite(0x4005,chan[1].sweep);
}

bool DivPlatformNES::keyOffAffectsArp(int ch) {
  return true;
}

void DivPlatformNES::setFlags(unsigned int flags) {
  if (flags==2) { // Dendy
    rate=COLOR_PAL*2.0/5.0;
    apuType=2;
    nes->apu.type=apuType;
  } else if (flags==1) { // PAL
    rate=COLOR_PAL*3.0/8.0;
    apuType=1;
    nes->apu.type=apuType;
  } else { // NTSC
    rate=COLOR_NTSC/2.0;
    apuType=0;
    nes->apu.type=apuType;
  }
  chipClock=rate;
}

void DivPlatformNES::notifyInsDeletion(void* ins) {
  for (int i=0; i<5; i++) {
    chan[i].std.notifyInsDeletion((DivInstrument*)ins);
  }
}

void DivPlatformNES::poke(unsigned int addr, unsigned short val) {
  rWrite(addr,val);
}

void DivPlatformNES::poke(std::vector<DivRegWrite>& wlist) {
  for (DivRegWrite& i: wlist) rWrite(i.addr,i.val);
}

int DivPlatformNES::init(DivEngine* p, int channels, int sugRate, unsigned int flags) {
  parent=p;
  apuType=flags;
  dumpWrites=false;
  skipRegisterWrites=false;
  for (int i=0; i<5; i++) {
    isMuted[i]=false;
  }
  nes=new struct NESAPU;
  setFlags(flags);

  init_nla_table(500,500);
  reset();
  return 5;
}

void DivPlatformNES::quit() {
  delete nes;
}

DivPlatformNES::~DivPlatformNES() {
}
