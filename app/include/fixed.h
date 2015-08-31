/*
Copyright (C) 2014-2015 Thiemar Pty Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <cfloat>
#include <cmath>

#if defined(CONFIG_STM32_STM32F302)
    /* STM32F30X-specific CMSIS defs */
    #include "cmsis_stm32f302.h"
#elif defined(CONFIG_STM32_STM32F446)
    /* STM32F44X-specific CMSIS defs */
    #include "cmsis_stm32f446.h"
#endif /* defined(CONFIG_STM32_STM32F302) */

#include "core_cm4.h"


#ifndef M_PI
#define M_PI 3.141592653589793f
#endif


struct motor_state_t {
    float angular_acceleration_rad_per_s2;
    float angular_velocity_rad_per_s;
    float angle_rad; /* 0 .. 2 * pi */
    float i_dq_a[2];
};


struct motor_params_t {
    float rs_r; /* winding resistance in ohms */
    float ls_h; /* winding inductance in ohms */
    float phi_v_s_per_rad; /* speed constant in volt-seconds per radian */

    /* Operating limits */
    float max_current_a; /* RMS current limit in amps */
    float max_voltage_v; /* RMS voltage limit in volts */
    float accel_voltage_v; /* RMS initial open-loop voltage */
    float idle_speed_rad_per_s; /* Speed to spin at when armed but with zero
                                   setpoint, in rad/s */
    float spinup_rate_rad_per_s2; /* Rate at which to increase the motor speed
                                     when starting up. */

    uint32_t num_poles; /* number of poles */

    /* Inertia of the motor and prop */
    float rotor_inertia_kg_m2;
    /* Motor drag torque */
    float drag_torque_n_m_s2_per_rad2;
};


struct control_params_t {
    float bandwidth_hz;
    float accel_gain;
    float accel_time_s;
};


inline float __attribute__((optimize("O3"),always_inline))
fast_expf(float x) {
/*
fast_expf is copyright (C) 2011 Paul Mineiro
All rights reserved.

Redistribution and use in source and binary forms, with
or without modification, are permitted provided that the
following conditions are met:

    * Redistributions of source code must retain the
    above copyright notice, this list of conditions and
    the following disclaimer.

    * Redistributions in binary form must reproduce the
    above copyright notice, this list of conditions and
    the following disclaimer in the documentation and/or
    other materials provided with the distribution.

    * Neither the name of Paul Mineiro nor the names
    of other contributors may be used to endorse or promote
    products derived from this software without specific
    prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

Contact: Paul Mineiro <paul@mineiro.com>
*/
  float p = x * 1.442695040f;
  float offset = (p < 0) ? 1.0f : 0.0f;
  float clipp = (p < -126) ? -126.0f : p;
  int32_t w = (int32_t)clipp;
  float z = clipp - (float)w + offset;
  union { uint32_t i; float f; } v = { (uint32_t)((1 << 23) * (clipp + 121.2740575f + 27.7280233f / (4.84252568f - z) - 1.49012907f * z)) };

  return v.f;
}


inline void __attribute__((optimize("O3"),always_inline))
sin_cos(
    float& sinx,
    float& cosx,
    float x /* x must be in the range [-pi, pi] */
) {
    const float Q = 3.1f;
    const float P = 3.6f;

    float y;

    x *= float(1.0 / M_PI);

    y = x - x * std::abs(x);
    sinx = y * (Q + P * std::abs(y));

    /* Calculate the cosine */
    x += 0.5f;
    if (x > 1.0f) {
        x -= 2.0f;
    }

    y = x - x * std::abs(x);
    cosx = y * (Q + P * std::abs(y));
}


inline float __attribute__((optimize("O3"),always_inline))
__VSQRTF(float x) {
    float result;
    asm ("vsqrt.f32 %0, %1" : "=w" (result) : "w" (x) );
    return result;
}
