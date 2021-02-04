//-----------------------------------------------------------------------------
// Copyright (C) 2010 iZsh <izsh at fail0verflow.com>
// Modified by
//    Marshellow
//    Iceman
//    Doegox
//
// This code is licensed to you under the terms of the GNU GPL, version 2 or,
// at your option, any later version. See the LICENSE.txt file for the text of
// the license.
//-----------------------------------------------------------------------------
// Low frequency commands
//-----------------------------------------------------------------------------
#include "cmdlf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include "cmdparser.h"      // command_t
#include "comms.h"
#include "commonutil.h"     // ARRAYLEN
#include "lfdemod.h"        // device/client demods of LF signals
#include "ui.h"             // for show graph controls
#include "proxgui.h"
#include "cliparser.h"      // args parsing
#include "graph.h"          // for graph data
#include "cmddata.h"        // for `lf search`
#include "cmdlfawid.h"      // for awid menu
#include "cmdlfem.h"        // for em menu
#include "cmdlfem410x.h"      // for em4x menu
#include "cmdlfem4x05.h"    // for em4x05 / 4x69
#include "cmdlfem4x50.h"    // for em4x50
#include "cmdlfem4x70.h"    // for em4x70
#include "cmdlfhid.h"       // for hid menu
#include "cmdlfhitag.h"     // for hitag menu
#include "cmdlfidteck.h"    // for idteck menu
#include "cmdlfio.h"        // for ioprox menu
#include "cmdlfcotag.h"     // for COTAG meny
#include "cmdlfdestron.h"   // for FDX-A FECAVA Destron menu
#include "cmdlffdxb.h"      // for FDX-B menu
#include "cmdlfgallagher.h" // for GALLAGHER menu
#include "cmdlfguard.h"     // for gproxii menu
#include "cmdlfindala.h"    // for indala menu
#include "cmdlfjablotron.h" // for JABLOTRON menu
#include "cmdlfkeri.h"      // for keri menu
#include "cmdlfmotorola.h"  // for Motorola menu
#include "cmdlfnedap.h"     // for NEDAP menu
#include "cmdlfnexwatch.h"  // for nexwatch menu
#include "cmdlfnoralsy.h"   // for NORALSY meny
#include "cmdlfpac.h"       // for pac menu
#include "cmdlfparadox.h"   // for paradox menu
#include "cmdlfpcf7931.h"   // for pcf7931 menu
#include "cmdlfpresco.h"    // for presco menu
#include "cmdlfpyramid.h"   // for pyramid menu
#include "cmdlfsecurakey.h" // for securakey menu
#include "cmdlft55xx.h"     // for t55xx menu
#include "cmdlfti.h"        // for ti menu
#include "cmdlfviking.h"    // for viking menu
#include "cmdlfvisa2000.h"  // for VISA2000 menu

#define LF_CMDREAD_MAX_EXTRA_SYMBOLS 4
static bool g_lf_threshold_set = false;

static int CmdHelp(const char *Cmd);

