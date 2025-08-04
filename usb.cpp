#include <cstdlib>
#include <string>
#include <filesystem>
#include "usb.h"

void usb_switch_to_device_mode() {
#ifdef USB_ON
    system("/etc/uhubon.sh device");
#endif
}

void usb_gadget_add_msc(std::filesystem::path block_dev) {
#ifdef USB_ON
    std::filesystem::path path_absolute = std::filesystem::absolute(block_dev);
    system(("./run_usb.sh probe msc '" + path_absolute.string() + "'").c_str());
#endif
}

void usb_gadget_add_cdrom(std::filesystem::path iso_path) {
#ifdef USB_ON
    std::filesystem::path path_absolute = std::filesystem::absolute(iso_path);
    system(("./run_usb.sh probe cdrom '" + path_absolute.string() + "'").c_str());
#endif
}

void usb_gadget_add_cdrom() {
#ifdef USB_ON
    system("./run_usb.sh probe cdrom");
#endif
}

void usb_gadget_start() {
#ifdef USB_ON
    system("./run_usb.sh start");
#endif
}

void usb_gadget_stop() {
#ifdef USB_ON
    system("./run_usb.sh stop");    
#endif
}

void usb_gadget_add_rndis() {
#ifdef USB_ON
    system("./run_usb.sh probe rndis");
#endif
}

void usb_gadget_add_mtp() {
#ifdef USB_ON
    system("./run_usb.sh probe mtp");
#endif
}