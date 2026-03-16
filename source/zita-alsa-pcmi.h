// ----------------------------------------------------------------------------
//
//  Copyright (C) 2006-2022 Fons Adriaensen <fons@linuxaudio.org>
//  Copyright (C) 2014-2021 Robin Gareus <robin@gareus.org>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------

#ifndef __ZITA_ALSA_PCMI_H
#define __ZITA_ALSA_PCMI_H

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API
#include <alsa/asoundlib.h>

/// @brief Library major version number.
#define ZITA_ALSA_PCMI_MAJOR_VERSION 0
/// @brief Library minor version number.
#define ZITA_ALSA_PCMI_MINOR_VERSION 6

/// @brief Return the major version number of the library.
extern int zita_alsa_pcmi_major_version(void);

/// @brief Return the minor version number of the library.
extern int zita_alsa_pcmi_minor_version(void);

/**
 * @brief Low-latency ALSA PCM interface for full-duplex audio I/O.
 *
 * Alsa_pcmi opens and configures one or two ALSA hw: PCM devices (playback
 * and/or capture) in memory-mapped mode and presents audio data to the
 * application as single-precision floating-point samples in the range
 * [-1.0, 1.0].  Sample format and channel count are negotiated automatically
 * with the driver.
 *
 * When both directions are open, the two streams are linked with
 * snd_pcm_link() so that they start atomically and share the same clock,
 * eliminating the need for drift compensation.
 *
 * **Typical usage:**
 * 1. Construct with the desired device names, sample rate, period size, and
 *    period count.  Check state() == STATE_OPEN before proceeding.
 * 2. Call pcm_start() to pre-fill the playback buffer and start the streams.
 * 3. Loop:
 *    - Call pcm_wait() to block until one period is available in each
 *      open direction.
 *    - Call capt_init(), capt_chan() for each channel, capt_done() to read
 *      capture data.
 *    - Process the audio.
 *    - Call play_init(), then either clear_chan() or play_chan() for each
 *      channel, then play_done() to write playback data.
 *    - If capt_init() or play_init() returned fewer frames than requested
 *      (ring buffer wrap), repeat the init/chan/done sequence for the
 *      remainder.
 * 4. Call pcm_stop() to stop the streams.
 */
class Alsa_pcmi {
public:
    /**
     * @brief Open and configure ALSA PCM device(s).
     *
     * Attempts to open the named devices and negotiate the requested
     * parameters with the driver.  Either @p play_name or @p capt_name
     * (but not both) may be null to open only one direction.
     *
     * On return, check state() == STATE_OPEN to determine whether
     * initialisation succeeded before calling any other method.
     *
     * The playback ring buffer size is @p frsize * @p play_nfrags frames;
     * capture is @p frsize * @p capt_nfrags frames.  In the default
     * operating mode (full pre-fill) the playback latency is
     * @p play_nfrags periods and the capture latency is one period.
     *
     * @param play_name   ALSA device name for playback (e.g. "hw:0,0"),
     *                    or null to disable playback.
     * @param capt_name   ALSA device name for capture (e.g. "hw:0,0"),
     *                    or null to disable capture.
     * @param ctrl_name   ALSA control device name (e.g. "hw:0") used to
     *                    query card information, or null to skip.
     * @param rate        Requested sample rate in Hz.
     * @param frsize      Requested period size in frames.  The hardware
     *                    generates one interrupt per period, which is the
     *                    wakeup granularity and the minimum achievable
     *                    latency.
     * @param play_nfrags Number of periods in the playback ring buffer
     *                    (>= 2).  A larger value increases tolerance for
     *                    scheduling jitter at the cost of higher latency
     *                    in the default operating mode.
     * @param capt_nfrags Number of periods in the capture ring buffer.
     * @param debug       Bitmask of DEBUG_* flags enabling diagnostic
     *                    messages on stderr, optionally combined with
     *                    FORCE_16B or FORCE_2CH.  Overridden at runtime
     *                    by the environment variable ZITA_ALSA_PCMI_DEBUG
     *                    if that variable is set to a non-empty value.
     */
    Alsa_pcmi(const char *play_name,
              const char *capt_name,
              const char *ctrl_name,
              unsigned int rate,
              unsigned int frsize,
              unsigned int play_nfrags,
              unsigned int capt_nfrags,
              unsigned int debug = 0);

    ~Alsa_pcmi(void);

    /// @brief Values returned by state().
    enum {
        STATE_OPEN = 0, ///< Initialisation succeeded; the device is ready to use.
        STATE_FAIL = 1  ///< Initialisation failed; no other methods may be called.
    };