static int usage_lf_cmdread(void) {
    PrintAndLogEx(NORMAL, "Usage: lf cmdread d <delay duration> z <zero duration> o <one duration> [e <symbol> <duration>] c <cmd symbols> [q] [s #samples] [@]");
    PrintAndLogEx(NORMAL, "Options:");
    PrintAndLogEx(NORMAL, "       h             This help");
    PrintAndLogEx(NORMAL, "       d <duration>  delay OFF period, (0 for bitbang mode)");
    PrintAndLogEx(NORMAL, "       z <duration>  ZERO time period");
    PrintAndLogEx(NORMAL, "       o <duration>  ONE time period");
    PrintAndLogEx(NORMAL, "       e <symbol> <duration> Extra symbol definition and duration (up to %i)", LF_CMDREAD_MAX_EXTRA_SYMBOLS);
    PrintAndLogEx(NORMAL, "       b <duration>  B period");
    PrintAndLogEx(NORMAL, "       c <cmd>       Command symbols (0/1/...)");
    PrintAndLogEx(NORMAL, "       q             silent (optional)");
    PrintAndLogEx(NORMAL, "       s #samples    number of samples to collect (optional)");
    PrintAndLogEx(NORMAL, "       @             run continuously until a key is pressed (optional)");
    PrintAndLogEx(NORMAL, "");
    PrintAndLogEx(NORMAL, "       ************* " _YELLOW_("All periods in decimal and in microseconds (us)"));
    PrintAndLogEx(NORMAL, "Examples:");
    PrintAndLogEx(NORMAL, _CYAN_(" probing for Hitag 1/Hitag S") ":");
    PrintAndLogEx(NORMAL, _YELLOW_("      lf cmdread d 50 z 116 o 166 e W 3000 c W00110"));
    PrintAndLogEx(NORMAL, _CYAN_(" probing for Hitag 2") ":");
    PrintAndLogEx(NORMAL, _YELLOW_("      lf cmdread d 50 z 116 o 166 e W 3000 c W11000"));
    PrintAndLogEx(NORMAL, _CYAN_(" probing for Hitag 2, oscilloscope style") ":");
    PrintAndLogEx(NORMAL, _YELLOW_("      data plot"));
    PrintAndLogEx(NORMAL, _YELLOW_("      lf cmdread d 50 z 116 o 166 e W 3000 c W11000 q s 2000 @"));
    PrintAndLogEx(NORMAL, _CYAN_(" probing for Hitag (us)") ":");
    PrintAndLogEx(NORMAL, _YELLOW_("      lf cmdread d 48 z 112 o 176 e W 3000 e S 240 e E 336 c W0S00000010000E"));
    PrintAndLogEx(NORMAL, "Extras:");
    PrintAndLogEx(NORMAL, "  use " _YELLOW_("'lf config'")" to set parameters.");
    return PM3_SUCCESS;
}
static int usage_lf_read(void) {
    PrintAndLogEx(NORMAL, "Usage: lf read [h] [q] [s #samples] [@]");
    PrintAndLogEx(NORMAL, "Options:");
    PrintAndLogEx(NORMAL, "       h            This help");
    PrintAndLogEx(NORMAL, "       q            silent (optional)");
    PrintAndLogEx(NORMAL, "       s #samples   number of samples to collect (optional)");
    PrintAndLogEx(NORMAL, "       @            run continuously until a key is pressed (optional)");
    PrintAndLogEx(NORMAL, "");
    PrintAndLogEx(NORMAL, "Examples:");
    PrintAndLogEx(NORMAL, "      lf read");
    PrintAndLogEx(NORMAL, "- collecting quietly 12000 samples:");
    PrintAndLogEx(NORMAL, "      lf read q s 12000     - ");
    PrintAndLogEx(NORMAL, "- oscilloscope style:");
    PrintAndLogEx(NORMAL, "      data plot");
    PrintAndLogEx(NORMAL, "      lf read q s 3000 @");
    PrintAndLogEx(NORMAL, "Extras:");
    PrintAndLogEx(NORMAL, "  use " _YELLOW_("'lf config'")" to set parameters.");
    return PM3_SUCCESS;
}
static int usage_lf_sim(void) {
    PrintAndLogEx(NORMAL, "Simulate low frequency tag from graphbuffer.");
    PrintAndLogEx(NORMAL, "Usage: lf sim [h] <gap>");
    PrintAndLogEx(NORMAL, "Options:");
    PrintAndLogEx(NORMAL, "       h         This help");
    PrintAndLogEx(NORMAL, "       <gap>     Start gap (in microseconds)");
    PrintAndLogEx(NORMAL, "Examples:");
    PrintAndLogEx(NORMAL, _YELLOW_("         lf sim 240") "     - start simulating with 240ms gap");
    PrintAndLogEx(NORMAL, _YELLOW_("         lf sim"));
    PrintAndLogEx(NORMAL, "Extras:");
    PrintAndLogEx(NORMAL, "  use " _YELLOW_("'lf config'")" to set parameters.");
    return PM3_SUCCESS;
}
static int usage_lf_sniff(void) {
    PrintAndLogEx(NORMAL, "Sniff low frequency signal.");
    PrintAndLogEx(NORMAL, "Usage: lf sniff [h] [q] [s #samples] [@]");
    PrintAndLogEx(NORMAL, "Options:");
    PrintAndLogEx(NORMAL, "       h         This help");
    PrintAndLogEx(NORMAL, "       q            silent (optional)");
    PrintAndLogEx(NORMAL, "       s #samples   number of samples to collect (optional)");
    PrintAndLogEx(NORMAL, "       @            run continuously until a key is pressed (optional)");
    PrintAndLogEx(NORMAL, "Examples:");
    PrintAndLogEx(NORMAL, "      lf sniff");
    PrintAndLogEx(NORMAL, _CYAN_(" oscilloscope style") ":");
    PrintAndLogEx(NORMAL, _YELLOW_("      data plot"));
    PrintAndLogEx(NORMAL, _YELLOW_("      lf sniff q s 3000 @"));
    PrintAndLogEx(NORMAL, "Extras:");
    PrintAndLogEx(NORMAL, "  use " _YELLOW_("'lf config'")" to set parameters.");
    PrintAndLogEx(NORMAL, "  use " _YELLOW_("'data plot'")" to look at it");
    return PM3_SUCCESS;
}
static int usage_lf_config(void) {
    PrintAndLogEx(NORMAL, "Usage: lf config [h] [L | H | q <divisor> | f <freq>] [b <bps>] [d <decim>] [a 0|1]");
    PrintAndLogEx(NORMAL, "Options:");
    PrintAndLogEx(NORMAL, "       h                 This help");
    PrintAndLogEx(NORMAL, "       L                 Low frequency (125 kHz)");
    PrintAndLogEx(NORMAL, "       H                 High frequency (134 kHz)");
    PrintAndLogEx(NORMAL, "       q <divisor>       Manually set freq divisor. %d -> 134 kHz, %d -> 125 kHz", LF_DIVISOR_134, LF_DIVISOR_125);
    PrintAndLogEx(NORMAL, "       f <freq>          Manually set frequency in kHz");
    PrintAndLogEx(NORMAL, "       b <bps>           Sets resolution of bits per sample. Default (max): 8");
    PrintAndLogEx(NORMAL, "       d <decimate>      Sets decimation. A value of N saves only 1 in N samples. Default: 1");
    PrintAndLogEx(NORMAL, "       a [0|1]           Averaging - if set, will average the stored sample value when decimating. Default: 1");
    PrintAndLogEx(NORMAL, "       t <threshold>     Sets trigger threshold. 0 means no threshold (range: 0-128)");
    PrintAndLogEx(NORMAL, "       s <samplestoskip> Sets a number of samples to skip before capture. Default: 0");
    PrintAndLogEx(NORMAL, "Examples:");
    PrintAndLogEx(NORMAL, _YELLOW_("      lf config") "              - shows current config");
    PrintAndLogEx(NORMAL, _YELLOW_("      lf config b 8 L") "        - samples at 125 kHz, 8bps.");
    PrintAndLogEx(NORMAL, _YELLOW_("      lf config H b 4 d 3") "    - samples at 134 kHz, averages three samples into one, stored with ");
    PrintAndLogEx(NORMAL, "                                a resolution of 4 bits per sample.");
    PrintAndLogEx(NORMAL, _YELLOW_("      lf read") "                - performs a read (active field)");
    PrintAndLogEx(NORMAL, _YELLOW_("      lf sniff") "               - performs a sniff (no active field)");
    return PM3_SUCCESS;
}
static int usage_lf_simfsk(void) {
    PrintAndLogEx(NORMAL, "Usage: lf simfsk [h] [c <clock>] [H <fcHigh>] [L <fcLow>] [d <hexdata>]");
    PrintAndLogEx(NORMAL, "there are about four FSK modulations to know of.");
    PrintAndLogEx(NORMAL, "FSK1  -  where fc/8 = high  and fc/5 = low");
    PrintAndLogEx(NORMAL, "FSK1a -  is inverted FSK1,  ie:   fc/5 = high and fc/8 = low");
    PrintAndLogEx(NORMAL, "FSK2  -  where fc/10 = high  and fc/8 = low");
    PrintAndLogEx(NORMAL, "FSK2a -  is inverted FSK2,  ie:   fc/10 = high and fc/8 = low");
    PrintAndLogEx(NORMAL, "");
    PrintAndLogEx(NORMAL, "Options:");
    PrintAndLogEx(NORMAL, "       h              This help");
    PrintAndLogEx(NORMAL, "       c <clock>      Manually set clock - can autodetect if using DemodBuffer");
    PrintAndLogEx(NORMAL, "       H <fcHigh>     Manually set the larger Field Clock");
    PrintAndLogEx(NORMAL, "       L <fcLow>      Manually set the smaller Field Clock");
    //PrintAndLogEx(NORMAL, "       s              TBD- -STT to enable a gap between playback repetitions - default: no gap");
    PrintAndLogEx(NORMAL, "       d <hexdata>    Data to sim as hex - omit to sim from DemodBuffer");
    PrintAndLogEx(NORMAL, "\n  NOTE: if you set one clock manually set them all manually");
    PrintAndLogEx(NORMAL, "");
    PrintAndLogEx(NORMAL, "Examples:");
    PrintAndLogEx(NORMAL,  _YELLOW_("       lf simfsk c 40 H 8 L 5 d 010203") "      -  FSK1  rf/40  data 010203");
    PrintAndLogEx(NORMAL,  _YELLOW_("       lf simfsk c 40 H 5 L 8 d 010203") "      -  FSK1a rf/40  data 010203");
    PrintAndLogEx(NORMAL,  _YELLOW_("       lf simfsk c 64 H 10 L 8 d 010203") "     -  FSK2  rf/64  data 010203");
    PrintAndLogEx(NORMAL,  _YELLOW_("       lf simfsk c 64 H 8 L 10 d 010203") "     -  FSK2a rf/64  data 010203");
    PrintAndLogEx(NORMAL, "");
    return PM3_SUCCESS;
}
static int usage_lf_simask(void) {
    PrintAndLogEx(NORMAL, "Usage: lf simask [c <clock>] [i] [b|m|r] [s] [d <raw hex to sim>]");
    PrintAndLogEx(NORMAL, "Options:");
    PrintAndLogEx(NORMAL, "       h              This help");
    PrintAndLogEx(NORMAL, "       c <clock>      Manually set clock - can autodetect if using DemodBuffer");
    PrintAndLogEx(NORMAL, "       i              invert data");
    PrintAndLogEx(NORMAL, "       b              sim ask/biphase");
    PrintAndLogEx(NORMAL, "       m              sim ask/manchester - Default");
    PrintAndLogEx(NORMAL, "       r              sim ask/raw");
    PrintAndLogEx(NORMAL, "       s              add t55xx Sequence Terminator gap - default: no gaps (only manchester)");
    PrintAndLogEx(NORMAL, "       d <hexdata>    Data to sim as hex - omit to sim from DemodBuffer");
    return PM3_SUCCESS;
}
static int usage_lf_simpsk(void) {
    PrintAndLogEx(NORMAL, "Usage: lf simpsk [1|2|3] [c <clock>] [i] [r <carrier>] [d <raw hex to sim>]");
    PrintAndLogEx(NORMAL, "Options:");
    PrintAndLogEx(NORMAL, "       h              This help");
    PrintAndLogEx(NORMAL, "       c <clock>      Manually set clock - can autodetect if using DemodBuffer");
    PrintAndLogEx(NORMAL, "       i              invert data");
    PrintAndLogEx(NORMAL, "       1              set PSK1 (default)");
    PrintAndLogEx(NORMAL, "       2              set PSK2");
    PrintAndLogEx(NORMAL, "       3              set PSK3");
    PrintAndLogEx(NORMAL, "       r <carrier>    2|4|8 are valid carriers: default = 2");
    PrintAndLogEx(NORMAL, "       d <hexdata>    Data to sim as hex - omit to sim from DemodBuffer");
    return PM3_SUCCESS;
}

