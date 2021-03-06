/* -*- c++ -*- */
/*
 * Copyright 2011 Free Software Foundation, Inc.
 * 
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <fcd_source_c.h>
#include <fcd.h>
#include <fcdhidcmd.h> // needed for extended API
#include <gr_io_signature.h>
#include <gr_audio_source.h>
#include <gr_float_to_complex.h>

//#include <iostream>
//using namespace std;

/*
 * Create a new instance of fcd_source_c and return
 * a boost shared_ptr. This is effectively the public constructor.
 */
fcd_source_c_sptr fcd_make_source_c(const std::string device_name)
{
    return gnuradio::get_initial_sptr(new fcd_source_c(device_name));
}


static const int MIN_IN = 0;  /*!< Mininum number of input streams. */
static const int MAX_IN = 0;  /*!< Maximum number of input streams. */
static const int MIN_OUT = 1; /*!< Minimum number of output streams. */
static const int MAX_OUT = 1; /*!< Maximum number of output streams. */


fcd_source_c::fcd_source_c(const std::string device_name) 
    : gr_hier_block2 ("fcd_source_c",
                      gr_make_io_signature (MIN_IN, MAX_IN, sizeof (gr_complex)),
                      gr_make_io_signature (MIN_OUT, MAX_OUT, sizeof (gr_complex))),
    d_freq_corr(-120),
    d_freq_req(0)
{
    gr_float_to_complex_sptr f2c;
    
    /* Audio source; sample rate fixed at 96kHz */
    fcd = audio_make_source(96000, device_name, true);
    
    /* block to convert stereo audio to a complex stream */
    f2c = gr_make_float_to_complex(1);

    connect(fcd, 0, f2c, 0);
    connect(fcd, 1, f2c, 1);
    connect(f2c, 0, self(), 0);

}


fcd_source_c::~fcd_source_c ()
{

}

// Set frequency with Hz resolution
void fcd_source_c::set_freq(int freq)
{
    FCD_MODE_ENUM fme;
    double f = (double)freq;
    
    /* valid range 50 MHz - 2.0 GHz */
    if ((freq < 50000000) || (freq > 2000000000)) {
        return;
    }

    d_freq_req = freq;
    f *= 1.0 + d_freq_corr/1000000.0;

    fme = fcdAppSetFreq((int)f);
    /* TODO: check fme */
}

// Set frequency with Hz resolution (type float)
void fcd_source_c::set_freq(float freq)
{
    FCD_MODE_ENUM fme;
    double f = (double)freq;
    
    /* valid range 50 MHz - 2.0 GHz */
    if ((freq < 50.0e6) || (freq > 2.0e9)) {
        return;
    }

    d_freq_req = (int)freq;
    f *= 1.0 + d_freq_corr/1000000.0;

    fme = fcdAppSetFreq((int)f);
    /* TODO: check fme */
}

    
// Set frequency with kHz resolution.
void fcd_source_c::set_freq_khz(int freq)
{
    FCD_MODE_ENUM fme;
    double f = freq*1000.0;

    /* valid range 50 MHz - 2.0 GHz */
    if ((freq < 50000) || (freq > 2000000)) {
        return;
    }

    d_freq_req = freq*1000;
    f *= 1.0 + d_freq_corr/1000000.0;

    fme = fcdAppSetFreqkHz((int)(f/1000.0));
    /* TODO: check fme */
}


// Set LNA gain
void fcd_source_c::set_lna_gain(float gain)
{
    FCD_MODE_ENUM fme;
    unsigned char g;
    
    /* convert to nearest discrete value */
    if (gain > 27.5) {
        g = 14;              // 30.0 dB
    }
    else if (gain > 22.5) {
        g = 13;              // 25.0 dB
    }
    else if (gain > 18.75) {
        g = 12;              // 20.0 dB
    }
    else if (gain > 16.25) {
        g = 11;              // 17.5 dB
    }
    else if (gain > 13.75) {
        g = 10;              // 15.0 dB
    }
    else if (gain > 11.25) {
        g = 9;               // 12.5 dB
    }
    else if (gain > 8.75) {
        g = 8;               // 10.0 dB
    }
    else if (gain > 6.25) {
        g = 7;               // 7.5 dB
    }
    else if (gain > 3.75) {
        g = 6;               // 5.0 dB
    }
    else if (gain > 1.25) {
        g = 5;               // 2.5 dB
    }
    else if (gain > -1.25) {
        g = 4;               // 0.0 dB
    }
    else if (gain > -3.75) {
        g = 1;               // -2.5 dB
    }
    else {
        g = 0;               // -5.0 dB
    }
    
    fme = fcdAppSetParam(FCD_CMD_APP_SET_LNA_GAIN, &g, 1);
    /* TODO: check fme */
}

// Set new frequency correction
void fcd_source_c::set_freq_corr(int ppm)
{
    d_freq_corr = ppm;
    // re-tune with new correction value
    set_freq(d_freq_req);
}


// Set DC offset correction.
void fcd_source_c::set_dc_corr(double _dci, double _dcq)
{
    union {
        unsigned char auc[4];
        struct {
            signed short dci;  // equivalent of qint16 which should be 16 bit everywhere
            signed short dcq;
        };
    } dcinfo;

    if ((_dci < -1.0) || (_dci > 1.0) || (_dcq < -1.0) || (_dcq > 1.0))
        return;

    dcinfo.dci = static_cast<signed short>(_dci*32768.0);
    dcinfo.dcq = static_cast<signed short>(_dcq*32768.0);

    fcdAppSetParam(FCD_CMD_APP_SET_DC_CORR, dcinfo.auc, 4);

}


// Set IQ phase and gain balance.
void fcd_source_c::set_iq_corr(double _gain, double _phase)
{
    union {
        unsigned char auc[4];
        struct {
            signed short phase;
            signed short gain;
        };
    } iqinfo;

    if ((_gain < -1.0) || (_gain > 1.0) || (_phase < -1.0) || (_phase > 1.0))
        return;

    iqinfo.phase = static_cast<signed short>(_phase*32768.0);
    iqinfo.gain = static_cast<signed short>(_gain*32768.0);

    fcdAppSetParam(FCD_CMD_APP_SET_IQ_CORR, iqinfo.auc, 4);

}
