#include <cstring>

extern "C"{
#include <stm32f3.h>
#include <math.h>

#include "adc.h"
#include "commproto.h"
#include "hardware.h"
#include "i2c.h"
#include "mlxproc.h"
#include "strfunc.h"
#include "usart.h"
#include "usb_dev.h"
}

// image aquisition time
const char* const Timage = "TIMAGE";

// Global senders to send info into other interface
static int (*usb_sender)(const char*) = nullptr;
static int (*usart_sender)(const char*) = nullptr;
static int (*usb_putb)(uint8_t) = nullptr;
static int (*usart_putb)(uint8_t) = nullptr;
static int (*usb_sendbin)(const uint8_t*, int) = nullptr;
static int (*usart_sendbin)(const uint8_t*, int) = nullptr;

// Function helpers
static int (*SEND)(const char*) = nullptr;
static int (*putb)(uint8_t) = nullptr;
static int (*sendbin)(const uint8_t*, int) = nullptr;

#define N()             putb('\n')
#define printu(x)       SEND(u2str(x))
#define printi(x)       SEND(i2str(x))
#define printuhex(x)    SEND(uhex2str(x))
#define printfl(x,n)    SEND(float2str(x, n))

void set_senders(int (*usbs)(const char *),
                 int (*usbb)(uint8_t),
                 int (*usbbin)(const uint8_t *, int),
                 int (*usarts)(const char *),
                 int (*usartb)(uint8_t),
                 int (*usartbin)(const uint8_t *, int)){
    usb_sender = usbs;
    usb_putb = usbb;
    usb_sendbin = usbbin;
    usart_sender = usarts;
    usart_putb = usartb;
    usart_sendbin = usartbin;
}

// Local buffer for I2C data
#define LOCBUFFSZ       32
static uint16_t locBuffer[LOCBUFFSZ];
// default I2C address of sensor
static uint8_t I2Caddress = 0x33 << 1;
extern volatile uint32_t Tms;
// show `cartoon` - continuously draw ASCII image of current sensor
uint8_t cartoon = 0;

// Command list
#define COMMAND_TABLE \
    COMMAND(help,       "show this help") \
    COMMAND(ascii,      "draw nth image in ASCII (n=0..4)") \
    COMMAND(binary,     "get nth image as text array of floats") \
    COMMAND(listids,    "list active sensors IDs") \
    COMMAND(tempmap,    "show temperature map of nth image") \
    COMMAND(acqtime,    "show nth image aquisition time") \
    COMMAND(bmereinit,  "reinit BME280") \
    COMMAND(environ,    "get environment parameters") \
    COMMAND(state,      "get MLX state") \
    COMMAND(reset,      "reset MCU") \
    COMMAND(time,       "print current Tms") \
    COMMAND(iicaddr,    "get/set I2C address (non-shifted)") \
    COMMAND(mlxcont,    "continue MLX") \
    COMMAND(iicspeed,   "get/set I2C speed (0..4)") \
    COMMAND(mlxpause,   "pause MLX") \
    COMMAND(mlxstop,    "stop MLX") \
    COMMAND(adc,        "get n'th ADC values") \
    COMMAND(ntc,        "get n'th NTC temperatures") \
    COMMAND(cartoon,    "toggle cartoon mode") \
    COMMAND(mlxdump,    "dump MLX parameters for sensor n") \
    COMMAND(mlxaddr,    "get/set MLX address of sensor n") \
    COMMAND(readreg,    "read I2C register: readreg reg [= nwords]") \
    COMMAND(writedata,  "write I2C data: writedata = val1 val2 ...") \
    COMMAND(iicscan,    "scan I2C bus") \
    COMMAND(mcutemp,    "get MCU temperature") \
    COMMAND(mcuvdd,     "get MCU Vdd") \
    COMMAND(dac,        "get/set DAC value") \
    COMMAND(pwm,        "get/set PWM for channel n (0..100%)") \
    COMMAND(sendstr,    "send string to other interface: sendstr = text")


