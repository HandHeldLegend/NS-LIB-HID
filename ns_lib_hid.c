/**
 * @file ns_lib_hid.c
 * @brief ROM USB/Bluetooth HID report descriptors and USB device descriptor for Pro-style gamepads.
 *
 * Copyright (c) 2026 Mitchell Cairns, Hand Held Legend, LLC.
 *
 * Licensed under the Creative Commons Attribution-NonCommercial 4.0 International License
 * (CC BY-NC 4.0). Non-commercial use with attribution; commercial licensing from Hand Held Legend.
 * Full terms: https://creativecommons.org/licenses/by-nc/4.0/legalcode — see LICENSING.md in this folder
 *
 * SPDX-License-Identifier: CC-BY-NC-4.0
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "ns_lib_hid.h"
#include "ns_lib_config.h"
#include "ns_lib_types.h"

static const char k_ns_dev_name[] = "Pro Controller";

static const ns_usb_device_descriptor_t k_ns_dev_usb_device_descriptor = {
    .bLength = NS_USB_DEVICE_DESCRIPTOR_LEN,
    .bDescriptorType = 1u, /* Device Descriptor */
    .bcdUSB = 0x0200, // Changed from 0x0200 to 2.1 0x0201 for BOS & WebUSB
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,

    .bMaxPacketSize0 = 64,
    .idVendor = 0x057Eu,  /* Nintendo VID */
    .idProduct = 0x2009u, /* Switch Pro Controller PID */

    .bcdDevice = 0x0210,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01
};

static const uint8_t k_ns_dev_hid_descriptor_usb[NS_HID_USB_REPORT_DESCRIPTOR_LEN] = {
    0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
    0x15, 0x00, // Logical Minimum (0)

    0x09, 0x04, // Usage (Joystick)
    0xA1, 0x01, // Collection (Application)

    0x85, 0x30, //   Report ID (48)
    0x05, 0x01, //   Usage Page (Generic Desktop Ctrls)
    0x05, 0x09, //   Usage Page (Button)
    0x19, 0x01, //   Usage Minimum (0x01)
    0x29, 0x0A, //   Usage Maximum (0x0A)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x01, //   Logical Maximum (1)
    0x75, 0x01, //   Report Size (1)
    0x95, 0x0A, //   Report Count (10)
    0x55, 0x00, //   Unit Exponent (0)
    0x65, 0x00, //   Unit (None)
    0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x09, //   Usage Page (Button)
    0x19, 0x0B, //   Usage Minimum (0x0B)
    0x29, 0x0E, //   Usage Maximum (0x0E)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x01, //   Logical Maximum (1)
    0x75, 0x01, //   Report Size (1)
    0x95, 0x04, //   Report Count (4)
    0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x01, //   Report Size (1)
    0x95, 0x02, //   Report Count (2)
    0x81, 0x03, //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x0B, 0x01, 0x00, 0x01, 0x00, //   Usage (0x010001)
    0xA1, 0x00,                   //   Collection (Physical)
    0x0B, 0x30, 0x00, 0x01, 0x00, //     Usage (0x010030)
    0x0B, 0x31, 0x00, 0x01, 0x00, //     Usage (0x010031)
    0x0B, 0x32, 0x00, 0x01, 0x00, //     Usage (0x010032)
    0x0B, 0x35, 0x00, 0x01, 0x00, //     Usage (0x010035)
    0x15, 0x00,                   //     Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00, //     Logical Maximum (65534)
    0x75, 0x10,                   //     Report Size (16)
    0x95, 0x04,                   //     Report Count (4)
    0x81, 0x02,                   //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,                         //   End Collection

    0x0B, 0x39, 0x00, 0x01, 0x00, //   Usage (0x010039)
    0x15, 0x00,                   //   Logical Minimum (0)
    0x25, 0x07,                   //   Logical Maximum (7)
    0x35, 0x00,                   //   Physical Minimum (0)
    0x46, 0x3B, 0x01,             //   Physical Maximum (315)
    0x65, 0x14,                   //   Unit (System: English Rotation, Length: Centimeter)
    0x75, 0x04,                   //   Report Size (4)
    0x95, 0x01,                   //   Report Count (1)
    0x81, 0x02,                   //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x09,                   //   Usage Page (Button)
    0x19, 0x0F,                   //   Usage Minimum (0x0F)
    0x29, 0x12,                   //   Usage Maximum (0x12)
    0x15, 0x00,                   //   Logical Minimum (0)
    0x25, 0x01,                   //   Logical Maximum (1)
    0x75, 0x01,                   //   Report Size (1)
    0x95, 0x04,                   //   Report Count (4)
    0x81, 0x02,                   //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x08,                   //   Report Size (8)
    0x95, 0x34,                   //   Report Count (52)
    0x81, 0x03,                   //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x06, 0x00, 0xFF, //   Usage Page (Vendor Defined 0xFF00)
    0x85, 0x21,       //   Report ID (33)
    0x09, 0x01,       //   Usage (0x01)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x3F,       //   Report Count (63)
    0x81, 0x03,       //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x85, 0x81, //   Report ID (-127)
    0x09, 0x02, //   Usage (0x02)
    0x75, 0x08, //   Report Size (8)
    0x95, 0x3F, //   Report Count (63)
    0x81, 0x03, //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x85, 0x01, //   Report ID (1)
    0x09, 0x03, //   Usage (0x03)
    0x75, 0x08, //   Report Size (8)
    0x95, 0x3F, //   Report Count (63)
    0x91, 0x83, //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)

    0x85, 0x10, //   Report ID (16)
    0x09, 0x04, //   Usage (0x04)
    0x75, 0x08, //   Report Size (8)
    0x95, 0x3F, //   Report Count (63)
    0x91, 0x83, //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)

    0x85, 0x80, //   Report ID (-128)
    0x09, 0x05, //   Usage (0x05)
    0x75, 0x08, //   Report Size (8)
    0x95, 0x3F, //   Report Count (63)
    0x91, 0x83, //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)

    0x85, 0x82, //   Report ID (-126)
    0x09, 0x06, //   Usage (0x06)
    0x75, 0x08, //   Report Size (8)
    0x95, 0x3F, //   Report Count (63)
    0x91, 0x83, //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)

    0xC0, // End Collection

};

