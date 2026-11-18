// PLEXIS Labs — FlexStrip DSP core
// Pure C++17, no JUCE dependency, so it can be unit-tested with synthetic signals.
//
// Signal flow (one channel), inspired by the Flex-10 block diagram:
//   [Pad -20 dB] -> [Polarity] -> [Gain] -> [HP 18 dB/oct] -> [LP 12 dB/oct]
//   -> [H-DRIVE transformer saturation] -> [Input LED meter] -> [Output trim] -> [VU meter]
//
// ASSUMPTIONS (the manual does not specify them):
//   - HP slope 18 dB/oct, LP slope 12 dB/oct (Butterworth)
//   - H-DRIVE is a model of transformer-style saturation, not a measured clone
//   - 0 VU = -18 dBFS RMS (adjustable with VU trim)
#pragma once

#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>

namespace plexis
{
constexpr double kPi = 3.14159265358979323846;

inline double dbToLin (double db) { return std::pow (10.0, db / 20.0); }
inline double linToDb (double lin) { return 20.0 * std::log10 (std::max (lin, 1.0e-12)); }

//==============================================================================
// Exponential parameter smoother
class Smoother
{
public:
    void prepare (double sampleRate, double timeMs)
    {
        coeff = std::exp (-1.0 / (0.001 * timeMs * sampleRate));
    }
    void setImmediate (double v) { current = target = v; }
    void setTarget (double v) { target = v; }
    double next() { current = target + coeff * (current - target); return current; }
    double getTarget() const { return target; }
    double getCurrent() const { return current; }
    bool isSettled() const { return std::abs (current - target) < 1.0e-7; }

private:
    double coeff = 0.999, current = 0.0, target = 0.0;
};

//==============================================================================
// Direct-form II transposed biquad (double precision)
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1 = 0, z2 = 0;

    void reset() { z1 = z2 = 0.0; }

    inline double process (double x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void setLowpass (double fs, double f, double q)
    {
        f = std::min (f, 0.49 * fs);
        const double w0 = 2.0 * kPi * f / fs, c = std::cos (w0), s = std::sin (w0);
        const double alpha = s / (2.0 * q), a0 = 1.0 + alpha;
        b0 = (1.0 - c) * 0.5 / a0; b1 = (1.0 - c) / a0; b2 = b0;
        a1 = -2.0 * c / a0; a2 = (1.0 - alpha) / a0;
    }

    void setHighpass (double fs, double f, double q)
    {
        f = std::min (f, 0.49 * fs);
        const double w0 = 2.0 * kPi * f / fs, c = std::cos (w0), s = std::sin (w0);
        const double alpha = s / (2.0 * q), a0 = 1.0 + alpha;
        b0 = (1.0 + c) * 0.5 / a0; b1 = -(1.0 + c) / a0; b2 = b0;
        a1 = -2.0 * c / a0; a2 = (1.0 - alpha) / a0;
    }

    // Low shelf (RBJ), gain in dB, S = 1
    void setLowShelf (double fs, double f, double gainDb)
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * kPi * f / fs, c = std::cos (w0), s = std::sin (w0);
        const double alpha = s / 2.0 * std::sqrt (2.0);
        const double sqA = std::sqrt (A);
        const double a0 = (A + 1) + (A - 1) * c + 2 * sqA * alpha;
        b0 = A * ((A + 1) - (A - 1) * c + 2 * sqA * alpha) / a0;
        b1 = 2 * A * ((A - 1) - (A + 1) * c) / a0;
        b2 = A * ((A + 1) - (A - 1) * c - 2 * sqA * alpha) / a0;
        a1 = -2 * ((A - 1) + (A + 1) * c) / a0;
        a2 = ((A + 1) + (A - 1) * c - 2 * sqA * alpha) / a0;
    }
};

// First-order high-pass (for the 3rd-order HP: one pole + one biquad)
struct OnePoleHP
{
    double b0 = 1, b1 = -1, a1 = 0, z1 = 0;
    void reset() { z1 = 0.0; }
    void set (double fs, double f)
    {
        f = std::min (f, 0.49 * fs);
        const double k = std::tan (kPi * f / fs);
        const double a0 = 1.0 + k;
        b0 = 1.0 / a0; b1 = -1.0 / a0; a1 = (k - 1.0) / a0;
    }
    inline double process (double x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y;
        return y;
    }
};

//==============================================================================
// Parameter set passed to the DSP (already in user units)
struct StripParams
{
    int    source      = 0;       // 0 = Mic, 1 = Line, 2 = Inst
    double gainKnob    = 0.0;     // 0..10
    bool   pad         = false;   // -20 dB
    bool   polarity    = false;   // true = inverted
    bool   filtersIn   = false;
    double hpHz        = 25.0;    // 25 .. 3150
    double lpHz        = 20000.0; // 160 .. 20000
    bool   hdriveIn    = false;
    double hdrive      = 0.0;     // 0..1
    double outputDb    = 0.0;     // output trim
    double vuTrimDb    = 0.0;     // VU calibration trim