// Command prototypes
#define COMMAND(name, desc)   static errcodes_t cmd_ ## name(const char*, char*);
COMMAND_TABLE
#undef COMMAND

// descrtiptions for `help`
typedef struct {
    const char *name;
    const char *desc;
} CmdInfo;

static const CmdInfo cmdInfo[] = {
#define COMMAND(name, desc) { #name, desc },
    COMMAND_TABLE
#undef COMMAND
};

// Text descriptions for error codes
static const char* errtxt[ERR_AMOUNT] = {
    [ERR_OK]        = "OK\n",
    [ERR_BADCMD]    = "BADCMD\n",
    [ERR_BADPAR]    = "BADPAR\n",
    [ERR_BADVAL]    = "BADVAL\n",
    [ERR_WRONGLEN]  = "WRONGLEN\n",
    [ERR_CANTRUN]   = "CANTRUN\n",
    [ERR_BUSY]      = "BUSY\n",
    [ERR_OVERFLOW]  = "OVERFLOW\n",
};

const char *EQ = " = "; // equal sign for getters

// send `command = `
#define CMDEQ()   do{SEND(cmd); SEND(EQ);}while(0)
// send `commandXXX = `
#define CMDEQP(x)   do{SEND(cmd); SEND(u2str((uint32_t)x)); SEND(EQ);}while(0)

/**
 * @brief splitargs - get command parameter and setter from `args`
 * @param args (i) - rest of string after command (like `1 = PU OD OUT`)
 * @param parno (o) - parameter number or -1 if none
 * @return setter (part after `=` without leading spaces) or NULL if none
 */
static const char *splitargs(char *args, int32_t *parno){
    if(!args) return NULL;
    uint32_t U32;
    const char *next = getnum(args, &U32);
    int p = -1;
    if(next != args && U32 <= MAXPARNO) p = U32;
    if(parno) *parno = p;
    next = strchr(next, '=');
    if(next){
        if(*(++next)) next = omit_spaces(next);
        if(*next == 0) next = NULL;
    }
    return next;
}

/**
 * @brief argsvals - split `args` into `parno` and setter's value
 * @param args - rest of string after command
 * @param parno (o) - parameter number or -1 if none
 * @param parval - integer setter's value
 * @return false if no setter or it's not a number, true - got setter's num
 */
static bool argsvals(char *args, int32_t *parno, int32_t *parval){
    const char *setter = splitargs(args, parno);
    if(!setter) return false;
    int32_t I32;
    const char *next = getint(setter, &I32);
    if(next != setter && parval){
        *parval = I32;
        return true;
    }
    return false;
}


/************* List of proto functions for each command *************/

static errcodes_t cmd_help(const char*, char*){
    SEND(REPOURL);
    for(size_t i = 0; i < sizeof(cmdInfo)/sizeof(cmdInfo[0]); ++i){
        SEND(cmdInfo[i].name);
        SEND(" - ");
        SEND(cmdInfo[i].desc);
        SEND("\n");
    }
    return ERR_AMOUNT;
}

static errcodes_t cmd_time(const char* cmd, char*){
    CMDEQ();
    printu(Tms); N();
    return ERR_AMOUNT;
}

static errcodes_t cmd_reset(const char*, char*){
    NVIC_SystemReset();
    return ERR_CANTRUN; // unreacheable
}

// send acquisition time
static void imaqtime(uint8_t sensno){
    uint32_t T = mlx_lastimT(sensno);
    SEND(Timage); printu(sensno); SEND(EQ);
    printu(T); N();
}

// Common image command for ASCII/binary/tempmap
static errcodes_t image_cmd(char* args, int mode){
    int32_t sensno = -1;
    splitargs(args, &sensno);
    if(sensno < 0 || sensno >= N_SENSORS) return ERR_BADPAR;
    fp_t *img = mlx_getimage(sensno);
    if(!img) return ERR_CANTRUN;

    // Frame number
    imaqtime(sensno);

    switch(mode){
        case 0: // tempmap
            dumpIma(img);
            break;
        case 1: // ascii
            drawIma(img);
            break;
        case 2: // binary
            SEND("BINARY"); putb('0'+sensno); putb('=');
            uint8_t *d = (uint8_t*)img;
            uint32_t _2send = MLX_PIXNO * sizeof(float);
            // send by portions of 256 bytes
            while(_2send){
                uint32_t portion = (_2send > 256) ? 256 : _2send;
                if(sendbin(d, portion)){
                    _2send -= portion;
                    d += portion;
                }
            }
            SEND("ENDIMAGE"); N();
            break;
    }
    return ERR_AMOUNT;
}