static int CmdLFTune(const char *Cmd) {

    CLIParserContext *ctx;
    CLIParserInit(&ctx, "lf tune",
                  "Continuously measure LF antenna tuning.\n"
                  "Press button or <Enter> to interrupt.",
                  "lf tune\n"
                  "lf tune --mix"
                 );

    char q_str[60];
    snprintf(q_str, sizeof(q_str), "Frequency divisor. %d -> 134 kHz, %d -> 125 kHz", LF_DIVISOR_134, LF_DIVISOR_125);
    void *argtable[] = {
        arg_param_begin,
        arg_u64_0("n", "iter", "<dec>", "number of iterations (default: 0=infinite)"),
        arg_u64_0("q", "divisor", "<dec>", q_str),
        arg_dbl0("f", "freq", "<float>", "Frequency in kHz"),
        arg_lit0(NULL, "bar", "bar style"),
        arg_lit0(NULL, "mix", "mixed style"),
        arg_lit0(NULL, "value", "values style"),
        arg_param_end
    };
    CLIExecWithReturn(ctx, Cmd, argtable, true);
    uint32_t iter = arg_get_u32_def(ctx, 1, 0);
    uint8_t divisor = arg_get_u32_def(ctx, 2, LF_DIVISOR_125);
    double freq = arg_get_dbl_def(ctx, 3, 125);

    bool is_bar = arg_get_lit(ctx, 4);
    bool is_mix = arg_get_lit(ctx, 5);
    bool is_value = arg_get_lit(ctx, 6);
    CLIParserFree(ctx);

    if (divisor < 19) {
        PrintAndLogEx(ERR, "divisor must be between 19 and 255");
        return PM3_EINVARG;
    }

    if ((freq < 47) || (freq > 600)) {
        PrintAndLogEx(ERR, "freq must be between 47 and 600");
        return PM3_EINVARG;
    }
    divisor = LF_FREQ2DIV(freq);

    if ((is_bar + is_mix + is_value) > 1) {
        PrintAndLogEx(ERR, "Select only one output style");
        return PM3_EINVARG;
    }

    barMode_t style = session.bar_mode;
    if (is_bar)
        style = STYLE_BAR;
    if (is_mix)
        style = STYLE_MIXED;
    if (is_value)
        style = STYLE_VALUE;

    PrintAndLogEx(INFO, "Measuring LF antenna at " _YELLOW_("%.2f") " kHz, click " _GREEN_("pm3 button") " or press " _GREEN_("Enter") " to exit", LF_DIV2FREQ(divisor));

    uint8_t params[] = {1, 0};
    params[1] = divisor;
    PacketResponseNG resp;
    clearCommandBuffer();

    SendCommandNG(CMD_MEASURE_ANTENNA_TUNING_LF, params, sizeof(params));
    if (!WaitForResponseTimeout(CMD_MEASURE_ANTENNA_TUNING_LF, &resp, 1000)) {
        PrintAndLogEx(WARNING, "Timeout while waiting for Proxmark LF initialization, aborting");
        return PM3_ETIMEOUT;
    }

    params[0] = 2;

//    #define MAX_ADC_LF_VOLTAGE 140800
    uint32_t max = 71000;
    bool first = true;

    print_progress(0, max, style);

    // loop forever (till button pressed) if iter = 0 (default)
    for (uint8_t i = 0; iter == 0 || i < iter; i++) {
        if (kbd_enter_pressed()) {
            break;
        }

        SendCommandNG(CMD_MEASURE_ANTENNA_TUNING_LF, params, sizeof(params));
        if (!WaitForResponseTimeout(CMD_MEASURE_ANTENNA_TUNING_LF, &resp, 1000)) {
            PrintAndLogEx(NORMAL, "");
            PrintAndLogEx(WARNING, "Timeout while waiting for Proxmark LF measure, aborting");
            break;
        }

        if ((resp.status == PM3_EOPABORTED) || (resp.length != sizeof(uint32_t))) {
            PrintAndLogEx(NORMAL, "");
            break;
        }

        uint32_t volt = resp.data.asDwords[0];
        if (first) {
            max = (volt * 1.03);
            first = false;
        }
        if (volt > max) {
            max = (volt * 1.03);
        }
        print_progress(volt, max, style);
    }

    params[0] = 3;
    SendCommandNG(CMD_MEASURE_ANTENNA_TUNING_LF, params, sizeof(params));
    if (!WaitForResponseTimeout(CMD_MEASURE_ANTENNA_TUNING_LF, &resp, 1000)) {
        PrintAndLogEx(WARNING, "Timeout while waiting for Proxmark LF shutdown, aborting");
        return PM3_ETIMEOUT;
    }
    PrintAndLogEx(NORMAL, "\x1b%c[2K\r", 30);
    PrintAndLogEx(INFO, "Done.");
    return PM3_SUCCESS;
}

/* send a LF command before reading */
int CmdLFCommandRead(const char *Cmd) {

    if (!session.pm3_present) return PM3_ENOTTY;

    bool errors = false;
    bool verbose = true;
    bool continuous = false;
    uint32_t samples = 0;
    uint16_t datalen = 0;

    const uint8_t payload_header_size = 12 + (3 * LF_CMDREAD_MAX_EXTRA_SYMBOLS);
    struct p {
        uint32_t delay;
        uint16_t period_0;
        uint16_t period_1;
        uint8_t  symbol_extra[LF_CMDREAD_MAX_EXTRA_SYMBOLS];
        uint16_t period_extra[LF_CMDREAD_MAX_EXTRA_SYMBOLS];
        uint32_t samples : 31;
        bool     verbose : 1;
        uint8_t data[PM3_CMD_DATA_SIZE - payload_header_size];
    } PACKED payload;
    payload.samples = samples;
    payload.verbose = verbose;
    uint8_t index_extra = 0;

    uint8_t cmdp = 0;
    while (param_getchar(Cmd, cmdp) != 0x00 && !errors) {
        switch (tolower(param_getchar(Cmd, cmdp))) {
            case 'h':
                return usage_lf_cmdread();
            case 'c':  // cmd bytes 1010
                datalen = param_getstr(Cmd, cmdp + 1, (char *)&payload.data, sizeof(payload.data));
                cmdp += 2;
                break;
            case 'd':  // delay
                payload.delay = param_get32ex(Cmd, cmdp + 1, 0, 10);
                cmdp += 2;
                break;
            case 'z':  // zero
                payload.period_0 = param_get32ex(Cmd, cmdp + 1, 0, 10) & 0xFFFF;
                cmdp += 2;
                break;
            case 'o':  // ones
                payload.period_1 = param_get32ex(Cmd, cmdp + 1, 0, 10) & 0xFFFF;
                cmdp += 2;
                break;
            case 'e':  // extra symbol definition
                if (index_extra < LF_CMDREAD_MAX_EXTRA_SYMBOLS - 1) {
                    payload.symbol_extra[index_extra] = param_getchar(Cmd, cmdp + 1);
                    payload.period_extra[index_extra] = param_get32ex(Cmd, cmdp + 2, 0, 10) & 0xFFFF;
                    index_extra++;
                    cmdp += 3;
                } else {
                    PrintAndLogEx(WARNING, "Too many extra symbols, please define up to %i symbols", LF_CMDREAD_MAX_EXTRA_SYMBOLS);
                    errors = true;
                }
                break;
            case 's':
                samples = param_get32ex(Cmd, cmdp + 1, 0, 10);
                payload.samples = samples;
                cmdp += 2;
                break;
            case 'q':
                verbose = false;
                payload.verbose = verbose;
                cmdp++;
                break;
            case '@':
                continuous = true;
                cmdp++;
                break;
            default:
                PrintAndLogEx(WARNING, "Unknown parameter '%c'", param_getchar(Cmd, cmdp));
                errors = true;
                break;
        }
    }

    // bitbang mode
    if (payload.delay == 0) {
        if (payload.period_0 < 7 || payload.period_1 < 7) {
            PrintAndLogEx(WARNING, "warning periods cannot be less than 7us in bit bang mode");
            return PM3_EINVARG;
        }
    }

    //Validations
    if (errors || cmdp == 0)  return usage_lf_cmdread();
    if (continuous) {
        PrintAndLogEx(INFO, "Press " _GREEN_("Enter") " to exit");
    }
    if (verbose) {
        PrintAndLogEx(SUCCESS, "Sending command...");
    }

    int ret = PM3_SUCCESS;
    do {
        clearCommandBuffer();
        SendCommandNG(CMD_LF_MOD_THEN_ACQ_RAW_ADC, (uint8_t *)&payload, payload_header_size + datalen);

        PacketResponseNG resp;

        uint8_t i = 10;
        // 20sec wait loop
        while (!WaitForResponseTimeout(CMD_LF_MOD_THEN_ACQ_RAW_ADC, &resp, 2000) && i != 0) {
            if (verbose) {
                PrintAndLogEx(NORMAL, "." NOLF);
            }
            i--;
        }
        if (verbose) {
            PrintAndLogEx(NORMAL, "");
        }
        if (resp.status == PM3_SUCCESS) {
            if (i) {
                if (verbose) {
                    PrintAndLogEx(SUCCESS, "downloading response signal data");
                }
                getSamples(samples, false);
                ret = PM3_SUCCESS;
            } else {
                PrintAndLogEx(WARNING, "timeout while waiting for reply.");
                return PM3_ETIMEOUT;
            }
        } else {
            PrintAndLogEx(WARNING, "command failed.");
            return PM3_ESOFT;
        }
        if (kbd_enter_pressed()) {
            break;
        }
    } while (continuous);
    return ret;
}

