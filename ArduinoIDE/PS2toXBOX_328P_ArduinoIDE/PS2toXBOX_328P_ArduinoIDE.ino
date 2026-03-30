/*
 * PS2toXBOX_328P_ArduinoIDE.ino
 *
 * Firmware PS2 -> Original Xbox (XID) para ATmega328P 5V/16MHz con V-USB.
 *
 * Dependencias:
 * - Este sketch incluye el stack V-USB y los módulos C en carpetas locales.
 * - Compilar en Arduino IDE con placa basada en ATmega328P @16MHz.
 */

#include <Arduino.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <string.h>

extern "C" {
#include "usbdrv.h"
#include "oddebug.h"
#include "descriptors.h"
#include "ps2.h"
}

#define XID_REPORT_SIZE      0x14u
#define XID_OUT_REPORT_SIZE  0x06u
#define ENUM_DELAY_TICKS     250u

#define USBREQ_TYPE_VENDOR_IN (USBRQ_RCPT_INTERFACE | USBRQ_TYPE_VENDOR | USBRQ_DIR_DEVICE_TO_HOST)
#define USBREQ_TYPE_CLASS_IN  (USBRQ_TYPE_CLASS | USBRQ_DIR_DEVICE_TO_HOST | USBRQ_RCPT_INTERFACE)
#define USBREQ_TYPE_CLASS_OUT (USBRQ_TYPE_CLASS | USBRQ_DIR_HOST_TO_DEVICE | USBRQ_RCPT_INTERFACE)

#define XID_DESCRIPTOR_TYPE   0x42u
#define XID_REPORT_IN_SELECTOR USBDESCR_DEVICE
#define XID_REPORT_OUT_SELECTOR USBDESCR_CONFIG

volatile bool enumWindowElapsed = false;

uchar actuator[2] = {0x00, 0x00};
USB_JoystickReport_Data_t gamepad_state;
XIDGamepadOutputReport out_XID_report;

static void setup10msTimer() {
  TCCR0A = (1 << WGM01);
  TCCR0B = 0;
  OCR0A = 155;
  TIMSK0 |= (1 << OCIE0A);
  TCNT0 = 0;
  TIFR0 = (1 << OCF0A);
}

ISR(TIMER0_COMPA_vect) {
  enumWindowElapsed = true;
  pad = 1;
  TCCR0B = 0;
  TIFR0 = (1 << OCF0A);
  TIMSK0 &= ~(1 << OCIE0A);
}

static void initReports() {
  memset(&gamepad_state, 0x00, sizeof(gamepad_state));
  gamepad_state.rid = 0x00;
  gamepad_state.rsize = XID_REPORT_SIZE;

  memset(&out_XID_report, 0x00, sizeof(out_XID_report));
  out_XID_report.rid = 0x00;
  out_XID_report.rsize = XID_OUT_REPORT_SIZE;
}

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

  if (rq->bmRequestType == USBREQ_TYPE_CLASS_IN && rq->bRequest == USBRQ_HID_GET_REPORT) {
    usbMsgPtr = (usbMsgPtr_t)&gamepad_state;
    TCCR0B |= (1 << CS02) | (1 << CS00);
    return sizeof(gamepad_state);
  }

  if (rq->bmRequestType == USBREQ_TYPE_CLASS_OUT && rq->bRequest == USBRQ_HID_SET_REPORT && rq->wValue.bytes[1] == XID_REPORT_OUT_SELECTOR) {
    return USB_NO_MSG;
  }

  return 0;
}

uchar usbFunctionWrite(uchar *data, uchar len) {
  (void)len;
  actuator[0] = data[3];
  actuator[1] = data[5];
  return 1;
}

static void fakeDisconnect() {
  for (uint16_t i = 0; i < 255; ++i) {
    wdt_reset();
    delay(1);
  }
}

void setup() {
  odDebugInit();
  wdt_enable(WDTO_1S);
  initReports();
  setup10msTimer();
  spi_mInit();

  pad = 0;
  enumWindowElapsed = false;

  cli();
  usbInit();
  usbDeviceDisconnect();
  fakeDisconnect();
  setup_actuator();
  usbDeviceConnect();
  sei();

  uint16_t timeout = 0;
  while ((pad != 1) && (timeout < 2000)) {
    usbPoll();
    wdt_reset();
    delay(1);
    timeout++;
  }

  uint8_t cntr = 0;
  while ((cntr < ENUM_DELAY_TICKS) && !enumWindowElapsed) {
    usbPoll();
    wdt_reset();
    cntr++;
  }
}

void loop() {
  wdt_reset();
  usbPoll();

  if (!usbInterruptIsReady3()) {
    return;
  }

  gamepad_state = translatePS2toXbox(getPS2ControllerInputData());
  usbSetInterrupt3((uchar *)&gamepad_state, sizeof(gamepad_state));
}