    /**
     * @brief Flags for the @p debug constructor parameter.
     *
     * The DEBUG_* flags enable diagnostic messages on stderr.  They may
     * be combined as a bitmask.  DEBUG_ALL enables all four message
     * categories.  FORCE_16B and FORCE_2CH alter hardware negotiation
     * rather than enabling output.
     */
    enum {
        DEBUG_INIT = 1,   ///< Report errors and decisions during initialisation.
        DEBUG_STAT = 2,   ///< Report errors during start, stop, and xrun recovery.
        DEBUG_WAIT = 4,   ///< Report events inside pcm_wait().
        DEBUG_DATA = 8,   ///< Report errors during MMAP I/O.
        DEBUG_ALL  = 15,  ///< Enable all diagnostic message categories.
        FORCE_16B  = 256, ///< Restrict sample format negotiation to 16-bit formats.
        FORCE_2CH  = 512  ///< Restrict channel count negotiation to two channels.
    };

    /// @brief Print a summary of the negotiated parameters to stdout.
    void printinfo(void);

    /**
     * @brief Pre-fill the playback buffer with silence and start the streams.
     *
     * Writes @c play_nfrag() periods of silence into the playback ring
     * buffer, then calls snd_pcm_start().  When both directions are open
     * and were successfully linked, only the playback handle is started
     * explicitly; the capture stream starts atomically via the link.
     *
     * @return 0 on success, -1 on failure (state() becomes STATE_FAIL).
     */
    int pcm_start(void);

    /**
     * @brief Stop both streams immediately, discarding buffered data.
     *
     * @return 0 on success, -1 on failure (state() becomes STATE_FAIL).
     */
    int pcm_stop(void);

    /**
     * @brief Block until at least one period is available in every open direction.
     *
     * Polls the file descriptors for all open streams, looping until both
     * the playback direction (POLLOUT, space available) and the capture
     * direction (POLLIN, data available) have signalled ready.  Only the
     * directions that are actually open are waited for.
     *
     * The return value is the minimum of the available playback space and
     * the available capture data, both expressed in frames.  It is
     * guaranteed to be at least fsize() on success.  This value should be
     * passed as the @p len argument to capt_init() and play_init(); those
     * functions may return fewer frames if the available region wraps
     * around the end of the ring buffer, in which case the
     * init/chan/done sequence must be repeated for the remainder.
     *
     * @return Number of available frames (>= fsize()), or 0 on timeout,
     *         EINTR, or error.
     */
    snd_pcm_sframes_t pcm_wait(void);

    /**
     * @brief Consume @p len frames without processing them.
     *
     * Advances the stream pointers by @p len frames, writing silence to
     * the playback buffer and discarding capture data.  Loops internally
     * to handle ring buffer wrap.  Useful for priming or draining the
     * streams without running DSP processing.
     *
     * @param len Number of frames to consume.
     * @return 0.
     */
    int pcm_idle(int len);

    /**
     * @brief Begin a playback MMAP region.
     *
     * Calls snd_pcm_mmap_begin() for the playback stream and sets up
     * internal per-channel write pointers.  Must be followed by
     * clear_chan() or play_chan() for each channel, then play_done().
     *
     * The returned frame count may be less than @p len if the requested
     * region wraps around the end of the ring buffer.  In that case the
     * caller must call play_init() again for the remaining frames after
     * play_done().
     *
     * @param len Requested number of frames.
     * @return Actual number of frames in the mapped region (<= @p len),
     *         or -1 on error.
     */
    int play_init(snd_pcm_uframes_t len);

    /**
     * @brief Write silence to a playback channel in the current MMAP region.
     *
     * @param chan Channel index (0-based, must be < nplay()).
     * @param len  Number of frames to clear (as returned by play_init()).
     */
    void clear_chan(int chan, int len);

    /**
     * @brief Convert and write float samples to a playback channel.
     *
     * Converts @p len samples from @p src from single-precision float to
     * the negotiated hardware format and writes them into the current MMAP
     * playback region.
     *
     * @param chan Channel index (0-based, must be < nplay()).
     * @param src  Source buffer of float samples in [-1.0, 1.0].
     * @param len  Number of frames to write (as returned by play_init()).
     * @param step Stride between consecutive samples in @p src.  Use 1
     *             for a contiguous single-channel buffer, or the total
     *             channel count for an interleaved multi-channel buffer.
     */
    void play_chan(int chan, const float *src, int len, int step = 1);

    /**
     * @brief Commit the current playback MMAP region to the hardware.
     *
     * Calls snd_pcm_mmap_commit() to hand the written frames to the
     * hardware.  Must be called after play_init() and the channel writes.
     *
     * @param len Number of frames to commit (as returned by play_init()).
     * @return Return value of snd_pcm_mmap_commit().
     */
    int play_done(int len);

