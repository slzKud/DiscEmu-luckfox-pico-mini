#include <cstdlib>
#include <string>
#include <boost/filesystem.hpp>
#include "usb.h"
namespace fs = boost::filesystem;
void usb_switch_to_device_mode() {
// #ifdef USB_ON
//     system("/etc/uhubon.sh device");
// #endif
}

void usb_gadget_add_msc(fs::path block_dev) {
#ifdef USB_ON
    fs::path path_absolute = fs::absolute(block_dev);
    system(("./run_usb.sh probe msc file=" + path_absolute.string() + "").c_str());
#endif
}

void usb_gadget_add_cdrom(fs::path iso_path) {
#ifdef USB_ON
    fs::path path_absolute = fs::absolute(iso_path);
    if(fs::exists("/sys/kernel/config/usb_gadget/rockchip/functions/mass_storage.0/lun.0/file")){
        system(("echo \""+ path_absolute.string()+"\" > /sys/kernel/config/usb_gadget/rockchip/functions/mass_storage.0/lun.0/file").c_str());
        return;
    }
    system(("./run_usb.sh probe cdrom iso=" + path_absolute.string() + "").c_str());
#endif
}

void usb_gadget_add_cdrom() {
#ifdef USB_ON
    system("./run_usb.sh probe cdrom");
#endif
}

void usb_gadget_start() {
#ifdef USB_ON
    if(fs::exists("/sys/kernel/config/usb_gadget"))
        return;
    system("./run_usb.sh start");
#endif
}

void usb_gadget_stop() {
// #ifdef USB_ON
//     system("./run_usb.sh stop");    
// #endif
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