static errcodes_t cmd_ascii(const char* , char* args){
    return image_cmd(args, 1);
}
static errcodes_t cmd_binary(const char* , char* args){
    return image_cmd(args, 2);
}
static errcodes_t cmd_tempmap(const char* , char* args){
    return image_cmd(args, 0);
}

static errcodes_t cmd_acqtime(const char* , char* args){
    int32_t sensno = -1;
    splitargs(args, &sensno);
    if(sensno < 0 || sensno >= N_SENSORS) return ERR_BADPAR;
    imaqtime(sensno);
    return ERR_AMOUNT;
}

static errcodes_t cmd_listids(const char*, char*){
    int N = mlx_nactive();
    if(!N) return ERR_CANTRUN;
    uint8_t *ids = mlx_activeids();
    SEND("Found "); printu(N); SEND(" active sensors:\n");
    for(int i = 0; i < N_SENSORS; ++i){
        if(ids[i]){
            SEND("SENSID"); printu(i); SEND(EQ); printuhex(ids[i]>>1); N();
        }
    }
    return ERR_AMOUNT;
}

static errcodes_t cmd_bmereinit(const char*, char*){
    if(bme_init()) return ERR_OK;
    return ERR_CANTRUN;
}

static errcodes_t cmd_environ(const char*, char*){
    bme280_t env;
    if(!get_environment(&env)) return ERR_CANTRUN;
    SEND("TEMPERATURE="); printfl(env.T, 2); N();
    SEND("SKYTEMPERATURE="); printfl(env.Tsky, 2); N();
    SEND("PRESSURE_HPA="); printfl(env.P/100.f, 2); N();
    SEND("PRESSURE_MM="); printfl(env.P * 0.00750062f, 2); N();
    SEND("HUMIDITY="); printfl(env.H, 2); N();
    SEND("TEMP_DEW="); printfl(env.Tdew, 1); N();
    SEND("T_MEASUREMENT="); printu(env.Tmeas); N();
    return ERR_OK;
}

static errcodes_t cmd_state(const char* cmd, char*){
    static const char *states[] = {
        [MLX_NOTINIT] = "not init",
        [MLX_WAITPARAMS] = "wait parameters DMA read",
        [MLX_WAITSUBPAGE] = "wait subpage",
        [MLX_READSUBPAGE] = "wait subpage DMA read",
        [MLX_RELAX] = "do nothing"
    };
    mlx_state_t s = mlx_state();
    CMDEQ(); SEND(states[s]); N();
    return ERR_AMOUNT;
}

/********** I2C commands **********/

static errcodes_t cmd_iicaddr(const char* cmd, char* args){
    int32_t addr;
    if(argsvals(args, NULL, &addr)){
        if(addr < 0 || addr > 0x7f) return ERR_BADVAL;
        I2Caddress = (uint8_t)(addr << 1);
        mlx_sethwaddr(I2Caddress, addr);
        return ERR_AMOUNT;
    }
    // getter
    CMDEQ(); printuhex(I2Caddress >> 1); N();
    return ERR_AMOUNT;
}

static errcodes_t cmd_mlxcont(const char*, char*){
    mlx_continue();
    return ERR_OK;
}

static errcodes_t cmd_iicspeed(const char* cmd, char* args){
    static const char *speeds[] = {"10K","100K","400K","1M","2M"};
    int32_t speed;
    // TODO: allow string parameter
    if(argsvals(args, NULL, &speed)){
        if (speed < 0 || speed >= I2C_SPEED_AMOUNT) return ERR_BADVAL;
        i2c_setup((i2c_speed_t)speed);
    }
    // getter
    CMDEQ(); SEND(speeds[i2c_curspeed]); N();
    return ERR_AMOUNT;
}

