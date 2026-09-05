/*
 * This file is part of the cordic project.
 * Copyright 2026 Edward V. Emelianov <edward.emelianoff@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>
#include <stdint.h>

#include "astro.h"
#include "cordic.h"

static int sincosflag = 0; // 0: math.h, 1: cordic
// longitude/latitude + in rad/hrs
//static float longitude = 41.44143375f, latitude = 43.6535278f;
static float lat_rad = DEG2RAD(43.6535278f);
static float long_hrs = DEG2HOURS(41.44143375f);

static void sincosf_m(float angle, float *s, float *c){
    if(s) *s = sin(angle);
    if(c) *c = cos(angle);
}

static void (*sincosf)(float, float*, float*) = sincosf_m;

void set_sincos(int iscordic){
    if(iscordic) sincosf = cordic_sincos;
    else sincosf = sincosf_m;
    sincosflag = iscordic;
}

int get_sincos(){ return sincosflag; }

/* Helper functions for angle normalization (single precision) */
static float normalize_degrees(float angle){
    angle = fmodf(angle, 360.0f);
    if (angle < 0.0f) angle += 360.0f;
    return angle;
}

static float normalize_hours(float hours){
    hours = fmodf(hours, 24.0f);
    if (hours < 0.0f) hours += 24.0f;
    return hours;
}

/* 1. Compute Modified Julian Date from UNIX time (seconds since 1970-01-01 00:00:00 UTC) */
float MJD_from_unix(uint32_t t){
    return 40587.f + (float)t / 86400.0f;
}

float LST_from_unix(uint32_t t){
    uint32_t days = t / 86400;
    uint32_t sec  = t % 86400;

    float mjd_int = 40587.0f + (float)days;

    float T = (mjd_int - 51544.5f) / 36525.0f, T2 = T*T, T3 = T2*T;
    float gmst0_sec = 24110.54841f + 8640184.812866f*T + 0.093104f*T2 - 6.2e-6f*T3;
    float ut1_sec = (float)sec * 1.00273790935f;
    float gmst_sec = gmst0_sec + ut1_sec;
    float lst_hours = gmst_sec / 3600.0f + long_hrs;
    return normalize_hours(lst_hours);
}

/* 3. Convert Hour Angle (HA) to Right Ascension (RA) and vice versa.
   All angles in degrees. LST is Local Sidereal Time in degrees. */
float ha_to_ra(float ha, float lst_deg){
    float ra = lst_deg - ha;
    return normalize_degrees(ra);
}

float ra_to_ha(float ra, float lst_deg){
    float ha = lst_deg - ra;
    // Hour angle is usually in range [-180,180)
    ha = normalize_degrees(ha);
    if (ha > 180.0f) ha -= 360.0f;
    return ha;
}

/* 4. Convert Altitude-Azimuth coordinates to Equatorial (Hour Angle, Declination)
   and back. All angles in degrees. Azimuth is measured from North through East. */
void altaz_to_hadec(float alt_deg, float az_deg, float *ha_deg, float *dec_deg){
    float alt = DEG2RAD(alt_deg);
    float az  = DEG2RAD(az_deg);

    float sin_alt, cos_alt, sin_az, cos_az, sin_lat, cos_lat;
    sincosf(alt, &sin_alt, &cos_alt);
    sincosf(az,  &sin_az,  &cos_az);
    sincosf(lat_rad, &sin_lat, &cos_lat);

    /* Declination */
    float sin_dec = sin_alt * sin_lat + cos_alt * cos_lat * cos_az;
    float dec = asinf(sin_dec);

    /* Hour angle (using atan2f for sign determination) */
    float x = sin_alt * cos_lat - cos_alt * sin_lat * cos_az;
    float y = -cos_alt * sin_az;
    float ha = atan2f(y, x);   // radians

    *ha_deg  = RAD2DEG(ha);
    *dec_deg = RAD2DEG(dec);
}

void hadec_to_altaz(float ha_deg, float dec_deg, float *alt_deg, float *az_deg){
    float ha  = DEG2RAD(ha_deg);
    float dec = DEG2RAD(dec_deg);

    float sin_dec, cos_dec, sin_ha, cos_ha, sin_lat, cos_lat;
    sincosf(dec, &sin_dec, &cos_dec);
    sincosf(ha,  &sin_ha,  &cos_ha);
    sincosf(lat_rad, &sin_lat, &cos_lat);

    /* Altitude */
    float sin_alt = sin_lat * sin_dec + cos_lat * cos_dec * cos_ha;
    float alt = asinf(sin_alt);

    /* Azimuth (from North through East) */
    float x = sin_dec * cos_lat - cos_dec * sin_lat * cos_ha;
    float y = -cos_dec * sin_ha;
    float az = atan2f(y, x);   // radians

    *alt_deg = DEG2RAD(alt);
    *az_deg  = DEG2RAD(az);
    if (*az_deg < 0.0f) *az_deg += 360.0f;
}