int CmdFlexdemod(const char *Cmd) {
    (void)Cmd; // Cmd is not used so far

#ifndef LONG_WAIT
#define LONG_WAIT 100
#endif
    int i, j, start, bit, sum;

    int data[GraphTraceLen];
    memcpy(data, GraphBuffer, GraphTraceLen);

    size_t size = GraphTraceLen;

    for (i = 0; i < GraphTraceLen; ++i)
        data[i] = (data[i] < 0) ? -1 : 1;

    for (start = 0; start < size - LONG_WAIT; start++) {
        int first = data[start];
        for (i = start; i < start + LONG_WAIT; i++) {
            if (data[i] != first) {
                break;
            }
        }
        if (i == (start + LONG_WAIT))
            break;
    }

    if (start == size - LONG_WAIT) {
        PrintAndLogEx(WARNING, "nothing to wait for");
        return PM3_ENODATA;
    }

    data[start] = 4;
    data[start + 1] = 0;

    uint8_t bits[64] = {0x00};

    i = start;
    for (bit = 0; bit < 64; bit++) {
        sum = 0;
        for (j = 0; j < 16; j++) {
            sum += data[i++];
        }
        bits[bit] = (sum > 0) ? 1 : 0;
        PrintAndLogEx(NORMAL, "bit %d sum %d", bit, sum);
    }

    for (bit = 0; bit < 64; bit++) {
        sum = 0;
        for (j = 0; j < 16; j++)
            sum += data[i++];

        if (sum > 0 && bits[bit] != 1) PrintAndLogEx(WARNING, "oops1 at %d", bit);

        if (sum < 0 && bits[bit] != 0) PrintAndLogEx(WARNING, "oops2 at %d", bit);

    }

    // iceman,  use demod buffer?  blue line?
    // HACK writing back to graphbuffer.
    GraphTraceLen = 32 * 64;
    i = 0;
    for (bit = 0; bit < 64; bit++) {

        int phase = (bits[bit] == 0) ? 0 : 1;

        for (j = 0; j < 32; j++) {
            GraphBuffer[i++] = phase;
            phase = !phase;
        }
    }
    RepaintGraphWindow();
    return PM3_SUCCESS;
}

int lf_getconfig(sample_config *config) {
    if (!session.pm3_present) return PM3_ENOTTY;

    if (config == NULL)
        return PM3_EINVARG;

    clearCommandBuffer();

    SendCommandNG(CMD_LF_SAMPLING_GET_CONFIG, NULL, 0);
    PacketResponseNG resp;
    if (!WaitForResponseTimeout(CMD_LF_SAMPLING_GET_CONFIG, &resp, 2000)) {
        PrintAndLogEx(WARNING, "command execution time out");
        return PM3_ETIMEOUT;
    }
    memcpy(config, resp.data.asBytes, sizeof(sample_config));
    return PM3_SUCCESS;
}

int lf_config(sample_config *config) {
    if (!session.pm3_present) return PM3_ENOTTY;

    clearCommandBuffer();
    if (config != NULL)
        SendCommandNG(CMD_LF_SAMPLING_SET_CONFIG, (uint8_t *)config, sizeof(sample_config));
    else
        SendCommandNG(CMD_LF_SAMPLING_PRINT_CONFIG, NULL, 0);

    return PM3_SUCCESS;
}

int CmdLFConfig(const char *Cmd) {

    if (!session.pm3_present) return PM3_ENOTTY;

    // if called with no params, just print the device config
    if (strlen(Cmd) == 0) {
        return lf_config(NULL);
    }

    sample_config config = {
        .decimation = -1,
        .bits_per_sample = -1,
        .averaging = -1,
        .divisor = -1,
        .trigger_threshold = -1,
        .samples_to_skip = -1,
        .verbose = true
    };

    bool errors = false;

    uint8_t cmdp = 0;
    while (param_getchar(Cmd, cmdp) != 0x00 && !errors) {
        switch (param_getchar(Cmd, cmdp)) {
            case 'h':
                return usage_lf_config();
            case 'H':
                config.divisor = LF_DIVISOR_134;
                cmdp++;
                break;
            case 'L':
                config.divisor = LF_DIVISOR_125;
                cmdp++;
                break;
            case 'q':
                config.divisor = param_get8ex(Cmd, cmdp + 1, 95, 10);
                if (config.divisor < 19) {
                    PrintAndLogEx(ERR, "divisor must be between 19 and 255");
                    return PM3_EINVARG;
                }
                cmdp += 2;
                break;
            case 'f': {
                int freq = param_get32ex(Cmd, cmdp + 1, 125, 10);
                config.divisor = LF_FREQ2DIV(freq);
                if (config.divisor < 19) {
                    PrintAndLogEx(ERR, "freq must be between 47 and 600");
                    return PM3_EINVARG;
                }
                cmdp += 2;
                break;
            }
            case 't': {
                uint8_t trigg = 0;
                errors |= param_getdec(Cmd, cmdp + 1, &trigg);
                cmdp += 2;
                if (!errors) {
                    config.trigger_threshold = trigg;
                    g_lf_threshold_set = (config.trigger_threshold > 0);
                }
                break;
            }
            case 'b': {
                config.bits_per_sample = param_get8ex(Cmd, cmdp + 1, 8, 10);

                // bps is limited to 8
                if (config.bits_per_sample >> 4)
                    config.bits_per_sample = 8;

                cmdp += 2;
                break;
            }
            case 'd': {
                config.decimation = param_get8ex(Cmd, cmdp + 1, 1, 10);

                // decimation is limited to 255
                if (config.decimation >> 4)
                    config.decimation = 8;

                cmdp += 2;
                break;
            }
            case 'a':
                config.averaging = (param_getchar(Cmd, cmdp + 1) == '1');
                cmdp += 2;
                break;
            case 's':
                config.samples_to_skip = param_get32ex(Cmd, cmdp + 1, 0, 10);
                cmdp += 2;
                break;
            default:
                PrintAndLogEx(WARNING, "Unknown parameter '%c'", param_getchar(Cmd, cmdp));
                errors = 1;
                break;
        }
    }

    // validations
    if (errors) return usage_lf_config();

    return lf_config(&config);
}

int lf_read(bool verbose, uint32_t samples) {
    if (!session.pm3_present) return PM3_ENOTTY;

    struct p {
        uint32_t samples : 31;
        bool     verbose : 1;
    } PACKED;

    struct p payload;
    payload.verbose = verbose;
    payload.samples = samples;

    clearCommandBuffer();
    SendCommandNG(CMD_LF_ACQ_RAW_ADC, (uint8_t *)&payload, sizeof(payload));
    PacketResponseNG resp;
    if (g_lf_threshold_set) {
        WaitForResponse(CMD_LF_ACQ_RAW_ADC, &resp);
    } else {
        if (!WaitForResponseTimeout(CMD_LF_ACQ_RAW_ADC, &resp, 2500)) {
            PrintAndLogEx(WARNING, "(lf_read) command execution time out");
            return PM3_ETIMEOUT;
        }
    }

    // response is number of bits read
    uint32_t size = (resp.data.asDwords[0] / 8);
    getSamples(size, verbose);
    return PM3_SUCCESS;
}

int CmdLFRead(const char *Cmd) {

    if (!session.pm3_present) return PM3_ENOTTY;

    bool errors = false;
    bool verbose = true;
    bool continuous = false;
    uint32_t samples = 0;
    uint8_t cmdp = 0;
    while (param_getchar(Cmd, cmdp) != 0x00 && !errors) {
        switch (tolower(param_getchar(Cmd, cmdp))) {
            case 'h':
                return usage_lf_read();
            case 's':
                samples = param_get32ex(Cmd, cmdp + 1, 0, 10);
                cmdp += 2;
                break;
            case 'q':
                verbose = false;
                cmdp++;
                break;
            case '@':
                continuous = true;
                cmdp++;
                break;
            default:
                PrintAndLogEx(WARNING, "Unknown parameter '%c'", param_getchar(Cmd, cmdp));
                errors = true;
                break;
        }
    }

    //Validations
    if (errors) return usage_lf_read();
    if (continuous) {
        PrintAndLogEx(INFO, "Press " _GREEN_("Enter") " to exit");
    }
    int ret = PM3_SUCCESS;
    do {
        ret = lf_read(verbose, samples);
        if (kbd_enter_pressed()) {
            break;
        }
    } while (continuous);
    return ret;
}

