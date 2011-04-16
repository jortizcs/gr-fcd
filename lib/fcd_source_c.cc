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
#include <gr_io_signature.h>
#include <gr_audio_source.h>
#include <gr_float_to_complex.h>


/*
 * Create a new instance of fcd_source_c and return
 * a boost shared_ptr. This is effectively the public constructor.
 */
fcd_source_c_sptr fcd_make_source_c(const std::string device_name)
{
    return gnuradio::get_initial_sptr(new fcd_source_c(device_name));
}


static const int MIN_IN = 0;	// mininum number of input streams
static const int MAX_IN = 0;	// maximum number of input streams
static const int MIN_OUT = 1;	// minimum number of output streams
static const int MAX_OUT = 1;	// maximum number of output streams


fcd_source_c::fcd_source_c(const std::string device_name) 
    : gr_hier_block2 ("fcd_source_c",
                      gr_make_io_signature (MIN_IN, MAX_IN, sizeof (gr_complex)),
                      gr_make_io_signature (MIN_OUT, MAX_OUT, sizeof (gr_complex))),
    d_freq_corr(115)
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

    f *= 1.0 + d_freq_corr/1000000.0;

    fme = fcdAppSetFreqkHz((int)(f/1000.0));
    /* TODO: check fme */
}
    
// Set frequency with kHz resolution.
void fcd_source_c::set_freq_khz(int freq)
{
    FCD_MODE_ENUM fme;
    double f = freq*1000.0;

    f *= 1.0 + d_freq_corr/1000000.0;

    fme = fcdAppSetFreqkHz((int)(f/1000.0));
    /* TODO: check fme */
}

// Set new frequency correction
void fcd_source_c::set_freq_corr(int ppm)
{
    d_freq_corr = ppm;
}