    static double gainRangeDb (int source) { return source == 1 ? 27.0 : 63.0; }
    double gainDb() const { return gainKnob / 10.0 * gainRangeDb (source); }
};

//==============================================================================
// H-DRIVE: transformer-style saturation.
//   LF-weighted pre-emphasis (transformers saturate first at low frequencies)
//   -> asymmetric tanh waveshaper with first-order antiderivative anti-aliasing (ADAA)
//   -> level compensation.
class HDrive
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        reset();
    }
    void reset() { shelf.reset(); xPrev = 0.0; shelfDb = -1000.0; }

    // amount 0..1
    void setAmount (double a)
    {
        amount = std::clamp (a, 0.0, 1.0);
        k = 1.0 + 11.0 * std::pow (amount, 1.5);
        bias = 0.12 * amount; // slight asymmetry -> even harmonics
        // Compensation: unity gain at reference peak level
        const double refLvl = 0.3;
        comp = std::pow (refLvl / std::max (1.0e-9, waveshape (k * refLvl)), 1.15);
        const double newShelfDb = 5.0 * amount;
        if (std::abs (newShelfDb - shelfDb) > 0.02)
        {
            shelfDb = newShelfDb;
            shelf.setLowShelf (fs, 90.0, shelfDb);
        }
    }

    inline double process (double x)
    {
        const double u = k * shelf.process (x);
        double y;
        const double du = u - xPrev;
        if (std::abs (du) > 1.0e-5)
            y = (F (u) - F (xPrev)) / du;
        else
            y = waveshape (0.5 * (u + xPrev));
        xPrev = u;
        return y * comp;
    }

private:
    inline double waveshape (double u) const { return std::tanh (u + bias) - std::tanh (bias); }
    // antiderivative of waveshape
    inline double F (double u) const
    {
        const double v = u + bias;
        // log(cosh(v)) in a numerically stable form
        const double av = std::abs (v);
        const double lc = av + std::log1p (std::exp (-2.0 * av)) - std::log (2.0);
        return lc - u * std::tanh (bias);
    }

    double fs = 44100.0, amount = 0.0, k = 1.0, bias = 0.0, comp = 1.0;
    double xPrev = 0.0, shelfDb = -1000.0;
    Biquad shelf;
};

//==============================================================================
// Meters
struct LedMeterState
{
    // 7 LEDs: 0, -6, -12, -18, -24, -30, -36 dBFS
    static constexpr std::array<double, 7> thresholds { 0.0, -6.0, -12.0, -18.0, -24.0, -30.0, -36.0 };
};

class PeakMeter
{
public:
    void prepare (double sampleRate) { fs = sampleRate; peakDb = -120.0; }
    void process (const double* x, int n)
    {
        double blockPeak = 0.0;
        for (int i = 0; i < n; ++i) blockPeak = std::max (blockPeak, std::abs (x[i]));
        const double db = linToDb (blockPeak);
        const double decay = 20.0 * (double) n / fs; // 20 dB/s fall-back
        peakDb = std::max (db, peakDb - decay);
    }
    double getPeakDb() const { return peakDb; }
    // LED i (0 = top, 0 dBFS) is lit when peakDb >= thresholds[i]; returns the count of lit LEDs
    static int litSegments (double peakDb)
    {
        int n = 0;
        for (double t : LedMeterState::thresholds) if (peakDb >= t) ++n;
        return n;
    }

private:
    double fs = 44100.0, peakDb = -120.0;
};

// VU meter: mean rectified value (x1.1107 so a sine reads its RMS) through two cascaded
// one-pole low-passes (critically damped, 99 % in ~300 ms), 0 VU = refDbfs.
class VuMeter
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        const double omega = 6.64 / 0.300; // rad/s for 99% in 300 ms (critically damped 2nd order)
        const double tau = 1.0 / omega;
        a = std::exp (-1.0 / (tau * fs));
        s1 = s2 = 0.0;
    }
    void process (const double* x, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            const double r = std::abs (x[i]);
            s1 = r + a * (s1 - r);
            s2 = s1 + a * (s2 - s1);
        }
    }
    // reading in VU (dB relative to 0 VU), reference level in dBFS
    double getVuDb (double refDbfs) const { return linToDb (s2 * 1.1107) - refDbfs; }
    // reading as fraction of full scale (0..1), VU scale: -20 VU -> 0.0, +3 VU -> 1.0
    static double needlePosition (double vuDb)
    {
        const double pct = 100.0 * std::pow (10.0, vuDb / 20.0); // 100 % = 0 VU
        return std::clamp (pct / 141.25, 0.0, 1.05);
    }