int lf_sniff(bool verbose, uint32_t samples) {
    if (!session.pm3_present) return PM3_ENOTTY;

    struct p {
        uint32_t samples : 31;
        bool     verbose : 1;
    } PACKED;

    struct p payload;
    payload.verbose = verbose;
    payload.samples = samples;

    clearCommandBuffer();
    SendCommandNG(CMD_LF_SNIFF_RAW_ADC, (uint8_t *)&payload, sizeof(payload));
    PacketResponseNG resp;
    if (g_lf_threshold_set) {
        WaitForResponse(CMD_LF_SNIFF_RAW_ADC, &resp);
    } else {
        if (!WaitForResponseTimeout(CMD_LF_SNIFF_RAW_ADC, &resp, 2500)) {
            PrintAndLogEx(WARNING, "(lf_read) command execution time out");
            return PM3_ETIMEOUT;
        }
    }

    // response is number of bits read
    uint32_t size = (resp.data.asDwords[0] / 8);
    getSamples(size, verbose);
    return PM3_SUCCESS;
}

int CmdLFSniff(const char *Cmd) {

    if (!session.pm3_present) return PM3_ENOTTY;

    bool errors = false;
    bool verbose = true;
    bool continuous = false;
    uint32_t samples = 0;
    uint8_t cmdp = 0;
    while (param_getchar(Cmd, cmdp) != 0x00 && !errors) {
        switch (tolower(param_getchar(Cmd, cmdp))) {
            case 'h':
                return usage_lf_sniff();
            case 's':
                samples = param_get32ex(Cmd, cmdp + 1, 0, 10);
                cmdp += 2;
                break;
            case 'q':
                verbose = false;
                cmdp++;
                break;
            case '@':
                continuous = true;
                cmdp++;
                break;
            default:
                PrintAndLogEx(WARNING, "Unknown parameter '%c'", param_getchar(Cmd, cmdp));
                errors = true;
                break;
        }
    }

    //Validations
    if (errors) return usage_lf_sniff();
    if (continuous) {
        PrintAndLogEx(INFO, "Press " _GREEN_("Enter") " to exit");
    }
    int ret = PM3_SUCCESS;
    do {
        ret = lf_sniff(verbose, samples);
        if (kbd_enter_pressed()) {
            break;
        }
    } while (continuous);
    return ret;
}

static void ChkBitstream(void) {
    // convert to bitstream if necessary
    for (int i = 0; i < (int)(GraphTraceLen / 2); i++) {
        if (GraphBuffer[i] > 1 || GraphBuffer[i] < 0) {
            CmdGetBitStream("");
            PrintAndLogEx(INFO, "Converted to bitstream");
            break;
        }
    }
}

//Attempt to simulate any wave in buffer (one bit per output sample)
// converts GraphBuffer to bitstream (based on zero crossings) if needed.
int CmdLFSim(const char *Cmd) {

    if (!session.pm3_present) return PM3_ENOTTY;

    // sanity check
    if (GraphTraceLen < 20) {
        PrintAndLogEx(ERR, "No data in Graphbuffer");
        return PM3_ENODATA;
    }

    uint8_t cmdp = tolower(param_getchar(Cmd, 0));
    if (cmdp == 'h') return usage_lf_sim();

    uint16_t gap = param_get32ex(Cmd, 0, 0, 10) & 0xFFFF;

    // convert to bitstream if necessary
    ChkBitstream();

    PrintAndLogEx(DEBUG, "DEBUG: Uploading %zu bytes", GraphTraceLen);

    PacketResponseNG resp;

    struct pupload {
        uint8_t flag;
        uint16_t offset;
        uint8_t data[PM3_CMD_DATA_SIZE - 3];
    } PACKED payload_up;

    // flag =
    //    b0  0
    //        1 clear bigbuff
    payload_up.flag = 0x1;

    // fast push mode
    conn.block_after_ACK = true;

    //can send only 512 bits at a time (1 byte sent per bit...)
    for (uint16_t i = 0; i < GraphTraceLen; i += PM3_CMD_DATA_SIZE - 3) {

        size_t len = MIN((GraphTraceLen - i), PM3_CMD_DATA_SIZE - 3);
        clearCommandBuffer();
        payload_up.offset = i;

        for (uint16_t j = 0; j < len; j++)
            payload_up.data[j] = GraphBuffer[i + j];

        SendCommandNG(CMD_LF_UPLOAD_SIM_SAMPLES, (uint8_t *)&payload_up, sizeof(struct pupload));
        WaitForResponse(CMD_LF_UPLOAD_SIM_SAMPLES, &resp);
        if (resp.status != PM3_SUCCESS) {
            PrintAndLogEx(INFO, "Bigbuf is full.");
            break;
        }
        PrintAndLogEx(NORMAL, "." NOLF);
        payload_up.flag = 0;
    }

    // Disable fast mode before last command
    conn.block_after_ACK = false;
    PrintAndLogEx(NORMAL, "");
    PrintAndLogEx(INFO, "Simulating");

    struct p {
        uint16_t len;
        uint16_t gap;
    } PACKED payload;
    payload.len = GraphTraceLen;
    payload.gap = gap;

    clearCommandBuffer();
    SendCommandNG(CMD_LF_SIMULATE, (uint8_t *)&payload, sizeof(payload));

    WaitForResponse(CMD_LF_SIMULATE, &resp);

    PrintAndLogEx(INFO, "Done");
    if (resp.status != PM3_EOPABORTED)
        return resp.status;
    return PM3_SUCCESS;
}

// sim fsk data given clock, fcHigh, fcLow, invert
// - allow pull data from DemodBuffer
int CmdLFfskSim(const char *Cmd) {
    //might be able to autodetect FCs and clock from Graphbuffer if using demod buffer
    // otherwise will need FChigh, FClow, Clock, and bitstream
    uint8_t fcHigh = 0, fcLow = 0, clk = 0;
    bool errors = false, separator = false;
    char hexData[64] = {0x00}; // store entered hex data
    uint8_t data[255] = {0x00};
    int dataLen = 0;
    uint8_t cmdp = 0;

    while (param_getchar(Cmd, cmdp) != 0x00 && !errors) {
        switch (param_getchar(Cmd, cmdp)) {
            case 'h':
                return usage_lf_simfsk();
            case 'c':
                errors |= param_getdec(Cmd, cmdp + 1, &clk);
                cmdp += 2;
                break;
            case 'H':
                errors |= param_getdec(Cmd, cmdp + 1, &fcHigh);
                cmdp += 2;
                break;
            case 'L':
                errors |= param_getdec(Cmd, cmdp + 1, &fcLow);
                cmdp += 2;
                break;
            case 's':
                separator = true;
                cmdp++;
                break;
            case 'd':
                dataLen = param_getstr(Cmd, cmdp + 1, hexData, sizeof(hexData));
                if (dataLen == 0)
                    errors = true;
                else
                    dataLen = hextobinarray((char *)data, hexData);

                if (dataLen == 0) errors = true;
                if (errors) PrintAndLogEx(ERR, "Error getting hex data");
                cmdp += 2;
                break;
            default:
                PrintAndLogEx(WARNING, "Unknown parameter '%c'", param_getchar(Cmd, cmdp));
                errors = true;
                break;
        }
    }

    // No args
    if (cmdp == 0 && DemodBufferLen == 0) return usage_lf_simfsk();

    //Validations
    if (errors) return usage_lf_simfsk();

    int firstClockEdge = 0;
    if (dataLen == 0) { //using DemodBuffer
        if (clk == 0 || fcHigh == 0 || fcLow == 0) { //manual settings must set them all
            uint8_t ans = fskClocks(&fcHigh, &fcLow, &clk, &firstClockEdge);
            if (ans == 0) {
                if (!fcHigh) fcHigh = 10;
                if (!fcLow) fcLow = 8;
                if (!clk) clk = 50;
            }
        }
    } else {
        setDemodBuff(data, dataLen, 0);
    }

    //default if not found
    if (clk == 0) clk = 50;
    if (fcHigh == 0) fcHigh = 10;
    if (fcLow == 0) fcLow = 8;

    size_t size = DemodBufferLen;
    if (size > (PM3_CMD_DATA_SIZE - sizeof(lf_fsksim_t))) {
        PrintAndLogEx(WARNING, "DemodBuffer too long for current implementation - length: %zu - max: %zu", size, PM3_CMD_DATA_SIZE - sizeof(lf_fsksim_t));
        size = PM3_CMD_DATA_SIZE - sizeof(lf_fsksim_t);
    }

    lf_fsksim_t *payload = calloc(1, sizeof(lf_fsksim_t) + size);
    payload->fchigh = fcHigh;
    payload->fclow =  fcLow;
    payload->separator = separator;
    payload->clock = clk;
    memcpy(payload->data, DemodBuffer, size);

    PrintAndLogEx(INFO, "Simulating");

    clearCommandBuffer();
    SendCommandNG(CMD_LF_FSK_SIMULATE, (uint8_t *)payload,  sizeof(lf_fsksim_t) + size);
    free(payload);

    setClockGrid(clk, 0);
    PacketResponseNG resp;
    WaitForResponse(CMD_LF_FSK_SIMULATE, &resp);

    PrintAndLogEx(INFO, "Done");
    if (resp.status != PM3_EOPABORTED)
        return resp.status;
    return PM3_SUCCESS;
}