static const uint8_t k_ns_dev_hid_descriptor_bt[NS_HID_BTC_REPORT_DESCRIPTOR_LEN] = {
    0x05, 0x01,       // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,       // Usage (Game Pad)
    0xA1, 0x01,       // Collection (Application)
    0x06, 0x01, 0xFF, //   Usage Page (Vendor Defined 0xFF01)

    0x85, 0x21, //   Report ID (33)
    0x09, 0x21, //   Usage (0x21)
    0x75, 0x08, //   Report Size (8)
    0x95, 0x30, //   Report Count (48)
    0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null
                //   Position)

    0x85, 0x30, //   Report ID (48)
    0x09, 0x30, //   Usage (0x30)
    0x75, 0x08, //   Report Size (8)
    0x95, 0x30, //   Report Count (48)
    0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null
                //   Position)

    0x85, 0x31,       //   Report ID (49)
    0x09, 0x31,       //   Usage (0x31)
    0x75, 0x08,       //   Report Size (8)
    0x96, 0x69, 0x01, //   Report Count (361)
    0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null
                      //   Position)

    0x85, 0x32,       //   Report ID (50)
    0x09, 0x32,       //   Usage (0x32)
    0x75, 0x08,       //   Report Size (8)
    0x96, 0x69, 0x01, //   Report Count (361)
    0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null
                      //   Position)

    0x85, 0x33,       //   Report ID (51)
    0x09, 0x33,       //   Usage (0x33)
    0x75, 0x08,       //   Report Size (8)
    0x96, 0x69, 0x01, //   Report Count (361)
    0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null
                      //   Position)

    0x85, 0x3F,                   //   Report ID (63)
    0x05, 0x09,                   //   Usage Page (Button)
    0x19, 0x01,                   //   Usage Minimum (0x01)
    0x29, 0x10,                   //   Usage Maximum (0x10)
    0x15, 0x00,                   //   Logical Minimum (0)
    0x25, 0x01,                   //   Logical Maximum (1)
    0x75, 0x01,                   //   Report Size (1)
    0x95, 0x10,                   //   Report Count (16)
    0x81, 0x02,                   //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null
                                  //   Position)
    0x05, 0x01,                   //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x39,                   //   Usage (Hat switch)
    0x15, 0x00,                   //   Logical Minimum (0)
    0x25, 0x07,                   //   Logical Maximum (7)
    0x75, 0x04,                   //   Report Size (4)
    0x95, 0x01,                   //   Report Count (1)
    0x81, 0x42,                   //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null
                                  //   State)
    0x05, 0x09,                   //   Usage Page (Button)
    0x75, 0x04,                   //   Report Size (4)
    0x95, 0x01,                   //   Report Count (1)
    0x81, 0x01,                   //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No
                                  //   Null Position)
    0x05, 0x01,                   //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,                   //   Usage (X)
    0x09, 0x31,                   //   Usage (Y)
    0x09, 0x33,                   //   Usage (Rx)
    0x09, 0x34,                   //   Usage (Ry)
    0x16, 0x00, 0x00,             //   Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00, //   Logical Maximum (65534)
    0x75, 0x10,                   //   Report Size (16)
    0x95, 0x04,                   //   Report Count (4)
    0x81, 0x02,                   //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null
                                  //   Position)
    0x06, 0x01, 0xFF,             //   Usage Page (Vendor Defined 0xFF01)

    0x85, 0x01, //   Report ID (1)
    0x09, 0x01, //   Usage (0x01)
    0x75, 0x08, //   Report Size (8)
    0x95, 0x30, //   Report Count (48)
    0x91, 0x02, //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                //   Null Position,Non-volatile)

    0x85, 0x10, //   Report ID (16)
    0x09, 0x10, //   Usage (0x10)
    0x75, 0x08, //   Report Size (8)
    0x95, 0x09, //   Report Count (9)
    0x91, 0x02, //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                //   Null Position,Non-volatile)

    0x85, 0x11, //   Report ID (17)
    0x09, 0x11, //   Usage (0x11)
    0x75, 0x08, //   Report Size (8)
    0x95, 0x30, //   Report Count (48)
    0x91, 0x02, //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                //   Null Position,Non-volatile)

    0x85, 0x12, //   Report ID (18)
    0x09, 0x12, //   Usage (0x12)
    0x75, 0x08, //   Report Size (8)
    0x95, 0x30, //   Report Count (48)
    0x91, 0x02, //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                //   Null Position,Non-volatile)
    0xC0,       // End Collection
};