static errcodes_t cmd_mlxpause(const char*, char*){
    mlx_pause();
    return ERR_OK;
}

static errcodes_t cmd_mlxstop(const char*, char*){
    mlx_stop();
    return ERR_OK;
}

static errcodes_t cmd_adc(const char* cmd, char* args){
    if(!args){ // show all values
        for(uint8_t i = 0; i < NUMBER_OF_ADC_CHANNELS; ++i){
            CMDEQP(i); printu(getADCval(i)); N();
        }
        return ERR_AMOUNT;
    }
    int32_t addr;
    splitargs(args, &addr);
    if(addr < 0 || addr >= NUMBER_OF_ADC_CHANNELS) return ERR_BADPAR;
    CMDEQP(addr); printu(getADCval(static_cast<uint8_t>(addr))); N();
    return ERR_AMOUNT;
}

static errcodes_t cmd_ntc(const char* cmd, char* args){
    if(!args){ // show all values
        for(uint8_t i = 0; i <= ADC_AIN4; ++i){
            CMDEQP(i); printfl(getNTCtemp(i), 1); N();
        }
        return ERR_AMOUNT;
    }
    int32_t addr;
    splitargs(args, &addr);
    if(addr < 0 || addr > ADC_AIN4) return ERR_BADPAR;
    CMDEQP(addr); printfl(getNTCtemp(static_cast<uint8_t>(addr)), 1); N();
    return ERR_AMOUNT;
}

static errcodes_t cmd_cartoon(const char*, char*){
    // TODO: should be getter/setter!
    cartoon = !cartoon;
    return ERR_OK;
}

static errcodes_t cmd_mlxdump(const char*, char* args){
    int32_t sensno = -1;
    splitargs(args, &sensno);
    if (sensno < 0 || sensno >= N_SENSORS) return ERR_BADPAR;
    MLX90640_params *params = mlx_getparams(sensno);
    if(!params) return ERR_CANTRUN;

    SEND("SENSNO="); printi(sensno); N();
    SEND("kVdd="); printi(params->kVdd); N();
    SEND("vdd25="); printi(params->vdd25); N();
    SEND("KvPTAT="); printfl(params->KvPTAT, 4); N();
    SEND("KtPTAT="); printfl(params->KtPTAT, 4); N();
    SEND("vPTAT25="); printi(params->vPTAT25); N();
    SEND("alphaPTAT="); printfl(params->alphaPTAT, 2); N();
    SEND("gainEE="); printi(params->gainEE); N();
    SEND("Pixel offset parameters:\n");
    dumpIma(params->offset);
    SEND("K_talpha:\n");
    dumpIma(params->kta);
    SEND("Kv: ");
    for(int i = 0; i < 4; ++i) { printfl(params->kv[i], 2); putb(' '); }
    N();
    SEND("cpOffset="); printi(params->cpOffset[0]); SEND(", "); printi(params->cpOffset[1]); N();
    SEND("cpKta="); printfl(params->cpKta, 2); N();
    SEND("cpKv="); printfl(params->cpKv, 2); N();
    SEND("tgc="); printfl(params->tgc, 2); N();
    SEND("cpALpha="); printfl(params->cpAlpha[0], 2); SEND(", "); printfl(params->cpAlpha[1], 2); N();
    SEND("KsTa="); printfl(params->KsTa, 2); N();
    SEND("Alpha:\n");
    dumpIma(params->alpha);
    SEND("CT3="); printfl(params->CT[1], 2); N();
    SEND("CT4="); printfl(params->CT[2], 2); N();
    for(int i = 0; i < 4; ++i){
        SEND("KsTo"); putb('0'+i); putb('='); printfl(params->KsTo[i], 2); N();
        SEND("alphacorr"); putb('0'+i); putb('='); printfl(params->alphacorr[i], 2); N();
    }
    return ERR_AMOUNT;
}