    /**
     * @brief Begin a capture MMAP region.
     *
     * Calls snd_pcm_mmap_begin() for the capture stream and sets up
     * internal per-channel read pointers.  Must be followed by
     * capt_chan() for each channel, then capt_done().
     *
     * The returned frame count may be less than @p len if the available
     * region wraps around the end of the ring buffer.  In that case the
     * caller must call capt_init() again for the remaining frames after
     * capt_done().
     *
     * @param len Requested number of frames.
     * @return Actual number of frames in the mapped region (<= @p len),
     *         or -1 on error.
     */
    int capt_init(snd_pcm_uframes_t len);

    /**
     * @brief Convert and read float samples from a capture channel.
     *
     * Reads @p len frames from the current MMAP capture region, converts
     * them from the negotiated hardware format to single-precision float,
     * and writes the result to @p dst.
     *
     * @param chan Channel index (0-based, must be < ncapt()).
     * @param dst  Destination buffer for float samples in [-1.0, 1.0].
     * @param len  Number of frames to read (as returned by capt_init()).
     * @param step Stride between consecutive samples in @p dst.  Use 1
     *             for a contiguous single-channel buffer, or the total
     *             channel count for an interleaved multi-channel buffer.
     */
    void capt_chan(int chan, float *dst, int len, int step = 1);

    /**
     * @brief Commit the current capture MMAP region, marking frames as consumed.
     *
     * Calls snd_pcm_mmap_commit() to return the consumed frames to the
     * hardware.  Must be called after capt_init() and the channel reads.
     *
     * @param len Number of frames to commit (as returned by capt_init()).
     * @return Return value of snd_pcm_mmap_commit().
     */
    int capt_done(int len);

    /**
     * @brief Return the number of frames immediately writable to playback.
     *
     * Calls snd_pcm_avail() on the playback handle.  Unlike pcm_wait(),
     * this does not block.
     */
    int play_avail(void) {
        return snd_pcm_avail(_play_handle);
    }

    /**
     * @brief Return the number of capture frames immediately available to read.
     *
     * Calls snd_pcm_avail() on the capture handle.  Unlike pcm_wait(),
     * this does not block.
     */
    int capt_avail(void) {
        return snd_pcm_avail(_capt_handle);
    }

    /**
     * @brief Return the playback delay in frames.
     *
     * The delay is the number of frames currently queued ahead of the
     * hardware play pointer, i.e. the time in frames before a frame
     * written now will be heard.
     */
    int play_delay(void) {
        long k;
        snd_pcm_delay(_play_handle, &k);
        return k;
    }

    /**
     * @brief Return the capture delay in frames.
     *
     * The delay is the number of frames between the hardware capture
     * pointer and the current application read position.
     */
    int capt_delay(void) {
        long k;
        snd_pcm_delay(_capt_handle, &k);
        return k;
    }

    /// @brief Return the duration of the most recent playback xrun in seconds, or 0 if none.
    float play_xrun(void) const {
        return _play_xrun;
    }

    /// @brief Return the duration of the most recent capture xrun in seconds, or 0 if none.
    float capt_xrun(void) const {
        return _capt_xrun;
    }

    /// @brief Return the current state: STATE_OPEN or STATE_FAIL.
    int state(void) const {
        return _state;
    }

    /// @brief Return the negotiated sample rate in Hz.
    int fsamp(void) const {
        return _fsamp;
    }

    /// @brief Return the negotiated period size in frames.
    int fsize(void) const {
        return _fsize;
    }

    /// @brief Return the requested number of playback periods.
    int play_nfrag(void) const {
        return _play_nfrag;
    }

    /// @brief Return the requested number of capture periods.
    int capt_nfrag(void) const {
        return _capt_nfrag;
    }

    /// @brief Return the number of playback channels.
    int nplay(void) const {
        return _play_nchan;
    }

    /// @brief Return the number of capture channels.
    int ncapt(void) const {
        return _capt_nchan;
    }

    /// @brief Return the raw ALSA playback handle, or null if playback is not open.
    snd_pcm_t *play_handle(void) const {
        return _play_handle;
    }

    /// @brief Return the raw ALSA capture handle, or null if capture is not open.
    snd_pcm_t *capt_handle(void) const {
        return _capt_handle;
    }

private:
    typedef char *(Alsa_pcmi::*clear_function)(char *, int);
    typedef char *(Alsa_pcmi::*play_function)(const float *, char *, int, int);
    typedef const char *(Alsa_pcmi::*capt_function)(const char *, float *, int, int);

    enum { MAXPFD  = 16,
           MAXCHAN = 256 };

    void initialise(const char *play_name, const char *capt_name, const char *ctrl_name);
    int set_hwpar(snd_pcm_t *handle, snd_pcm_hw_params_t *hwpar, const char *sname, unsigned int nfrag, unsigned int *nchan);
    int set_swpar(snd_pcm_t *handle, snd_pcm_sw_params_t *swpar, const char *sname);
    int recover(void);
    float xruncheck(snd_pcm_status_t *stat);