static const uint8_t k_ns_usb_config_descriptor[NS_HID_USB_CONFIG_DESCRIPTOR_LEN] = {
    /* Configuration Descriptor (bLength=9) */
    9,    /* bLength              = 9 bytes */
    2,    /* bDescriptorType      = CONFIGURATION */
    41,   /* wTotalLength (lo)    = 41 bytes */
    0,    /* wTotalLength (hi) */
    1,    /* bNumInterfaces       = 2 */
    1,    /* bConfigurationValue  = 1 */
    0,    /* iConfiguration       = none */
    160,  /* bmAttributes         = 0xA0 (bus-powered, remote wakeup) */
    250,  /* bMaxPower            = 125 × 2mA = 250mA */

    /* Interface Descriptor 0 (HID) */
    9,    /* bLength              = 9 bytes */
    4,    /* bDescriptorType      = INTERFACE */
    0,    /* bInterfaceNumber     = 0 */
    0,    /* bAlternateSetting    = 0 */
    2,    /* bNumEndpoints        = 2 */
    3,    /* bInterfaceClass      = HID (0x03) */
    0,    /* bInterfaceSubClass   = none */
    0,    /* bInterfaceProtocol   = none */
    0,    /* iInterface           = none */

    /* HID Descriptor */
    9,    /* bLength              = 9 bytes */
    33,   /* bDescriptorType      = HID (0x21) */
    17,   /* bcdHID (lo)          = HID spec 1.11 */
    1,    /* bcdHID (hi) */
    0,    /* bCountryCode         = not localized */
    1,    /* bNumDescriptors      = 1 */
    34,   /* bDescriptorType      = REPORT (0x22) */
    203,  /* wDescriptorLength(lo)= 203 bytes */
    0,    /* wDescriptorLength(hi) */

    /* Endpoint Descriptor: EP1 IN (Interrupt) */
    7,    /* bLength              = 7 bytes */
    5,    /* bDescriptorType      = ENDPOINT */
    0x81, /* bEndpointAddress     = EP1 IN */
    3,    /* bmAttributes         = Interrupt */
    64,   /* wMaxPacketSize (lo)  = 64 bytes */
    0,    /* wMaxPacketSize (hi) */
    8,    /* bInterval            = 8ms (matches retail Pro Controller) */

    /* Endpoint Descriptor: EP1 OUT (Interrupt) */
    7,    /* bLength              = 7 bytes */
    5,    /* bDescriptorType      = ENDPOINT */
    0x01, /* bEndpointAddress     = EP1 OUT */
    3,    /* bmAttributes         = Interrupt */
    64,   /* wMaxPacketSize (lo)  = 64 bytes */
    0,    /* wMaxPacketSize (hi) */
    8,    /* bInterval            = 8ms (matches retail Pro Controller) */
};

