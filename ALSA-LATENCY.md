# ALSA Latency in zita-alsa-pcmi

This document explains what determines audio round-trip latency when using the ALSA
MMAP API, and specifically how the choices made in zita-alsa-pcmi relate to that
latency.

## Components of round-trip latency

The round-trip latency (capture → process → playback) has two additive parts:

- **Capture latency**: the hardware fills one period into the ring buffer, then signals
  the application. The application reads data that is already up to one period old.
  Capture latency is always approximately one period, regardless of buffer configuration.

- **Playback latency**: the distance, in frames, between where the application writes
  and where the hardware is currently playing. A newly written frame must wait for the
  hardware to advance through all the frames ahead of it.

The total round-trip minimum is therefore approximately two periods: one for capture,
one for playback.

## API choices that affect latency

### MMAP vs. read/write access

zita-alsa-pcmi insists on MMAP access (`SND_PCM_ACCESS_MMAP_*`) and fails if the
driver does not support it. With MMAP, the application writes directly into the DMA
ring buffer. The alternative read/write API (`snd_pcm_readi`/`snd_pcm_writei`) copies
data through an intermediate staging buffer inside ALSA, adding a period-sized copy
delay.

### avail_min

`avail_min` is a software parameter set via `snd_pcm_sw_params_set_avail_min()`. It is
the threshold used by `poll()`: the kernel wakes up the application when the number of
available frames (space for playback, or data ready for capture) reaches this value.

Setting `avail_min` to one period (`_fsize`) means the application wakes up as soon as
there is room for exactly one more period. This keeps the playback buffer nearly full
in steady state.

### start_threshold

Setting `start_threshold` to zero (done here for playback) disables the ALSA
auto-start mechanism. Without this, ALSA auto-starts the device only after the
application has filled the entire ring buffer, guaranteeing at least one full buffer of
latency before any sound is produced. With auto-start disabled, the application
controls exactly when the device starts and with how much data pre-filled.

### Buffer size

After negotiating period size and period count, the code explicitly calls
`snd_pcm_hw_params_set_buffer_size()` with `period_size × nperiods`. Without this,
ALSA may silently round the buffer up to the next power of two or another
hardware-preferred size, inflating latency.

### snd_pcm_link()

When both capture and playback handles are open, the code calls `snd_pcm_link()`. This
makes the two streams start atomically and share the same clock. Without linking, the
two independent clocks drift apart, and a robust application must keep extra buffer to
absorb that drift.

### Opening hw: directly

The device names passed to `snd_pcm_open()` come directly from the caller. zita
applications use raw `hw:` names. Opening `plughw:` inserts the ALSA plug layer (rate
conversion, format conversion, channel mixing), each plugin adding its own buffer.
Opening `default` routes through PulseAudio or PipeWire, adding tens of milliseconds.

---

## How pre_fill and avail_min jointly determine latency

The steady-state playback fill level — the distance in frames between the application
write pointer and the hardware play pointer — is what determines playback latency. It
is controlled by two parameters set at startup:

- **`pre_fill_periods`**: how many periods of silence are written before `snd_pcm_start()`
- **`avail_min`**: the fill level at which `poll()` wakes the application

In steady state the application writes one period each time poll fires, and the
hardware consumes one period per interrupt. The fill level after each write is
constant, equal to `pre_fill_periods × fsize`. The fill level just before each write
(the minimum) is one period less:

```
fill_min  = (pre_fill_periods − 1) × fsize
fill_max  = pre_fill_periods × fsize     (= latency of newly written data)
```

For this to be self-sustaining, `avail_min` must equal the available space when the
fill drops to its minimum:

```
avail_min = buffer_size − fill_min
          = (nfrag − pre_fill_periods + 1) × fsize
```

This gives a clean relationship between latency and xrun tolerance:

| Quantity         | Value                            |
|------------------|----------------------------------|
| Playback latency | `pre_fill_periods` periods       |
| Xrun tolerance   | `pre_fill_periods − 1` periods   |

Which means:

> **latency = xrun_tolerance + 1 period, always.**

The buffer size sets the ceiling for both quantities but does not by itself determine
either one.

### Example: nfrag = 4

| pre_fill_periods | avail_min   | playback latency | xrun tolerance |
|------------------|-------------|------------------|----------------|
| 4 *(current)*    | 1 × fsize   | 4 periods        | 3 periods      |
| 3                | 2 × fsize   | 3 periods        | 2 periods      |
| 2                | 3 × fsize   | 2 periods        | 1 period       |

With `pre_fill=2` and `avail_min=3×fsize` the trace is:

1. After pre-fill: fill = 2 periods, avail = 2 periods < avail_min. Poll does not fire.
2. After one hardware interrupt: fill = 1 period, avail = 3 periods = avail_min. Poll fires.
3. App writes one period: fill = 2 periods. avail = 2 periods < avail_min. Poll does not fire.
4. Repeat from step 2.

Steady-state fill oscillates between 1 and 2 periods. Latency = 2 periods. A 4-period
hardware buffer thus provides 1 period of xrun tolerance (the hardware can advance one
full extra period past the nominal wakeup point before running out of data) without
increasing latency beyond the ping-pong minimum.

If `pre_fill` were reduced without correspondingly increasing `avail_min`, poll would
fire immediately on startup (avail already ≥ avail_min), the app would write a period,
and fill would climb back up. The two parameters must be set consistently.

---

## What zita-alsa-pcmi currently does

`set_swpar()` always sets `avail_min = _fsize` (one period), and `pcm_start()`
pre-fills all `_play_nfrag` periods. This always operates at the maximum-latency /
maximum-tolerance corner:

- latency = `nfrag` periods
- xrun tolerance = `nfrag − 1` periods

This is simple and predictable. If minimum latency is the goal, use `nfrag=2`. The
larger buffer option is not currently exploited to provide tolerance at lower latency.

To operate at lower latency with a larger buffer (as JACK does), two things would need
to become independently configurable:

1. The number of periods pre-filled in `pcm_start()`.
2. `avail_min` in `set_swpar()`, set to `(nfrag − pre_fill_periods + 1) × fsize`.