    char *clear_32(char *dst, int nfrm);
    char *clear_24(char *dst, int nfrm);
    char *clear_16(char *dst, int nfrm);

    char *play_floatne(const float *src, char *dst, int nfrm, int step);
    char *play_floatre(const float *src, char *dst, int nfrm, int step);
    char *play_32le(const float *src, char *dst, int nfrm, int step);
    char *play_24le(const float *src, char *dst, int nfrm, int step);
    char *play_16le(const float *src, char *dst, int nfrm, int step);
    char *play_32be(const float *src, char *dst, int nfrm, int step);
    char *play_24be(const float *src, char *dst, int nfrm, int step);
    char *play_16be(const float *src, char *dst, int nfrm, int step);

    const char *capt_floatne(const char *src, float *dst, int nfrm, int step);
    const char *capt_floatre(const char *src, float *dst, int nfrm, int step);
    const char *capt_32le(const char *src, float *dst, int nfrm, int step);
    const char *capt_24le(const char *src, float *dst, int nfrm, int step);
    const char *capt_16le(const char *src, float *dst, int nfrm, int step);
    const char *capt_32be(const char *src, float *dst, int nfrm, int step);
    const char *capt_24be(const char *src, float *dst, int nfrm, int step);
    const char *capt_16be(const char *src, float *dst, int nfrm, int step);

    unsigned int _fsamp;              ///< Negotiated sample rate in Hz.
    snd_pcm_uframes_t _fsize;         ///< Negotiated period size in frames.
    unsigned int _real_nfrag;         ///< Actual number of playback periods as reported by the driver (may differ from _play_nfrag).
    unsigned int _play_nfrag;         ///< Requested number of playback periods.
    unsigned int _capt_nfrag;         ///< Requested number of capture periods.
    unsigned int _debug;              ///< Debug/option flags bitmask (DEBUG_* and FORCE_* values).
    int _state;                       ///< Current state: STATE_OPEN or STATE_FAIL.
    snd_pcm_t *_play_handle;          ///< ALSA playback PCM handle, or null if playback is not open.
    snd_pcm_t *_capt_handle;          ///< ALSA capture PCM handle, or null if capture is not open.
    snd_ctl_t *_ctrl_handle;          ///< ALSA control handle, or null if no ctrl_name was given.
    snd_pcm_hw_params_t *_play_hwpar; ///< Playback hardware parameter container.
    snd_pcm_sw_params_t *_play_swpar; ///< Playback software parameter container.
    snd_pcm_hw_params_t *_capt_hwpar; ///< Capture hardware parameter container.
    snd_pcm_sw_params_t *_capt_swpar; ///< Capture software parameter container.
    snd_pcm_format_t _play_format;    ///< Negotiated playback sample format.
    snd_pcm_format_t _capt_format;    ///< Negotiated capture sample format.
    snd_pcm_access_t _play_access;    ///< Negotiated playback MMAP access mode.
    snd_pcm_access_t _capt_access;    ///< Negotiated capture MMAP access mode.
    unsigned int _play_nchan;         ///< Number of playback channels.
    unsigned int _capt_nchan;         ///< Number of capture channels.
    float _play_xrun;                 ///< Duration of the most recent playback xrun in seconds, or 0.
    float _capt_xrun;                 ///< Duration of the most recent capture xrun in seconds, or 0.
    bool _synced;                     ///< True if capture and playback were successfully linked via snd_pcm_link().
    int _play_npfd;                   ///< Number of poll file descriptors for the playback handle.
    int _capt_npfd;                   ///< Number of poll file descriptors for the capture handle.
    struct pollfd _poll_fd[MAXPFD];   ///< Poll descriptor array, playback descriptors first then capture.
    snd_pcm_uframes_t _capt_offs;     ///< Frame offset of the current capture MMAP region within the ring buffer.
    snd_pcm_uframes_t _play_offs;     ///< Frame offset of the current playback MMAP region within the ring buffer.
    int _play_step;                   ///< Byte stride between consecutive frames in the playback MMAP area.
    int _capt_step;                   ///< Byte stride between consecutive frames in the capture MMAP area.
    char *_play_ptr[MAXCHAN];         ///< Per-channel write pointers into the current playback MMAP region.
    const char *_capt_ptr[MAXCHAN];   ///< Per-channel read pointers into the current capture MMAP region.
    clear_function _clear_func;       ///< Format-specific function for writing silence to the playback buffer.
    play_function _play_func;         ///< Format-specific function for converting float samples to the playback buffer.
    capt_function _capt_func;         ///< Format-specific function for converting the capture buffer to float samples.
    char _dummy[128];                 ///< Padding reserved for future use.
};

#endif