const uint8_t k_ns_usb_webusb_config_descriptor[NS_HID_USB_WEBUSB_CONFIG_DESCRIPTOR_LEN] = {
    /* Configuration Descriptor (bLength=9) */
    9,    /* bLength              = 9 bytes */
    2,    /* bDescriptorType      = CONFIGURATION */
    64,   /* wTotalLength (lo)    = 64 bytes */
    0,    /* wTotalLength (hi) */
    2,    /* bNumInterfaces       = 2 */
    1,    /* bConfigurationValue  = 1 */
    0,    /* iConfiguration       = none */
    160,  /* bmAttributes         = 0xA0 (bus-powered, remote wakeup) */
    125,  /* bMaxPower            = 125 × 2mA = 250mA */

    /* Interface Descriptor 0 (HID) */
    9,    /* bLength              = 9 bytes */
    4,    /* bDescriptorType      = INTERFACE */
    0,    /* bInterfaceNumber     = 0 */
    0,    /* bAlternateSetting    = 0 */
    2,    /* bNumEndpoints        = 2 */
    3,    /* bInterfaceClass      = HID (0x03) */
    0,    /* bInterfaceSubClass   = none */
    0,    /* bInterfaceProtocol   = none */
    0,    /* iInterface           = none */

    /* HID Descriptor */
    9,    /* bLength              = 9 bytes */
    33,   /* bDescriptorType      = HID (0x21) */
    17,   /* bcdHID (lo)          = HID spec 1.11 */
    1,    /* bcdHID (hi) */
    0,    /* bCountryCode         = not localized */
    1,    /* bNumDescriptors      = 1 */
    34,   /* bDescriptorType      = REPORT (0x22) */
    203,  /* wDescriptorLength(lo)= 203 bytes */
    0,    /* wDescriptorLength(hi) */

    /* Endpoint Descriptor: EP1 IN (Interrupt) */
    7,    /* bLength              = 7 bytes */
    5,    /* bDescriptorType      = ENDPOINT */
    0x81, /* bEndpointAddress     = EP1 IN */
    3,    /* bmAttributes         = Interrupt */
    64,   /* wMaxPacketSize (lo)  = 64 bytes */
    0,    /* wMaxPacketSize (hi) */
    8,    /* bInterval            = 8ms (matches retail Pro Controller) */

    /* Endpoint Descriptor: EP1 OUT (Interrupt) */
    7,    /* bLength              = 7 bytes */
    5,    /* bDescriptorType      = ENDPOINT */
    0x01, /* bEndpointAddress     = EP1 OUT */
    3,    /* bmAttributes         = Interrupt */
    64,   /* wMaxPacketSize (lo)  = 64 bytes */
    0,    /* wMaxPacketSize (hi) */
    8,    /* bInterval            = 8ms (matches retail Pro Controller) */

    /* Interface Descriptor 1 (Vendor-specific) */
    9,    /* bLength              = 9 bytes */
    4,    /* bDescriptorType      = INTERFACE */
    1,    /* bInterfaceNumber     = 1 */
    0,    /* bAlternateSetting    = 0 */
    2,    /* bNumEndpoints        = 2 */
    255,  /* bInterfaceClass      = Vendor-specific (0xFF) */
    0,    /* bInterfaceSubClass   = 0 */
    0,    /* bInterfaceProtocol   = 0 */
    0,    /* iInterface           = none */

    /* Endpoint Descriptor: EP2 IN (Bulk) */
    7,    /* bLength              = 7 bytes */
    5,    /* bDescriptorType      = ENDPOINT */
    0x82, /* bEndpointAddress     = EP2 IN */
    2,    /* bmAttributes         = Bulk */
    64,   /* wMaxPacketSize (lo)  = 64 bytes */
    0,    /* wMaxPacketSize (hi) */
    0,    /* bInterval            = N/A (bulk) */

    /* Endpoint Descriptor: EP2 OUT (Bulk) */
    7,    /* bLength              = 7 bytes */
    5,    /* bDescriptorType      = ENDPOINT */
    0x02, /* bEndpointAddress     = EP2 OUT */
    2,    /* bmAttributes         = Bulk */
    64,   /* wMaxPacketSize (lo)  = 64 bytes */
    0,    /* wMaxPacketSize (hi) */
    0,    /* bInterval            = N/A (bulk) */
};