// sim ask data given clock, invert, manchester or raw, separator
// - allow pull data from DemodBuffer
int CmdLFaskSim(const char *Cmd) {
    // autodetect clock from Graphbuffer if using demod buffer
    // needs clock, invert, manchester/raw as m or r, separator as s, and bitstream
    uint8_t encoding = 1, separator = 0, clk = 0, invert = 0;
    bool errors = false;
    char hexData[64] = {0x00};
    uint8_t data[255] = {0x00}; // store entered hex data
    int dataLen = 0;
    uint8_t cmdp = 0;

    while (param_getchar(Cmd, cmdp) != 0x00 && !errors) {
        switch (tolower(param_getchar(Cmd, cmdp))) {
            case 'h':
                return usage_lf_simask();
            case 'i':
                invert = 1;
                cmdp++;
                break;
            case 'c':
                errors |= param_getdec(Cmd, cmdp + 1, &clk);
                cmdp += 2;
                break;
            case 'b':
                encoding = 2; //biphase
                cmdp++;
                break;
            case 'm':
                encoding = 1; //manchester
                cmdp++;
                break;
            case 'r':
                encoding = 0; //raw
                cmdp++;
                break;
            case 's':
                separator = 1;
                cmdp++;
                break;
            case 'd':
                dataLen = param_getstr(Cmd, cmdp + 1, hexData, sizeof(hexData));
                if (dataLen == 0)
                    errors = true;
                else
                    dataLen = hextobinarray((char *)data, hexData);

                if (dataLen == 0) errors = true;
                if (errors) PrintAndLogEx(ERR, "Error getting hex data, datalen: %d", dataLen);
                cmdp += 2;
                break;
            default:
                PrintAndLogEx(WARNING, "Unknown parameter '%c'", param_getchar(Cmd, cmdp));
                errors = true;
                break;
        }
    }

    // No args
    if (cmdp == 0 && DemodBufferLen == 0) return usage_lf_simask();

    //Validations
    if (errors) return usage_lf_simask();

    if (dataLen == 0) { //using DemodBuffer
        if (clk == 0)
            clk = GetAskClock("0", false);
    } else {
        setDemodBuff(data, dataLen, 0);
    }
    if (clk == 0) clk = 64;
    if (encoding == 0) clk /= 2; //askraw needs to double the clock speed

    size_t size = DemodBufferLen;
    if (size > (PM3_CMD_DATA_SIZE - sizeof(lf_asksim_t))) {
        PrintAndLogEx(WARNING, "DemodBuffer too long for current implementation - length: %zu - max: %zu", size, PM3_CMD_DATA_SIZE - sizeof(lf_asksim_t));
        size = PM3_CMD_DATA_SIZE - sizeof(lf_asksim_t);
    }

    lf_asksim_t *payload = calloc(1, sizeof(lf_asksim_t) + size);
    payload->encoding =  encoding;
    payload->invert = invert;
    payload->separator = separator;
    payload->clock = clk;
    memcpy(payload->data, DemodBuffer, size);

    PrintAndLogEx(INFO, "Simulating");

    clearCommandBuffer();
    SendCommandNG(CMD_LF_ASK_SIMULATE, (uint8_t *)payload,  sizeof(lf_asksim_t) + size);
    free(payload);

    PacketResponseNG resp;
    WaitForResponse(CMD_LF_ASK_SIMULATE, &resp);

    PrintAndLogEx(INFO, "Done");
    if (resp.status != PM3_EOPABORTED)
        return resp.status;
    return PM3_SUCCESS;
}

// sim psk data given carrier, clock, invert
// - allow pull data from DemodBuffer or parameters
int CmdLFpskSim(const char *Cmd) {
    //might be able to autodetect FC and clock from Graphbuffer if using demod buffer
    //will need carrier, Clock, and bitstream
    uint8_t carrier = 0, clk = 0;
    uint8_t invert = 0;
    bool errors = false;
    char hexData[64] = {0x00}; // store entered hex data
    uint8_t data[255] = {0x00};
    int dataLen = 0;
    uint8_t cmdp = 0;
    uint8_t pskType = 1;

    while (param_getchar(Cmd, cmdp) != 0x00 && !errors) {
        switch (tolower(param_getchar(Cmd, cmdp))) {
            case 'h':
                return usage_lf_simpsk();
            case 'i':
                invert = 1;
                cmdp++;
                break;
            case 'c':
                errors |= param_getdec(Cmd, cmdp + 1, &clk);
                cmdp += 2;
                break;
            case 'r':
                errors |= param_getdec(Cmd, cmdp + 1, &carrier);
                cmdp += 2;
                break;
            case '1':
                pskType = 1;
                cmdp++;
                break;
            case '2':
                pskType = 2;
                cmdp++;
                break;
            case '3':
                pskType = 3;
                cmdp++;
                break;
            case 'd':
                dataLen = param_getstr(Cmd, cmdp + 1, hexData, sizeof(hexData));
                if (dataLen == 0)
                    errors = true;
                else
                    dataLen = hextobinarray((char *)data, hexData);

                if (dataLen == 0) errors = true;
                if (errors) PrintAndLogEx(ERR, "Error getting hex data");
                cmdp += 2;
                break;
            default:
                PrintAndLogEx(WARNING, "Unknown parameter '%c'", param_getchar(Cmd, cmdp));
                errors = true;
                break;
        }
    }
    // No args
    if (cmdp == 0 && DemodBufferLen == 0)
        errors = true;

    //Validations
    if (errors) return usage_lf_simpsk();

    if (dataLen == 0) { //using DemodBuffer
        PrintAndLogEx(INFO, "Getting Clocks");

        if (clk == 0) clk = GetPskClock("", false);
        PrintAndLogEx(INFO, "clk: %d", clk);

        if (!carrier) carrier = GetPskCarrier(false);
        PrintAndLogEx(INFO, "carrier: %d", carrier);

    } else {
        setDemodBuff(data, dataLen, 0);
    }

    if (clk == 0) clk = 32;

    if (carrier != 2 && carrier != 4 && carrier != 8)
        carrier = 2;

    if (pskType != 1) {
        if (pskType == 2) {
            //need to convert psk2 to psk1 data before sim
            psk2TOpsk1(DemodBuffer, DemodBufferLen);
        } else {
            PrintAndLogEx(WARNING, "Sorry, PSK3 not yet available");
        }
    }
    size_t size = DemodBufferLen;
    if (size > (PM3_CMD_DATA_SIZE - sizeof(lf_psksim_t))) {
        PrintAndLogEx(WARNING, "DemodBuffer too long for current implementation - length: %zu - max: %zu", size, PM3_CMD_DATA_SIZE - sizeof(lf_psksim_t));
        size = PM3_CMD_DATA_SIZE - sizeof(lf_psksim_t);
    }

    lf_psksim_t *payload = calloc(1, sizeof(lf_psksim_t) + size);
    payload->carrier =  carrier;
    payload->invert = invert;
    payload->clock = clk;
    memcpy(payload->data, DemodBuffer, size);

    PrintAndLogEx(INFO, "Simulating");

    clearCommandBuffer();
    SendCommandNG(CMD_LF_PSK_SIMULATE, (uint8_t *)payload,  sizeof(lf_psksim_t) + size);
    free(payload);

    PacketResponseNG resp;
    WaitForResponse(CMD_LF_PSK_SIMULATE, &resp);

    PrintAndLogEx(INFO, "Done");
    if (resp.status != PM3_EOPABORTED)
        return resp.status;
    return PM3_SUCCESS;
}

int CmdLFSimBidir(const char *Cmd) {

    CLIParserContext *ctx;
    CLIParserInit(&ctx, "lf simbidir",
                  "Simulate LF tag with bidirectional data transmission between reader and tag",
                  "lf simbidir"
                 );

    void *argtable[] = {
        arg_param_begin,
        arg_param_end
    };
    CLIExecWithReturn(ctx, Cmd, argtable, true);
    CLIParserFree(ctx);

    // Set ADC to twice the carrier for a slight supersampling
    // HACK: not implemented in ARMSRC.
    PrintAndLogEx(INFO, "Not implemented yet.");
//    SendCommandMIX(CMD_LF_SIMULATE_BIDIR, 47, 384, 0, NULL, 0);
    return PM3_SUCCESS;
}

