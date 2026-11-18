// Unit tests for the FlexStrip DSP core, using synthetic signals. No JUCE needed.
#include "../Source/Dsp/FlexStripDsp.h"
#include <cstdio>
#include <vector>
#include <complex>

using namespace plexis;

static int failures = 0;
#define CHECK(cond, msg, ...) do { if (!(cond)) { ++failures; std::printf ("  FAIL: " msg "\n", ##__VA_ARGS__); } else std::printf ("  ok:   " msg "\n", ##__VA_ARGS__); } while (0)

static const double FS = 48000.0;

static std::vector<float> sine (double f, double amp, int n)
{
    std::vector<float> v (n);
    for (int i = 0; i < n; ++i) v[i] = (float) (amp * std::sin (2.0 * kPi * f * i / FS));
    return v;
}

// Amplitude of component at frequency f over the last `len` samples (Goertzel-like DFT bin)
static double amplitudeAt (const std::vector<float>& x, double f, int len)
{
    std::complex<double> acc = 0;
    const int start = (int) x.size() - len;
    for (int i = 0; i < len; ++i)
        acc += (double) x[start + i] * std::polar (1.0, -2.0 * kPi * f * i / FS);
    return 2.0 * std::abs (acc) / len;
}

static double rms (const std::vector<float>& x, int len)
{
    double s = 0; const int start = (int) x.size() - len;
    for (int i = start; i < (int) x.size(); ++i) s += (double) x[i] * x[i];
    return std::sqrt (s / len);
}

static std::vector<float> run (ChannelStrip& s, std::vector<float> x)
{
    std::vector<double> scratch (x.size());
    // process in odd-sized blocks to exercise block-boundary handling
    int pos = 0, blk = 1;
    while (pos < (int) x.size())
    {
        const int n = std::min (blk, (int) x.size() - pos);
        s.process (x.data() + pos, n, scratch.data());
        pos += n; blk = (blk * 7 + 13) % 997 + 1;
    }
    return x;
}

static ChannelStrip make (const StripParams& p)
{
    ChannelStrip s; s.setParams (p, true); s.prepare (FS); s.setParams (p, true); return s;
}