static errcodes_t cmd_mlxaddr(const char* cmd, char* args){
    int32_t sensno = -1;
    if(!args || !*args) { // without args: show global address
        //CMDEQ(); printuhex(I2Caddress>>1); N();
        //return ERR_AMOUNT;
        return ERR_BADPAR;
    }
    const char *setter = splitargs(args, &sensno);
    if(sensno < 0 || sensno >= N_SENSORS) return ERR_BADPAR;

    if(setter){ // setter: set current address
        uint32_t a;
        const char *nxt = getnum(setter, &a);
        if(nxt == setter || a > 0x7f) return ERR_BADVAL;
        mlx_setaddr(sensno, (uint8_t)a);
        return ERR_AMOUNT;
    }else{ // getter
        uint8_t a = mlx_getaddr(sensno);
        CMDEQP(sensno); printuhex(a); N();
        return ERR_AMOUNT;
    }
}

static errcodes_t cmd_readreg(const char* cmd, char* args){
    int32_t reg = -1, nwords = 1;
    const char *setter = splitargs(args, &reg);
    if(reg < 0) return ERR_BADPAR;
    if(setter){ // read more than one byte
        uint32_t n;
        const char *nxt = getnum(setter, &n);
        if (nxt == setter || n == 0 || n > I2C_BUFSIZE) return ERR_BADVAL;
        nwords = (int32_t)n;
    }
    uint16_t *data = i2c_read_reg16(I2Caddress, (uint16_t)reg, nwords, 0);
    if(!data) return ERR_CANTRUN;
    if(nwords == 1){
        CMDEQP(reg); printuhex(*data); N();
    }else{
        CMDEQP(reg); N(); hexdump16(SEND, data, nwords);
    }
    return ERR_AMOUNT;
}

static errcodes_t cmd_writedata(const char*, char* args){
    const char *setter = splitargs(args, NULL);
    if(!setter) return ERR_BADVAL;
    int N = 0;
    const char *p = setter;
    while (*p){
        if(N >= LOCBUFFSZ) return ERR_AMOUNT;
        uint32_t val;
        p = getnum(p, &val);
        if(p == setter) break; // not a number
        locBuffer[N++] = (uint16_t)val;
        p = omit_spaces(p);
        if (!*p) break;
    }
    if(N == 0) return ERR_BADVAL;
    if(!i2c_write(I2Caddress, locBuffer, N)) return ERR_CANTRUN;
    return ERR_OK;
}

static errcodes_t cmd_iicscan(const char*, char*) {
    i2c_init_scan_mode();
    return ERR_OK;
}

static errcodes_t cmd_mcutemp(const char* cmd, char*){
    CMDEQ(); printfl(getMCUtemp(), 2); N();
    return ERR_AMOUNT;
}

static errcodes_t cmd_mcuvdd(const char* cmd, char*){
    CMDEQ(); printfl(getVdd(), 2); N();
    return ERR_AMOUNT;
}

static errcodes_t cmd_dac(const char* cmd, char* args){
    int32_t val;
    if(argsvals(args, NULL, &val)){
        if(val < 0 || val > 4095) return ERR_BADVAL;
        DAC1->DHR12R1 = static_cast<uint32_t>(val);
    }
    // getter
    CMDEQ(); printu(DAC1->DHR12R1); N();
    return ERR_AMOUNT;
}

static void showpwm(const char* cmd, uint8_t nch){
    uint16_t ccr;
    switch(nch){
    case 0: ccr = TIM3->CCR1; break;
    case 1: ccr = TIM3->CCR2; break;
    case 2: ccr = TIM3->CCR3; break;
    case 3: ccr = TIM3->CCR4; break;
    default: return;
    }
    CMDEQP(nch); printu(ccr); N();
}

