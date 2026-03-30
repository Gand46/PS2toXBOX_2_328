/*
 * PS2 to OG Xbox adapter firmware for ATmega328P + V-USB.
 *
 * This implementation keeps compatibility with the XID flow used by the
 * original Xbox and improves robustness vs. the legacy single-file approach:
 * - bounded waits during enumeration
 * - clearer request handling
 * - consistent report initialization
 * - non-blocking main loop with watchdog resets
 */

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include <string.h>

#include "usbdrv.h"
#include "oddebug.h"
#include "descriptors.h"
#include "ps2.h"

#define XID_REPORT_SIZE           0x14u
#define XID_OUT_REPORT_SIZE       0x06u
#define ENUM_DELAY_TICKS          250u
#define USB_REENUM_DELAY_MS       255u

#define USBREQ_TYPE_VENDOR_IN (USBRQ_RCPT_INTERFACE | USBRQ_TYPE_VENDOR | USBRQ_DIR_DEVICE_TO_HOST)
#define USBREQ_TYPE_CLASS_IN  (USBRQ_TYPE_CLASS | USBRQ_DIR_DEVICE_TO_HOST | USBRQ_RCPT_INTERFACE)
#define USBREQ_TYPE_CLASS_OUT (USBRQ_TYPE_CLASS | USBRQ_DIR_HOST_TO_DEVICE | USBRQ_RCPT_INTERFACE)

#define XID_DESCRIPTOR_TYPE       0x42u
#define XID_REPORT_IN_SELECTOR    USBDESCR_DEVICE
#define XID_REPORT_OUT_SELECTOR   USBDESCR_CONFIG

static volatile bool enum_window_elapsed = false;

uchar actuator[2] = {0x00, 0x00};
USB_JoystickReport_Data_t gamepad_state;
XIDGamepadOutputReport out_XID_report;

static void setup_10ms_timer(void);
static void fake_disconnect(void);
static void init_xid_structs(void);
static void wait_xbox_enumeration(void);

usbMsgLen_t usbFunctionSetup(uchar data[8]) {
    usbRequest_t *rq = (void *)data;

    if (rq->bmRequestType == USBREQ_TYPE_VENDOR_IN) {
        if ((rq->bRequest == USBRQ_GET_DESCRIPTOR) && (rq->wValue.bytes[1] == XID_DESCRIPTOR_TYPE)) {
            usbMsgPtr = (usbMsgPtr_t)usbDescriptorHidReport;
            usbMsgFlags = USB_FLG_MSGPTR_IS_ROM;
            return 16;
        }

        if (rq->bRequest == USBRQ_HID_GET_REPORT) {
            if (rq->wValue.bytes[1] == XID_REPORT_IN_SELECTOR) {
                usbMsgPtr = (usbMsgPtr_t)&gamepad_state;
                pad = 1;
                return sizeof(gamepad_state);
            }

            if (rq->wValue.bytes[1] == XID_REPORT_OUT_SELECTOR) {
                usbMsgPtr = (usbMsgPtr_t)&out_XID_report;
                return sizeof(out_XID_report);
            }
        }
    }

    if (rq->bmRequestType == USBREQ_TYPE_CLASS_IN) {
        if (rq->bRequest == USBRQ_HID_GET_REPORT) {
            usbMsgPtr = (usbMsgPtr_t)&gamepad_state;
            TCCR0B |= (1 << CS02) | (1 << CS00); /* start 10ms timer, clk/1024 */
            return sizeof(gamepad_state);
        }
    }

    if (rq->bmRequestType == USBREQ_TYPE_CLASS_OUT) {
        if ((rq->bRequest == USBRQ_HID_SET_REPORT) && (rq->wValue.bytes[1] == XID_REPORT_OUT_SELECTOR)) {
            return USB_NO_MSG; /* payload handled in usbFunctionWrite */
        }
    }

    return 0;
}

USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len) {
    (void)len;
    actuator[0] = data[3]; /* left motor */
    actuator[1] = data[5]; /* right motor */
    return 1;
}

static void init_xid_structs(void) {
    memset(&gamepad_state, 0x00, sizeof(gamepad_state));
    gamepad_state.rid = 0x00;
    gamepad_state.rsize = XID_REPORT_SIZE;

    memset(&out_XID_report, 0x00, sizeof(out_XID_report));
    out_XID_report.rid = 0x00;
    out_XID_report.rsize = XID_OUT_REPORT_SIZE;
}

static void setup_10ms_timer(void) {
    TCCR0A = (1 << WGM01);   /* CTC */
    TCCR0B = 0;              /* stopped */
    OCR0A = 155;             /* 10ms @ 16MHz / 1024 */
    TIMSK0 |= (1 << OCIE0A);
    TCNT0 = 0;
    TIFR0 = (1 << OCF0A);
}

ISR(TIMER0_COMPA_vect) {
    enum_window_elapsed = true;
    pad = 1;
    TCCR0B = 0;
    TIFR0 = (1 << OCF0A);
    TIMSK0 &= ~(1 << OCIE0A);
}

static void fake_disconnect(void) {
    for (uint16_t i = 0; i < USB_REENUM_DELAY_MS; ++i) {
        wdt_reset();
        _delay_ms(1);
    }
}

static void wait_xbox_enumeration(void) {
    uint16_t timeout = 0;
    while ((pad != 1) && (timeout < 2000u)) {
        usbPoll();
        wdt_reset();
        _delay_ms(1);
        ++timeout;
    }

    uint8_t cntr = 0;
    while ((cntr < ENUM_DELAY_TICKS) && !enum_window_elapsed) {
        wdt_reset();
        usbPoll();
        ++cntr;
    }
}

int main(void) {
    _delay_ms(5);

    odDebugInit();
    wdt_enable(WDTO_1S);

    init_xid_structs();
    setup_10ms_timer();
    spi_mInit();

    pad = 0;
    enum_window_elapsed = false;

    cli();
    usbInit();
    usbDeviceDisconnect();
    fake_disconnect();
    setup_actuator();
    usbDeviceConnect();
    sei();

    wait_xbox_enumeration();

    for (;;) {
        wdt_reset();
        usbPoll();

        if (!usbInterruptIsReady3()) {
            continue;
        }

        gamepad_state = translatePS2toXbox(getPS2ControllerInputData());
        usbSetInterrupt3((uchar *)&gamepad_state, sizeof(gamepad_state));
    }
}
