#include <util/delay.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <string.h>

#include "ps2.h"
#include "gamePadDefinitions.h"

#define PS2_MODE_DIGITAL  0x41u
#define PS2_MODE_ANALOG   0x73u

#define PS2_CMD_POLL      0x42u
#define PS2_CMD_CONFIG    0x43u
#define PS2_CMD_ANALOG    0x44u
#define PS2_CMD_MOTOR_MAP 0x4Du

#define PS2_ATT_LOW()     (PORTB &= ~(1 << PB2))
#define PS2_ATT_HIGH()    (PORTB |=  (1 << PB2))

static uchar ps2buffer[9] = {0};

static inline uint8_t bit_to_analog(uint8_t value, uint8_t mask) {
    return (value & mask) ? 0xFF : 0x00;
}

static USB_JoystickReport_Data_t neutral_report(void) {
    USB_JoystickReport_Data_t report;
    memset(&report, 0, sizeof(report));
    report.rsize = 0x14;
    return report;
}

void spi_mInit(void) {
    /* SPI master, LSB first, CPOL=1, CPHA=1, f = 16MHz / 32 = 500kHz */
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << DORD) | (1 << CPHA) | (1 << CPOL);
    SPSR = (1 << SPI2X);

    DDRB |= (1 << PB2) | (1 << PB3) | (1 << PB5);
    DDRB &= ~(1 << PB4);
    PS2_ATT_HIGH();
}

unsigned char spi_mSend(uchar data_com) {
    SPDR = data_com;
    while (!(SPSR & (1 << SPIF))) {
    }
    return SPDR;
}

void wait100us(void) {
    _delay_us(25);
}

uchar sendCommandToPS2(uchar *data, uchar len) {
    uchar response = 0x00;
    PS2_ATT_LOW();
    _delay_us(50);

    for (uchar i = 0; i < len; ++i) {
        uchar rx = spi_mSend(data[i]);
        if (i == 1) {
            response = rx;
        }
        wait100us();
    }

    _delay_us(50);
    PS2_ATT_HIGH();
    wdt_reset();
    return response;
}

uchar *getPS2ControllerInputData(void) {
    wdt_reset();

    memset(ps2buffer, 0x00, sizeof(ps2buffer));

    PS2_ATT_LOW();
    _delay_us(50);

    ps2buffer[0] = spi_mSend(0x01);
    wait100us();
    ps2buffer[1] = spi_mSend(PS2_CMD_POLL);
    wait100us();

    if ((ps2buffer[1] != PS2_MODE_DIGITAL) && (ps2buffer[1] != PS2_MODE_ANALOG)) {
        PS2_ATT_HIGH();
        return ps2buffer;
    }

    ps2buffer[2] = spi_mSend(0x00);
    wait100us();

    if (ps2buffer[1] == PS2_MODE_ANALOG) {
        ps2buffer[3] = spi_mSend(actuator[1]);
        wait100us();
        ps2buffer[4] = spi_mSend(actuator[0]);
    } else {
        ps2buffer[3] = spi_mSend(0x00);
        wait100us();
        ps2buffer[4] = spi_mSend(0x00);
    }

    wait100us();
    ps2buffer[5] = spi_mSend(0x00);
    wait100us();
    ps2buffer[6] = spi_mSend(0x00);
    wait100us();
    ps2buffer[7] = spi_mSend(0x00);
    wait100us();
    ps2buffer[8] = spi_mSend(0x00);

    _delay_us(50);
    PS2_ATT_HIGH();
    wdt_reset();
    return ps2buffer;
}

USB_JoystickReport_Data_t translatePS2toXbox(uchar *in) {
    if ((in[1] != PS2_MODE_DIGITAL) && (in[1] != PS2_MODE_ANALOG)) {
        return neutral_report();
    }

    USB_JoystickReport_Data_t report = neutral_report();

    uint8_t temp1 = (uint8_t)(255 - in[3]);
    uint8_t temp2 = (uint8_t)(255 - in[4]);

    report.digital_buttons =
        ((temp1 & PS2_DPAD_UP) >> 4) |
        ((temp1 & PS2_DPAD_DOWN) >> 5) |
        ((temp1 & PS2_DPAD_LEFT) >> 5) |
        ((temp1 & PS2_DPAD_RIGHT) >> 2) |
        ((temp1 & PS2_START) << 1) |
        ((temp1 & PS2_SELECT) << 5) |
        ((temp1 & PS2_L3) << 5) |
        ((temp1 & PS2_R3) << 5);

    report.a = bit_to_analog(temp2, PS2_X);
    report.b = bit_to_analog(temp2, PS2_O);
    report.x = bit_to_analog(temp2, PS2_S);
    report.y = bit_to_analog(temp2, PS2_T);
    report.black = bit_to_analog(temp2, PS2_L2);
    report.white = bit_to_analog(temp2, PS2_R2);
    report.l = bit_to_analog(temp2, PS2_L1);
    report.r = bit_to_analog(temp2, PS2_R1);

    if (in[1] == PS2_MODE_ANALOG) {
        report.r_x = in[5] - 0x80;
        report.r_y = ~(in[6] - 0x80);
        report.l_x = in[7] - 0x80;
        report.l_y = ~(in[8] - 0x80);
    }

    return report;
}

void setup_actuator(void) {
    uchar mode = 0x00;

    for (uint8_t attempts = 0; attempts < 8; ++attempts) {
        uchar probe[] = {0x01, PS2_CMD_POLL, 0x00, 0x00, 0x00};
        mode = sendCommandToPS2(probe, sizeof(probe));
        if ((mode == PS2_MODE_DIGITAL) || (mode == PS2_MODE_ANALOG)) {
            break;
        }
    }

    if ((mode != PS2_MODE_DIGITAL) && (mode != PS2_MODE_ANALOG)) {
        return;
    }

    uchar enter_cfg[] = {0x01, PS2_CMD_CONFIG, 0x00, 0x01, 0x00};
    uchar analog_on[] = {0x01, PS2_CMD_ANALOG, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00};
    uchar map_motor[] = {0x01, PS2_CMD_MOTOR_MAP, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF};
    uchar exit_cfg[] = {0x01, PS2_CMD_CONFIG, 0x00, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A};

    sendCommandToPS2(enter_cfg, sizeof(enter_cfg));
    sendCommandToPS2(analog_on, sizeof(analog_on));
    sendCommandToPS2(map_motor, sizeof(map_motor));
    sendCommandToPS2(exit_cfg, sizeof(exit_cfg));
}