// ICEMAN,  Verichip is Animal tag.  Tested against correct reader
/*

int CmdVchDemod(const char *Cmd) {

    if (GraphTraceLen < 4096) {
        PrintAndLogEx(DEBUG, "debug; VchDemod - too few samples");
        return PM3_EINVARG;
    }

    // Is this the entire sync pattern, or does this also include some
    // data bits that happen to be the same everywhere? That would be
    // lovely to know.
    static const int SyncPattern[] = {
        1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
        1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
        1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
        1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
        1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
        1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    };

    // iceman, using correlate as preamble detect seems way better than our current memcompare

    // So first, we correlate for the sync pattern, and mark that.
    int bestCorrel = 0, bestPos = 0;
    int i, j, sum = 0;

    // It does us no good to find the sync pattern, with fewer than 2048 samples after it.

    for (i = 0; i < (GraphTraceLen - 2048); i++) {
        for (j = 0; j < ARRAYLEN(SyncPattern); j++) {
            sum += GraphBuffer[i + j] * SyncPattern[j];
        }
        if (sum > bestCorrel) {
            bestCorrel = sum;
            bestPos = i;
        }
    }
    PrintAndLogEx(NORMAL, "best sync at %d [metric %d]", bestPos, bestCorrel);

    char bits[257];
    bits[256] = '\0';

    int worst = INT_MAX, worstPos = 0;

    for (i = 0; i < 2048; i += 8) {
        sum = 0;
        for (j = 0; j < 8; j++)
            sum += GraphBuffer[bestPos + i + j];

        if (sum < 0)
            bits[i / 8] = '.';
        else
            bits[i / 8] = '1';

        if (abs(sum) < worst) {
            worst = abs(sum);
            worstPos = i;
        }
    }
    PrintAndLogEx(NORMAL, "bits:");
    PrintAndLogEx(NORMAL, "%s", bits);
    PrintAndLogEx(NORMAL, "worst metric: %d at pos %d", worst, worstPos);

    // clone
    if (strcmp(Cmd, "clone") == 0) {
        GraphTraceLen = 0;
        char *s;
        for (s = bits; *s; s++) {
            for (j = 0; j < 16; j++) {
                GraphBuffer[GraphTraceLen++] = (*s == '1') ? 1 : 0;
            }
        }
        RepaintGraphWindow();
    }
    return PM3_SUCCESS;
}
*/

static bool CheckChipType(bool getDeviceData) {

    bool retval = false;

    if (!getDeviceData) return retval;

    save_restoreGB(GRAPH_SAVE);
    save_restoreDB(GRAPH_SAVE);

    //check for em4x05/em4x69 chips first
    uint32_t word = 0;
    if (em4x05_isblock0(&word)) {
        PrintAndLogEx(SUCCESS, "Chipset detection: " _GREEN_("EM4x05 / EM4x69"));
        PrintAndLogEx(HINT, "Hint: try " _YELLOW_("`lf em 4x05`") " commands");
        retval = true;
        goto out;
    }

    //check for t55xx chip...
    if (tryDetectP1(true)) {
        PrintAndLogEx(SUCCESS, "Chipset detection: " _GREEN_("T55xx"));
        PrintAndLogEx(HINT, "Hint: try " _YELLOW_("`lf t55xx`") " commands");
        retval = true;
        goto out;
    }

    // check for em4x50 chips
    if (detect_4x50_block()) {
        PrintAndLogEx(SUCCESS, "Chipset detection: " _GREEN_("EM4x50"));
        PrintAndLogEx(HINT, "Hint: try " _YELLOW_("`lf em 4x50`") " commands");
        retval = true;
        goto out;
    }

    // check for em4x70 chips
    if (detect_4x70_block()) {
        PrintAndLogEx(SUCCESS, "Chipset detection: " _GREEN_("EM4x70"));
        PrintAndLogEx(HINT, "Hint: try " _YELLOW_("`lf em 4x70`") " commands");
        retval = true;
        goto out;
    }

    PrintAndLogEx(NORMAL, "Couldn't identify a chipset");
out:
    save_restoreGB(GRAPH_RESTORE);
    save_restoreDB(GRAPH_RESTORE);
    return retval;
}

int CmdLFfind(const char *Cmd) {

    CLIParserContext *ctx;
    CLIParserInit(&ctx, "lf search",
                  "Read and search for valid known tag. For offline mode, you can `data load` first then search.",
                  "lf search       -> try reading data from tag & search for known tag\n"
                  "lf search -1    -> use data from GraphBuffer & search for known tag\n"
                  "lf search -u    -> try reading data from tag & search for known and unknown tag\n"
                  "lf search -1u   -> use data from GraphBuffer & search for known and unknown tag\n"
                 );

    void *argtable[] = {
        arg_param_begin,
        arg_lit0("1", NULL, "Use data from Graphbuffer to search"),
        arg_lit0("u", NULL, "Search for unknown tags, if not set, reads only known tags"),
        arg_param_end
    };
    CLIExecWithReturn(ctx, Cmd, argtable, true);
    bool use_gb = arg_get_lit(ctx, 1);
    bool search_unk = arg_get_lit(ctx, 2);
    CLIParserFree(ctx);

    bool is_online = (session.pm3_present && (use_gb == false));
    if (is_online)
        lf_read(false, 30000);

    size_t min_length = 2000;
    if (GraphTraceLen < min_length) {
        PrintAndLogEx(FAILED, "Data in Graphbuffer was too small.");
        return PM3_ESOFT;
    }

    PrintAndLogEx(NORMAL, "");
    PrintAndLogEx(INFO, "NOTE: some demods output possible binary");
    PrintAndLogEx(INFO, "if it finds something that looks like a tag");
    PrintAndLogEx(INFO, "False Positives " _YELLOW_("ARE") " possible");
    PrintAndLogEx(INFO, "");
    PrintAndLogEx(INFO, "Checking for known tags...");
    PrintAndLogEx(INFO, "");

    // only run these tests if device is online
    if (is_online) {

        if (IfPm3Hitag()) {
            if (readHitagUid()) {
                PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("Hitag") " found!");
                return PM3_SUCCESS;
            }
        }

        if (IfPm3EM4x50()) {
            if (read_em4x50_uid() == PM3_SUCCESS) {
                PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("EM4x50 ID") " found!");
                return PM3_SUCCESS;
            }
        }

        // only run if graphbuffer is just noise as it should be for hitag
        // The improved noise detection will find Cotag.
        if (getSignalProperties()->isnoise) {

            PrintAndLogEx(INPLACE, "Searching for MOTOROLA tag...");
            if (readMotorolaUid()) {
                PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("Motorola FlexPass ID") " found!");
                return PM3_SUCCESS;
            }

            PrintAndLogEx(INPLACE, "Searching for COTAG tag...");
            if (readCOTAGUid()) {
                PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("COTAG ID") " found!");
                return PM3_SUCCESS;
            }

            PrintAndLogEx(NORMAL, "");
            PrintAndLogEx(FAILED, _RED_("No data found!"));
            PrintAndLogEx(INFO, "Signal looks like noise. Maybe not an LF tag?");
            PrintAndLogEx(NORMAL, "");
            return PM3_ESOFT;
        }
    }

    int retval = PM3_SUCCESS;

    if (demodVisa2k(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("Visa2000 ID") " found!"); goto out;}
    if (demodDestron(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("FDX-A FECAVA Destron ID") " found!"); goto out;} // to do before HID
    if (demodHID(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("HID Prox ID") " found!"); goto out;}
    if (demodAWID(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("AWID ID") " found!"); goto out;}
    if (demodIOProx(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("IO Prox ID") " found!"); goto out;}
    if (demodParadox(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("Paradox ID") " found!"); goto out;}
    if (demodNexWatch(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("NexWatch ID") " found!"); goto out;}
    if (demodIndala(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("Indala ID") " found!");  goto out;}
    if (demodEM410x(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("EM410x ID") " found!"); goto out;}
    if (demodFDXB(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("FDX-B ID") " found!"); goto out;}
    if (demodGuard(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("Guardall G-Prox II ID") " found!"); goto out; }
    if (demodIdteck(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("Idteck ID") " found!"); goto out;}
    if (demodJablotron(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("Jablotron ID") " found!"); goto out;}
    if (demodNedap(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("NEDAP ID") " found!"); goto out;}
    if (demodNoralsy(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("Noralsy ID") " found!"); goto out;}
    if (demodKeri(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("KERI ID") " found!"); goto out;}
    if (demodPac(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("PAC/Stanley ID") " found!"); goto out;}
    if (demodPresco(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("Presco ID") " found!"); goto out;}
    if (demodPyramid(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("Pyramid ID") " found!"); goto out;}
    if (demodSecurakey(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("Securakey ID") " found!"); goto out;}
    if (demodViking(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("Viking ID") " found!"); goto out;}
    if (demodGallagher(true) == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("GALLAGHER ID") " found!"); goto out;}