/**
 * @brief Calculate refraction constants A and B for the model dZ = A*tan(Z) + B*tan^3(Z)
 *
 * This is a single-precision adaptation of the SOFA iauRefco / ERFA eraRefco function.
 * Optimized for microcontrollers without hardware double-precision support (e.g., STM32G431).
 *
 * @param phpa    Pressure at the observer (hPa = mbar)
 * @param tc      Ambient temperature at the observer (degrees C)
 * @param rh      Relative humidity at the observer (range 0-1)
 * @param wl      Wavelength (micrometers). Use 0.55 for optical, >100 for radio.
 * @param refa    Output: tan(Z) coefficient (radians)
 * @param refb    Output: tan^3(Z) coefficient (radians)
 */
void refco_f32(float phpa, float tc, float rh, float wl, float *refa, float *refb) {
    // Restrict input parameters to safe values (clamp)
    float t = tc;
    if(t < -150.0f) t = -150.0f;
    if(t > 200.0f)  t = 200.0f;

    float p = phpa;
    if(p < 0.0f)     p = 0.0f;
    if(p > 10000.0f) p = 10000.0f;

    float r = rh;
    if(r < 0.0f)    r = 0.0f;
    if(r > 1.0f)    r = 1.0f;

    float w = wl;
    if(w < 0.1f)  w = 0.1f;
    if(w > 10.0f) w = 10.0f;

    // Water vapour pressure at the observer
    float pw = 0.0f;
    if(p > 0.0f){
        // Saturation vapour pressure (empirical formula)
        float ps = powf(10.0f, (0.7859f + 0.03477f * t) / (1.0f + 0.00412f * t))
                   * (1.0f + p * (4.5e-6f + 6e-10f * t * t));
        pw = r * ps / (1.0f - (1.0f - r) * ps / p);
    }

    // Temperature in Kelvin
    float tk = t + 273.15f;

    // Refractive index minus 1 at the observer (gamma = (n - 1) at the observer)
    float gamma;
    // Optical/IR: wavelength-dependent formula
    float wlsq = w * w;
    gamma = ((77.53484e-6f + (4.39108e-7f + 3.666e-9f / wlsq) / wlsq) * p
             - 11.2684e-6f * pw) / tk;

    // Beta coefficient (from Stone, with empirical adjustments)
    float beta = 4.4474e-6f * tk;

    // Refraction constants (from Green)
    if(refa) *refa = gamma * (1.0f - beta);
    if(refb) *refb = -gamma * (beta - gamma / 2.0f);
}

#if 0
void refco_f32(float phpa, float tc, float rh, float wl, float *refa, float *refb){
    // Restrict input parameters to safe values (clamp)
    float t = tc;
    if(t < -150.0f) t = -150.0f;
    if(t > 200.0f)  t = 200.0f;

    float p = phpa;
    if(p < 0.0f)     p = 0.0f;
    if(p > 10000.0f) p = 10000.0f;

    float r = rh;
    if(r < 0.0f)    r = 0.0f;
    if(r > 1.0f)    r = 1.0f;

    float w = wl;
    if(w < 0.1f)  w = 0.1f;
    if(w > 10.0f) w = 10.0f;

    // Water vapour pressure at the observer
    float pw = 0.0f;
    if(p > 0.0f){
        // Saturation vapour pressure (empirical formula)
        float ps = powf(10.0f, (0.7859f + 0.03477f * t) / (1.0f + 0.00412f * t))
                   * (1.0f + p * (4.5e-6f + 6e-10f * t * t));
        float denom = 1.0f - (1.0f - r) * ps / p;
        if (denom < 1e-12f) denom = 1e-12f;
        pw = r * ps / denom;
    }

    // Temperature in Kelvin
    float tk = t + 273.15f;

    // Refractive index minus 1 at the observer (gamma = (n - 1) at the observer)
    float gamma;
    // Optical/IR: wavelength-dependent formula
    float wlsq = w * w;
    float coef = 77.53484e-6f + (4.39108e-7f + 3.666e-9f / wlsq) / wlsq;
    float num = fmaf(coef, p, -11.2684e-6f * pw);   // fmaf(a,b,c) = a*b+c
    gamma = num / tk;

    // Beta coefficient (from Stone, with empirical adjustments)
    float beta = 4.4474e-6f * tk;

    // Refraction constants (from Green)
    if(refa) *refa = gamma * (1.0f - beta);
    if(refb) *refb = -gamma * (beta - gamma / 2.0f);
}
#endif

/**
 * @brief refraction - calculates refraction (z = z0 - refraction)
 * @param phpa - pressure, Hpa
 * @param tc - temperature, degC
 * @param rh - relative humidity, 0..1
 * @param zd - zenith distance, degrees
 * @return refraction, degrees
 */
float refraction(float phpa, float tc, float rh, float zd){
    float A, B;
    refco_f32(phpa, tc, rh, 0.55, &A, &B);
    float tanZ = tanf(DEG2RAD(zd));
    float refr = A * tanZ + B * tanZ * tanZ * tanZ;
    return RAD2DEG(refr);
}
