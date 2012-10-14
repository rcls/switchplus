// Monkey serial driver.

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/tty.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/usb/serial.h>
#include <linux/uaccess.h>

//static bool debug;

static struct usb_device_id monkey_ids[] = {
    {
        .match_flags = USB_DEVICE_ID_MATCH_VENDOR
        | USB_DEVICE_ID_MATCH_PRODUCT
        | USB_DEVICE_ID_MATCH_INT_CLASS
        | USB_DEVICE_ID_MATCH_INT_SUBCLASS
        | USB_DEVICE_ID_MATCH_INT_PROTOCOL,
        .idVendor = 0xf055,
        .idProduct = 0x524c,
        .bInterfaceClass = 0xff,
        .bInterfaceSubClass = 'S',
        .bInterfaceProtocol = 'S'
    },
    {} };

MODULE_DEVICE_TABLE(usb, monkey_ids);

static struct usb_serial_driver monkey_device = {
    .driver = {
        .owner = THIS_MODULE,
        .name =  "monkey",
    },
    .id_table =  monkey_ids,
    .num_ports = 1,
};

static struct usb_serial_driver * const serial_drivers[] = {
    &monkey_device, NULL
};

module_usb_serial_driver(serial_drivers, monkey_ids);

MODULE_LICENSE("GPL");

// module_param(debug, bool, S_IRUGO | S_IWUSR);
// MODULE_PARM_DESC(debug, "Debug enabled or not");