private:
    double fs = 44100.0, a = 0.99, s1 = 0.0, s2 = 0.0;
};

//==============================================================================
// One channel strip
class ChannelStrip
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        gainSm.prepare (fs, 20.0);
        outSm.prepare (fs, 20.0);
        hpSm.prepare (fs, 30.0);
        lpSm.prepare (fs, 30.0);
        filtMix.prepare (fs, 8.0);
        driveMix.prepare (fs, 8.0);
        driveAmt.prepare (fs, 30.0);
        hdrive.prepare (fs);
        ledMeter.prepare (fs);
        vu.prepare (fs);
        reset();
        setParams (params, true);
    }

    void reset()
    {
        hp1.reset(); hp2.reset(); lp.reset(); hdrive.reset();
    }

    void setParams (const StripParams& p, bool immediate = false)
    {
        params = p;
        const double g = (p.polarity ? -1.0 : 1.0) * dbToLin (p.gainDb() - (p.pad ? 20.0 : 0.0));
        gainSm.setTarget (g);
        outSm.setTarget (dbToLin (p.outputDb));
        hpSm.setTarget (std::log (std::clamp (p.hpHz, 20.0, 3150.0)));
        lpSm.setTarget (std::log (std::clamp (p.lpHz, 160.0, 20000.0)));
        filtMix.setTarget (p.filtersIn ? 1.0 : 0.0);
        driveMix.setTarget (p.hdriveIn ? 1.0 : 0.0);
        driveAmt.setTarget (p.hdrive);
        if (immediate)
        {
            gainSm.setImmediate (g);
            outSm.setImmediate (dbToLin (p.outputDb));
            hpSm.setImmediate (hpSm.getTarget());
            lpSm.setImmediate (lpSm.getTarget());
            filtMix.setImmediate (p.filtersIn ? 1.0 : 0.0);
            driveMix.setImmediate (p.hdriveIn ? 1.0 : 0.0);
            driveAmt.setImmediate (p.hdrive);
            updateFilters();
            hdrive.setAmount (p.hdrive);
        }
    }

    // In-place processing. Uses a double scratch buffer provided by the caller (size >= n).
    void process (float* io, int n, double* scratch)
    {
        constexpr int kSub = 32; // coefficient update interval
        int pos = 0;
        while (pos < n)
        {
            const int m = std::min (kSub, n - pos);
            updateFilters();
            hdrive.setAmount (driveAmt.getCurrent());
            for (int i = 0; i < m; ++i)
            {
                double x = (double) io[pos + i] * gainSm.next();
                hpSm.next(); lpSm.next(); driveAmt.next();

                // filters (crossfaded in/out)
                const double fm = filtMix.next();
                if (fm > 1.0e-6 || filtMix.getTarget() > 0.5)
                {
                    const double f = lp.process (hp2.process (hp1.process (x)));
                    x = x + fm * (f - x);
                }
                else { hp1.reset(); hp2.reset(); lp.reset(); }

                // H-DRIVE (crossfaded)
                const double dm = driveMix.next();
                if (dm > 1.0e-6 || driveMix.getTarget() > 0.5)
                {
                    const double d = hdrive.process (x);
                    x = x + dm * (d - x);
                }
                scratch[pos + i] = x;
            }
            pos += m;
        }

        ledMeter.process (scratch, n);

        for (int i = 0; i < n; ++i)
        {
            const double y = scratch[i] * outSm.next();
            scratch[i] = y;
            io[i] = (float) y;
        }
        vu.process (scratch, n);
    }

    double getLedPeakDb() const { return ledMeter.getPeakDb(); }
    double getVuDb() const { return vu.getVuDb (-18.0 + params.vuTrimDb); }
    const StripParams& getParams() const { return params; }

private:
    void updateFilters()
    {
        const double hpf = std::exp (hpSm.getCurrent());
        const double lpf = std::exp (lpSm.getCurrent());
        hp1.set (fs, hpf);
        hp2.setHighpass (fs, hpf, 1.0); // 3rd-order Butterworth = 1 pole + biquad with Q = 1
        lp.setLowpass (fs, lpf, 1.0 / std::sqrt (2.0));
    }

    double fs = 44100.0;
    StripParams params;
    Smoother gainSm, outSm, hpSm, lpSm, filtMix, driveMix, driveAmt;
    OnePoleHP hp1;
    Biquad hp2, lp;
    HDrive hdrive;
    PeakMeter ledMeter;
    VuMeter vu;
};

} // namespace plexis