// four PWM channels: 1,2 - heaters, 3,4 - info
static errcodes_t cmd_pwm(const char* cmd, char* args){
    int32_t ch = -1, val;
    const char *setter = splitargs(args, &ch);
    if(ch < 0 || ch > PWM_CH_MAX){ // all channels
        for(uint8_t i = 0; i <= PWM_CH_MAX; ++i)
            showpwm(cmd, i);
        return ERR_AMOUNT;
    }
    if(setter){
        if(!getint(setter, &val) || val < 0 || val > 100) return ERR_BADVAL;
        if(!setPWM(static_cast<uint8_t>(ch), static_cast<uint32_t>(val)))
            return ERR_CANTRUN;
    }
    // getter
    showpwm(cmd, (uint8_t)ch);
    return ERR_AMOUNT;
}

static errcodes_t cmd_sendstr(const char*, char* args) {
    int32_t dummy;
    const char *text = splitargs(args, &dummy);
    if(!text || !*text) return ERR_BADVAL;
    // switch to other interface
    int (*other_sender)(const char*) = (SEND == usb_sender) ? usart_sender : usb_sender;
    if (!other_sender) return ERR_CANTRUN;
    other_sender(text);
    other_sender("\n");
    return ERR_OK;
}

static constexpr uint32_t hash(const char* str, uint32_t h = 0) {
    return *str ? hash(str + 1, h + ((h << 7) ^ *str)) : h;
}


const char *parse_cmd(int (*sendfun)(const char*), char *buf) {
    if(!buf || !*buf || !sendfun) return NULL;
    SEND = sendfun;
    if(sendfun == usb_sender){
        putb = usb_putb;
        sendbin = usb_sendbin;
    }else{
        putb = usart_putb;
        sendbin = usart_sendbin;
    }
    char command[CMD_MAXLEN+1];
    int i = 0;
    while(*buf > '@' && i < CMD_MAXLEN) command[i++] = *buf++;
    command[i] = 0;
    while(*buf && *buf <= ' ') ++buf;
    char *args = buf;
#ifdef EBUG
    USB_sendstr("__args='"); USB_sendstr(args); USB_sendstr("'\n");
#endif
    if(!*args) args = NULL;

    uint32_t h = hash(command);
    errcodes_t ecode = ERR_AMOUNT;
    switch (h){
#define COMMAND(name, desc) case hash(#name): ecode = cmd_##name(command, args); break;
        COMMAND_TABLE
#undef COMMAND
        default:
            SEND("Unknown command, try 'help'\n");
    }
    if(ecode < ERR_AMOUNT) return errtxt[ecode];
    return NULL;
}

void dumpIma(const fp_t im[MLX_PIXNO]){
    for(int row = 0; row < MLX_H; ++row){
        for(int col = 0; col < MLX_W; ++col){
            printfl(*im++, 1);
            putb(' ');
        }
        N();
    }
}

#define GRAY_LEVELS     16
static const char *const CHARS_16 = " .':;+*oxX#&%B$@";
void drawIma(const fp_t im[MLX_PIXNO]){
    fp_t min_val = im[0], max_val = im[0];
    const fp_t *iptr = im;
    for(int row = 0; row < MLX_H; ++row)
        for(int col = 0; col < MLX_W; ++col){
            fp_t cur = *iptr++;
            if (cur < min_val) min_val = cur;
            else if (cur > max_val) max_val = cur;
        }
    fp_t range = max_val - min_val;
    SEND("RANGE="); printfl(range, 3); N();
    SEND("MIN="); printfl(min_val, 3); N();
    SEND("MAX="); printfl(max_val, 3); N();
    if(fabsf(range) < 0.001) range = 1.0f;
    iptr = im;
    char string[MLX_W+2];
    string[MLX_W] = '\n'; string[MLX_W+1] = 0; // end of line
    for(int row = 0; row < MLX_H; ++row){
        for(int col = 0; col < MLX_W; ++col){
            fp_t normalized = ((*iptr++) - min_val) / range;
            int idx = (int)(normalized * GRAY_LEVELS);
            if(idx < 0) idx = 0;
            else if(idx >= GRAY_LEVELS) idx = GRAY_LEVELS-1;
            string[col] = CHARS_16[idx];
        }
        SEND(string);
    }
    N();
}