const ns_usb_device_descriptor_t *ns_hid_get_device_descriptor(void)
{
    return &k_ns_dev_usb_device_descriptor;
}

static void ns_hid_clear_descriptor_params(const uint8_t **hid_report_descriptor,
                                           uint16_t *hid_report_descriptor_len,
                                           const uint8_t **configuration_descriptor,
                                           uint16_t *configuration_descriptor_len, uint16_t *vid, uint16_t *pid)
{
    if (hid_report_descriptor != NULL)
    {
        *hid_report_descriptor = NULL;
    }
    if (hid_report_descriptor_len != NULL)
    {
        *hid_report_descriptor_len = 0u;
    }
    if (configuration_descriptor != NULL)
    {
        *configuration_descriptor = NULL;
    }
    if (configuration_descriptor_len != NULL)
    {
        *configuration_descriptor_len = 0u;
    }
    if (vid != NULL)
    {
        *vid = 0u;
    }
    if (pid != NULL)
    {
        *pid = 0u;
    }
}

bool ns_hid_get_descriptor_params(const uint8_t **hid_report_descriptor, uint16_t *hid_report_descriptor_len,
                                  const uint8_t **configuration_descriptor, uint16_t *configuration_descriptor_len,
                                  uint16_t *vid, uint16_t *pid)
{
    ns_device_config_s cfg = {0};
    ns_config_get(&cfg);

    if (cfg.transport != NS_TRANSPORT_USB && cfg.transport != NS_TRANSPORT_BTC)
    {
        ns_hid_clear_descriptor_params(hid_report_descriptor, hid_report_descriptor_len, configuration_descriptor,
                                       configuration_descriptor_len, vid, pid);
        return false;
    }

    const int use_bt = (cfg.transport == NS_TRANSPORT_BTC) ? 1 : 0;

    if (vid != NULL)
    {
        *vid = k_ns_dev_usb_device_descriptor.idVendor;
    }
    if (pid != NULL)
    {
        *pid = k_ns_dev_usb_device_descriptor.idProduct;
    }

    if (hid_report_descriptor != NULL)
    {
        *hid_report_descriptor = use_bt ? k_ns_dev_hid_descriptor_bt : k_ns_dev_hid_descriptor_usb;
    }
    if (hid_report_descriptor_len != NULL)
    {
        *hid_report_descriptor_len = use_bt ? NS_HID_BTC_REPORT_DESCRIPTOR_LEN : NS_HID_USB_REPORT_DESCRIPTOR_LEN;
    }

    if (configuration_descriptor != NULL)
    {
        *configuration_descriptor = use_bt ? NULL : k_ns_usb_config_descriptor;
    }
    if (configuration_descriptor_len != NULL)
    {
        *configuration_descriptor_len = use_bt ? 0u : NS_HID_USB_CONFIG_DESCRIPTOR_LEN;
    }

    return true;
}

const char *ns_hid_get_device_name(void)
{
    return k_ns_dev_name;
}