//    if (demodTI() == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("Texas Instrument ID") " found!"); goto out;}
//    if (demodFermax() == PM3_SUCCESS) { PrintAndLogEx(SUCCESS, "\nValid " _GREEN_("Fermax ID") " found!"); goto out;}

    PrintAndLogEx(FAILED, _RED_("No known 125/134 kHz tags found!"));

    if (search_unk) {
        //test unknown tag formats (raw mode)
        PrintAndLogEx(INFO, "\nChecking for unknown tags:\n");
        int ans = AutoCorrelate(GraphBuffer, GraphBuffer, GraphTraceLen, 8000, false, false);
        if (ans > 0) {

            PrintAndLogEx(INFO, "Possible auto correlation of %d repeating samples", ans);

            if (ans % 8 == 0)
                PrintAndLogEx(INFO, "Possible %d bytes", (ans / 8));
        }

        //fsk
        if (GetFskClock("", false)) {
            if (FSKrawDemod(0, 0, 0, 0, true) == PM3_SUCCESS) {
                PrintAndLogEx(NORMAL, "\nUnknown FSK Modulated Tag found!");
                goto out;
            }
        }

        bool st = true;
        if (ASKDemod_ext(0, 0, 0, 0, false, true, false, 1, &st) == PM3_SUCCESS) {
            PrintAndLogEx(NORMAL, "\nUnknown ASK Modulated and Manchester encoded Tag found!");
            PrintAndLogEx(NORMAL, "if it does not look right it could instead be ASK/Biphase - try " _YELLOW_("'data rawdemod ab'"));
            goto out;
        }

        if (CmdPSK1rawDemod("") == PM3_SUCCESS) {
            PrintAndLogEx(NORMAL, "Possible unknown PSK1 Modulated Tag found above!");
            PrintAndLogEx(NORMAL, "    Could also be PSK2 - try " _YELLOW_("'data rawdemod p2'"));
            PrintAndLogEx(NORMAL, "    Could also be PSK3 - [currently not supported]");
            PrintAndLogEx(NORMAL, "    Could also be  NRZ - try " _YELLOW_("'data rawdemod nr"));
            goto out;
        }

        PrintAndLogEx(FAILED, _RED_("\nNo data found!"));
    }

    retval = PM3_ESOFT;

out:
    // identify chipset
    if (CheckChipType(is_online) == false) {
        PrintAndLogEx(DEBUG, "Automatic chip type detection " _RED_("failed"));
        retval = false;
    }
    return retval;
}

static command_t CommandTable[] = {
    {"-----------", CmdHelp,            AlwaysAvailable, "-------------- " _CYAN_("Low Frequency") " --------------"},
    {"awid",        CmdLFAWID,          AlwaysAvailable, "{ AWID RFIDs...              }"},
    {"cotag",       CmdLFCOTAG,         AlwaysAvailable, "{ COTAG CHIPs...             }"},
    {"destron",     CmdLFDestron,       AlwaysAvailable, "{ FDX-A Destron RFIDs...     }"},
    {"em",          CmdLFEM,            AlwaysAvailable, "{ EM CHIPs & RFIDs...        }"},
    {"fdxb",        CmdLFFdxB,          AlwaysAvailable, "{ FDX-B RFIDs...             }"},
    {"gallagher",   CmdLFGallagher,     AlwaysAvailable, "{ GALLAGHER RFIDs...         }"},
    {"gproxii",     CmdLFGuard,         AlwaysAvailable, "{ Guardall Prox II RFIDs...  }"},
    {"hid",         CmdLFHID,           AlwaysAvailable, "{ HID Prox RFIDs...          }"},
    {"hitag",       CmdLFHitag,         AlwaysAvailable, "{ Hitag CHIPs...             }"},
    {"idteck",      CmdLFIdteck,        AlwaysAvailable, "{ Idteck RFIDs...            }"},
    {"indala",      CmdLFINDALA,        AlwaysAvailable, "{ Indala RFIDs...            }"},
    {"io",          CmdLFIO,            AlwaysAvailable, "{ ioProx RFIDs...            }"},
    {"jablotron",   CmdLFJablotron,     AlwaysAvailable, "{ Jablotron RFIDs...         }"},
    {"keri",        CmdLFKeri,          AlwaysAvailable, "{ KERI RFIDs...              }"},
    {"motorola",    CmdLFMotorola,      AlwaysAvailable, "{ Motorola RFIDs...          }"},
    {"nedap",       CmdLFNedap,         AlwaysAvailable, "{ Nedap RFIDs...             }"},
    {"nexwatch",    CmdLFNEXWATCH,      AlwaysAvailable, "{ NexWatch RFIDs...          }"},
    {"noralsy",     CmdLFNoralsy,       AlwaysAvailable, "{ Noralsy RFIDs...           }"},
    {"pac",         CmdLFPac,           AlwaysAvailable, "{ PAC/Stanley RFIDs...       }"},
    {"paradox",     CmdLFParadox,       AlwaysAvailable, "{ Paradox RFIDs...           }"},
    {"pcf7931",     CmdLFPCF7931,       AlwaysAvailable, "{ PCF7931 CHIPs...           }"},
    {"presco",      CmdLFPresco,        AlwaysAvailable, "{ Presco RFIDs...            }"},
    {"pyramid",     CmdLFPyramid,       AlwaysAvailable, "{ Farpointe/Pyramid RFIDs... }"},
    {"securakey",   CmdLFSecurakey,     AlwaysAvailable, "{ Securakey RFIDs...         }"},
    {"ti",          CmdLFTI,            AlwaysAvailable, "{ TI CHIPs...                }"},
    {"t55xx",       CmdLFT55XX,         AlwaysAvailable, "{ T55xx CHIPs...             }"},
    {"viking",      CmdLFViking,        AlwaysAvailable, "{ Viking RFIDs...            }"},
    {"visa2000",    CmdLFVisa2k,        AlwaysAvailable, "{ Visa2000 RFIDs...          }"},
    {"-----------", CmdHelp,            AlwaysAvailable, "--------------------- " _CYAN_("General") " ---------------------"},
    {"help",        CmdHelp,            AlwaysAvailable, "This help"},
    {"config",      CmdLFConfig,        IfPm3Lf,         "Get/Set config for LF sampling, bit/sample, decimation, frequency"},
    {"cmdread",     CmdLFCommandRead,   IfPm3Lf,         "Modulate LF reader field to send command before read (all periods in microseconds)"},
    {"read",        CmdLFRead,          IfPm3Lf,         "Read LF tag"},
    {"search",      CmdLFfind,          AlwaysAvailable, "Read and Search for valid known tag"},
    {"sim",         CmdLFSim,           IfPm3Lf,         "Simulate LF tag from buffer with optional GAP (in microseconds)"},
    {"simask",      CmdLFaskSim,        IfPm3Lf,         "Simulate " _YELLOW_("LF ASK tag") " from demodbuffer or input"},
    {"simfsk",      CmdLFfskSim,        IfPm3Lf,         "Simulate " _YELLOW_("LF FSK tag") " from demodbuffer or input"},
    {"simpsk",      CmdLFpskSim,        IfPm3Lf,         "Simulate " _YELLOW_("LF PSK tag") " from demodbuffer or input"},
//    {"simnrz",      CmdLFnrzSim,        IfPm3Lf,         "Simulate " _YELLOW_("LF NRZ tag") " from demodbuffer or input"},
    {"simbidir",    CmdLFSimBidir,      IfPm3Lf,         "Simulate LF tag (with bidirectional data transmission between reader and tag)"},
    {"sniff",       CmdLFSniff,         IfPm3Lf,         "Sniff LF traffic between reader and tag"},
    {"tune",        CmdLFTune,          IfPm3Lf,         "Continuously measure LF antenna tuning"},
//    {"vchdemod",    CmdVchDemod,        AlwaysAvailable, "['clone'] -- Demodulate samples for VeriChip"},
//    {"flexdemod",   CmdFlexdemod,       AlwaysAvailable, "Demodulate samples for Motorola FlexPass"},
    {NULL, NULL, NULL, NULL}
};

int CmdLF(const char *Cmd) {
    clearCommandBuffer();
    return CmdsParse(CommandTable, Cmd);
}

int CmdHelp(const char *Cmd) {
    (void)Cmd; // Cmd is not used so far
    CmdsHelp(CommandTable);
    return PM3_SUCCESS;
}