int main()
{
    const int N = (int) (FS * 1.0);
    const int L = 4800; // analysis length (100 ms, integer cycles for the test frequencies)

    std::printf ("[1] Unity when everything is neutral\n");
    {
        StripParams p; auto s = make (p);
        auto y = run (s, sine (1000, 0.5, N));
        CHECK (std::abs (amplitudeAt (y, 1000, L) - 0.5) < 1e-4, "amplitude 0.5 -> %.5f", amplitudeAt (y, 1000, L));
    }

    std::printf ("[2] Gain ranges\n");
    {
        StripParams p; p.gainKnob = 10; p.source = 0; auto s = make (p);
        auto y = run (s, sine (1000, 0.001, N));
        const double g = linToDb (amplitudeAt (y, 1000, L) / 0.001);
        CHECK (std::abs (g - 63.0) < 0.1, "Mic max gain = %.2f dB (expected 63)", g);
        p.source = 1; auto s2 = make (p);
        auto y2 = run (s2, sine (1000, 0.01, N));
        const double g2 = linToDb (amplitudeAt (y2, 1000, L) / 0.01);
        CHECK (std::abs (g2 - 27.0) < 0.1, "Line max gain = %.2f dB (expected 27)", g2);
    }

    std::printf ("[3] Pad and polarity\n");
    {
        StripParams p; p.pad = true; auto s = make (p);
        auto y = run (s, sine (1000, 0.5, N));
        CHECK (std::abs (linToDb (amplitudeAt (y, 1000, L) / 0.5) + 20.0) < 0.05, "pad = %.2f dB", linToDb (amplitudeAt (y, 1000, L) / 0.5));
        StripParams q; q.polarity = true; auto s2 = make (q);
        std::vector<float> x = sine (1000, 0.5, N);
        auto y2 = run (s2, x);
        double err = 0; for (int i = N - L; i < N; ++i) err = std::max (err, (double) std::abs (y2[i] + x[i]));
        CHECK (err < 1e-4, "polarity inverts (max error %.2e)", err);
    }

    std::printf ("[4] Filters\n");
    {
        StripParams p; p.filtersIn = true; p.hpHz = 1000; p.lpHz = 20000; auto s = make (p);
        auto at1k = linToDb (amplitudeAt (run (s, sine (1000, 0.5, N)), 1000, L) / 0.5);
        auto s2 = make (p);
        auto at250 = linToDb (amplitudeAt (run (s2, sine (250, 0.5, N)), 250, L) / 0.5);
        auto s3 = make (p);
        auto at4k = linToDb (amplitudeAt (run (s3, sine (4000, 0.5, N)), 4000, L) / 0.5);
        CHECK (std::abs (at1k + 3.0) < 0.3, "HP @ fc: %.2f dB (expected -3)", at1k);
        CHECK (std::abs (at250 + 36.0) < 2.0, "HP 2 octaves below fc: %.2f dB (18 dB/oct -> about -36)", at250);
        CHECK (std::abs (at4k) < 0.5, "HP passband: %.2f dB", at4k);

        StripParams q; q.filtersIn = true; q.hpHz = 25; q.lpHz = 2000; auto t = make (q);
        auto l2k = linToDb (amplitudeAt (run (t, sine (2000, 0.5, N)), 2000, L) / 0.5);
        auto t2 = make (q);
        auto l8k = linToDb (amplitudeAt (run (t2, sine (8000, 0.5, N)), 8000, L) / 0.5);
        CHECK (std::abs (l2k + 3.0) < 0.3, "LP @ fc: %.2f dB (expected -3)", l2k);
        CHECK (std::abs (l8k + 24.0) < 2.0, "LP 2 octaves above fc: %.2f dB (12 dB/oct -> about -24)", l8k);

        StripParams off = p; off.filtersIn = false; auto u = make (off);
        auto bypass = linToDb (amplitudeAt (run (u, sine (250, 0.5, N)), 250, L) / 0.5);
        CHECK (std::abs (bypass) < 0.01, "Filters out = transparent (%.3f dB)", bypass);
    }

    std::printf ("[5] H-DRIVE\n");
    {
        // level stays in a sane range across the knob, harmonics grow, no NaN
        double prevH3 = -1; bool monotone = true, finite = true, levelOk = true;
        for (int step = 0; step <= 10; ++step)
        {
            StripParams p; p.hdriveIn = true; p.hdrive = step / 10.0; auto s = make (p);
            auto y = run (s, sine (200, 0.2, N));
            for (float v : y) if (! std::isfinite (v)) finite = false;
            const double fund = amplitudeAt (y, 200, L);
            const double h3 = amplitudeAt (y, 600, L) / std::max (fund, 1e-9);
            const double lvl = linToDb (rms (y, L) / (0.2 / std::sqrt (2.0)));
            if (std::abs (lvl) > 6.0) levelOk = false;
            if (step > 0 && h3 + 1e-6 < prevH3) monotone = false;
            std::printf ("      drive %.1f: level %+5.2f dB, 3rd/fund %.1f dB\n", p.hdrive, lvl, linToDb (h3));
            prevH3 = h3;
        }
        CHECK (finite, "no NaN/Inf");
        CHECK (levelOk, "level stays within +/-6 dB across the knob (-14 dBFS input)");
        CHECK (monotone, "3rd harmonic grows with drive");

        StripParams p; p.hdriveIn = true; p.hdrive = 1.0; auto s = make (p);
        auto y = run (s, sine (200, 0.2, N));
        const double h2 = amplitudeAt (y, 400, L), fund = amplitudeAt (y, 200, L);
        CHECK (h2 / fund > 1e-3, "2nd harmonic present (asymmetry): %.1f dB", linToDb (h2 / fund));

        StripParams off; off.hdriveIn = false; off.hdrive = 1.0; auto so = make (off);
        auto yo = run (so, sine (200, 0.2, N));
        CHECK (std::abs (amplitudeAt (yo, 200, L) - 0.2) < 1e-4, "H-DRIVE out = transparent");
    }

    std::printf ("[6] Meters\n");
    {
        // LED: 0.5 -> about -6 dBFS peak
        StripParams p; auto s = make (p);
        run (s, sine (1000, 0.5, 4800));
        const double pk = s.getLedPeakDb();
        CHECK (std::abs (pk + 6.02) < 0.2, "LED peak %.2f dBFS (expected -6)", pk);
        CHECK (PeakMeter::litSegments (-6.02) == 5, "lit segments at -6.02 dBFS = %d (expected 5: -12 ... -36)", PeakMeter::litSegments (-6.02));
        CHECK (PeakMeter::litSegments (-0.5) == 6, "lit segments at -0.5 dBFS = %d (expected 6)", PeakMeter::litSegments (-0.5));
        CHECK (PeakMeter::litSegments (0.0) == 7, "lit segments at 0 dBFS = %d (expected 7)", PeakMeter::litSegments (0.0));
        CHECK (PeakMeter::litSegments (-100.0) == 0, "silence: no LED");

        // VU: sine at -18 dBFS RMS -> 0 VU after 1 s
        StripParams q; auto t = make (q);
        const double amp = dbToLin (-18.0) * std::sqrt (2.0);
        run (t, sine (1000, amp, N));
        CHECK (std::abs (t.getVuDb()) < 0.1, "VU reads %.3f (expected 0 at -18 dBFS RMS)", t.getVuDb());

        // VU ballistics: 99% (about -0.09 dB) in 300 ms, from silence
        StripParams r; auto u = make (r);
        const int n300 = (int) (0.3 * FS);
        run (u, sine (1000, amp, n300));
        const double pct = std::pow (10.0, u.getVuDb() / 20.0);
        CHECK (pct > 0.90 && pct < 1.0, "VU at 300 ms reaches %.1f %% (expected ~99; the rectified ripple at 1 kHz is small)", pct * 100.0);

        // VU trim shifts the reference
        StripParams v; v.vuTrimDb = 6.0; auto w = make (v);
        run (w, sine (1000, amp, N));
        CHECK (std::abs (w.getVuDb() + 6.0) < 0.1, "VU trim +6 dB -> %.2f VU", w.getVuDb());

        CHECK (std::abs (VuMeter::needlePosition (0.0) - 100.0 / 141.25) < 1e-6, "needle position of 0 VU = %.3f", VuMeter::needlePosition (0.0));
    }

    std::printf ("[7] Parameter changes are click-free\n");
    {
        StripParams p; auto s = make (p);
        std::vector<float> x = sine (1000, 0.3, N);
        std::vector<double> scratch (N);
        s.process (x.data(), N / 2, scratch.data());
        p.filtersIn = true; p.hdriveIn = true; p.hdrive = 0.8; p.gainKnob = 3; p.hpHz = 400; p.lpHz = 5000;
        s.setParams (p);
        s.process (x.data() + N / 2, N / 2, scratch.data());
        double maxJump = 0;
        for (int i = N / 2 - 4; i < N / 2 + 2000; ++i) maxJump = std::max (maxJump, (double) std::abs (x[i + 1] - x[i]));
        CHECK (maxJump < 0.6, "max sample-to-sample step after switching everything on: %.3f", maxJump);
    }

    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
