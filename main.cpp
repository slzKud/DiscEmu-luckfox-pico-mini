#include <algorithm>
#include <any>
#include <cstdarg>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <unistd.h>

#include <libu8g2arm/U8g2Controller.h>
#include <libu8g2arm/U8g2lib.h>
#include <libu8g2arm/u8g2.h>
#include <libu8g2arm/u8g2arm.h>
#include <libu8g2arm/u8x8.h>

#include <boost/filesystem.hpp>

#include "input.h"
#include "menu.h"
#include "network.h"
#include "usb.h"
#include "util.h"

namespace fs = boost::filesystem;

const std::string version = "0.2.1";
u8g2_t u8g2 = {};
int main_menu_flag=0;
typedef enum{
  KB = 1,
  MB = 2,
  GB = 3
} ImageSize;

typedef enum{
  NO_EMU,
  RNDIS,
  MTP,
  MSD,
  MSC,
  MSFD,
} EMUStatus;

typedef struct {
    std::string name = "";
    std::string path = "";
    ImageSize per_size = KB;
    int size = 0;
    bool aborted = false;
} ImageInfo;

int action_rndis(std::any arg);
int action_mtp(std::any arg);
int action_cdrom_emu(std::any arg);
int action_file_browser(std::any arg);
int action_status(std::any arg);
int action_shutdown(std::any arg);
int action_restart(std::any arg);
int action_reset(std::any arg);
int action_floppy_mode(std::any arg);
int action_ro_mode(std::any arg);
int action_do_nothing(std::any arg);
int action_main_menu(std::any arg);

ImageInfo image_info;
EMUStatus emu_status=NO_EMU;
fs::path iso_root = fs::path("isos");

std::vector<MenuItem> main_menu = {
    MenuItem{.name = "Image Browser",
             .action = action_file_browser,
             .action_arg = iso_root},
    MenuItem{.name = "File Transfer", .action = action_mtp},
    MenuItem{.name = "USB RNDIS", .action = action_rndis},
    MenuItem{.name = "Status", .action = action_status},
    MenuItem{.name = "Shutdown", .action = action_shutdown},
    MenuItem{.name = "Restart", .action = action_restart}};

void reset_disc_emu() {
  if(emu_status==NO_EMU){
    // 如果当前没有模拟,则暂时不模拟东西（避免额外的重启）,或者只模拟MSC(存在标志文件时)
    if(fs::exists("/etc/normal_msc.flag")){
      emu_status=MSC;
      usb_gadget_add_cdrom();
      usb_gadget_start();
      return;
    }
  }
  if(emu_status==RNDIS || emu_status==MTP){
    // 如果当前模拟的是RNDIS和MTP，则reset等于直接重启
    action_reset(NULL);
    return;
  }
  if(emu_status==MSD || emu_status==MSFD){
    // 如果当前模拟的是USB磁盘，也直接重启，因为清除file之后就是一个空磁盘，没有意义。
    action_reset(NULL);
    return;
  }
  if(emu_status==MSC){
    // 如果当前模拟的是USB光驱，只清除file。
    system("echo "" > /sys/kernel/config/usb_gadget/rockchip/functions/mass_storage.0/lun.0/file");
    if(main_menu_flag==0){
      action_main_menu(NULL);
    }
    return;
  }
}

int action_main_menu(std::any arg) {
  main_menu_flag=1;
  Menu menu { .title = "DiscEmu" };
  menu_init(&menu, &main_menu);
  menu_run(&menu, &u8g2);
  return 0;
}

int action_do_nothing(std::any arg) { return 0; }

int action_do_nothing_printf(std::any arg) { 
  printf("user do this action\n");
  return 0; 
}

int action_errmsg(std::any arg) {
  std::string msg = std::any_cast<std::string>(arg);
  u8g2_ClearBuffer(&u8g2);
  u8g2_SetDrawColor(&u8g2, 1);
  u8g2_DrawStr(&u8g2, 0, 28, msg.c_str());
  u8g2_SendBuffer(&u8g2);

  sleep(3);
  return 0;
}

int action_disk_emu(std::any arg) {
  auto path = std::any_cast<fs::path>(arg);

  std::string ext = path.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  
  usb_gadget_stop();
  if (ext == ".iso") {
    emu_status=MSC;
    system(("echo \""+ path.string()+"\" > /etc/msc.flag").c_str());
    system("sync");
    usb_gadget_add_cdrom(path);
  } else if (ext == ".img") {
    if(emu_status==MSC){
      system(("echo \""+ path.string()+"\" > /etc/msc.flag").c_str());
      system("sync");
      action_reset(NULL);
    }
    emu_status=MSD;
    usb_gadget_add_msc(path);
  } else if (ext == ".flp") {
    if(emu_status==MSC){
      system(("echo \""+ path.string()+"\" > /etc/msc.flag").c_str());
      system("sync");
      action_reset(NULL);
    }
    emu_status=MSFD;
    usb_gadget_add_floppy(path);
  }
  usb_gadget_start();

  Menu emu_menu { .title = "Current image" };
  std::vector<MenuItem> emu_menu_items = {      
      MenuItem{.name = path.filename().c_str(), .action = action_do_nothing},
      MenuItem{.name = "Eject",
               .action =
                  [](std::any) {
                    system("rm -rf /etc/msc.flag");
                    system("sync");
                    reset_disc_emu();
                    usleep(500 * 1000);
                    return -1;
                  }},
      MenuItem{.name = "", .action = action_do_nothing},
  };
  menu_init(&emu_menu, &emu_menu_items);
  menu_run(&emu_menu, &u8g2);
  return 0;
}
int action_file_delete(std::any arg) {
  auto path = std::any_cast<fs::path>(arg);
  bool result = false;
  bool file_or_folder=fs::is_directory(path);
  u8g2_ClearBuffer(&u8g2);
  u8g2_SetDrawColor(&u8g2, 1);
  u8g2_DrawStr(&u8g2, 0, 28, file_or_folder?"Deleting folder....":"Deleting file....");
  u8g2_SendBuffer(&u8g2);
  sleep(1);
  #ifdef USB_ON
  // TODO:Delete File.
  if(!fs::is_directory(path)){
    if(fs::remove(path)){
      result=true;
    }
  }else{
    if(fs::remove_all(path)>0){
      result=true;
    }
  }
  #endif
  u8g2_ClearBuffer(&u8g2);
  u8g2_SetDrawColor(&u8g2, 1);
  if(result){
    u8g2_DrawStr(&u8g2, 0, 28, file_or_folder?"Delete folder OK.":"Delete file OK.");
  }else{
    u8g2_DrawStr(&u8g2, 0, 28, "Delete Failed.");
  }
  u8g2_SendBuffer(&u8g2);
  sleep(3);
  if(result){
    return action_main_menu(NULL);
  }else{
    return -1;
  }
}
int action_file_delete_confirm(std::any arg) {
  auto path = std::any_cast<fs::path>(arg);
  Menu dir_menu { .title = fs::is_directory(path)?"Delele folder?":"Delete File?" };
  std::vector<MenuItem> dir_menu_items;
  dir_menu_items.push_back(
      MenuItem{ .name = path.filename().string()});
  dir_menu_items.push_back(
      MenuItem{.name = "Yes",
                    .action = action_file_delete,
                    .action_arg = path});
  dir_menu_items.push_back(
      MenuItem{.name = "No",
                    .action = nullptr});
  menu_init(&dir_menu, &dir_menu_items);
  menu_run(&dir_menu, &u8g2);
  return 0;
}

int action_create_image(std::any arg) {
  auto create_aborted = std::any_cast<bool>(arg);
  if(!create_aborted){
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetDrawColor(&u8g2, 1);
    u8g2_DrawStr(&u8g2, 0, 28, "Creating image....");
    u8g2_SendBuffer(&u8g2);
    #ifdef USB_ON
    menu_clear_off_on(false);
    system(("./make_fat32_img.sh "+ image_info.name +" "+std::to_string(image_info.size) +" GB").c_str());
    #else
    sleep(3);
    #endif
  }
  u8g2_ClearBuffer(&u8g2);
  u8g2_SetDrawColor(&u8g2, 1);
  if(!fs::exists(fs::path(image_info.name))){
    u8g2_DrawStr(&u8g2, 0, 28, "Create image Failed.");
  }else{
    u8g2_DrawStr(&u8g2, 0, 28, create_aborted?"Action aborted.":"Create image OK.");
  }
  u8g2_SendBuffer(&u8g2);
  sleep(3);
  #ifdef USB_ON
  menu_clear_off_on(true);
  #endif
  return action_main_menu(NULL);
}

int action_create_image_confirm(std::any arg) {
  auto image_size = std::any_cast<int>(arg);
  std::string name=getDiskImageFilename("./"+image_info.path,std::to_string(image_size) +"GB");
  image_info.size=image_size;
  image_info.name=image_info.path + "/" + name;
  Menu dir_menu { .title = "Confirm image info" };
  std::vector<MenuItem> dir_menu_items;
  dir_menu_items.push_back(
      MenuItem{ .name = "Name: "+ name + " Size:" + std::to_string(image_size) +" GB"});
  dir_menu_items.push_back(
      MenuItem{.name = "Yes",
                    .action = action_create_image,
                    .action_arg = false});
  dir_menu_items.push_back(
      MenuItem{.name = "No",
                    .action = action_create_image,
                   .action_arg = true});
  menu_init(&dir_menu, &dir_menu_items);
  menu_run(&dir_menu, &u8g2);
  return 0;
}
int action_create_image_select(std::any arg) {
  auto path = std::any_cast<fs::path>(arg);
  std::cout << "action_create_image_select:path="<< path.string() << std::endl;
  image_info.path=path.string();
  image_info.per_size=GB;
  image_info.size=0;
  image_info.aborted=false;
  DiskSpaceInfo disk_space;
  getDiskSpace("/mnt/sdcard",disk_space);
  Menu dir_menu { .title = "Choose Disk Size" };
  std::vector<MenuItem> dir_menu_items;
  dir_menu_items.push_back(
      MenuItem{.name = "Back",
                    .action = nullptr});
  if(disk_space.freeGB>1)
    dir_menu_items.push_back(
        MenuItem{.name = "1 GB",
                      .action = action_create_image_confirm,
                      .action_arg = 1});
  if(disk_space.freeGB>2)
    dir_menu_items.push_back(
        MenuItem{.name = "2 GB",
                      .action = action_create_image_confirm,
                      .action_arg = 2});
  if(disk_space.freeGB>4)
    dir_menu_items.push_back(
        MenuItem{.name = "4 GB",
                      .action = action_create_image_confirm,
                      .action_arg = 4});
  if(disk_space.freeGB>8)
    dir_menu_items.push_back(
        MenuItem{.name = "8 GB",
                      .action = action_create_image_confirm,
                      .action_arg = 8});
  if(disk_space.freeGB>16)
    dir_menu_items.push_back(
        MenuItem{.name = "16 GB",
                      .action = action_create_image_confirm,
                      .action_arg = 16});
  menu_init(&dir_menu, &dir_menu_items);
  menu_run(&dir_menu, &u8g2);
  return 0;
}
int action_file_mamager(std::any arg) {
  auto path = std::any_cast<fs::path>(arg);
  Menu dir_menu { .title = fs::is_directory(path)?"Folder action":"File action" };
  std::vector<MenuItem> dir_menu_items;
  dir_menu_items.push_back(
      MenuItem{ .name = "Back",
                .action = nullptr});
  dir_menu_items.push_back(
      MenuItem{.name = "Delete",
                    .action = action_file_delete_confirm,
                    .action_arg = path});
  dir_menu_items.push_back(
      MenuItem{.name = "Create disk image",
                    .action = action_create_image_select,
                    .action_arg = fs::is_directory(path)?path:path.parent_path()});
  menu_init(&dir_menu, &dir_menu_items);
  menu_run(&dir_menu, &u8g2);
  return 0;
}
int action_file_browser(std::any arg) {
  auto path = std::any_cast<fs::path>(arg);
  if (!fs::exists(path) || !fs::is_directory(path)) {
    action_errmsg(std::string("Failed to open dir"));
    return 0;
  }

  std::vector<MenuItem> dir_menu_items;
  std::vector<MenuItem> iso_menu_items;
  Menu dir_menu { .title = path.filename().string() };

  dir_menu_items.push_back(MenuItem{.name = "[..]", .action = nullptr});
  for (const auto &entry : fs::directory_iterator(path)) {
    std::string name;
    fs::path entry_path = entry.path();
    if (entry.is_directory()) {
      dir_menu_items.push_back(
          MenuItem{.name = '[' + entry_path.filename().string() + ']',
                   .action = action_file_browser,
                   .long_action = action_file_mamager,
                   .action_arg = entry.path()});
    } else {
      std::string ext = entry_path.extension().string();
      std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
      if (ext == ".iso" || ext == ".img" || ext == ".flp") {
        iso_menu_items.push_back(MenuItem{.name = entry_path.filename().string(),
                                          .action = action_disk_emu,
                                          .long_action = action_file_mamager,
                                          .action_arg = entry.path()});
      }
    }
  }

  std::sort(dir_menu_items.begin(), dir_menu_items.end(),
            [](MenuItem a, MenuItem b) { return a.name < b.name; });
  std::sort(iso_menu_items.begin(), iso_menu_items.end(),
            [](MenuItem a, MenuItem b) { return a.name < b.name; });

  for (auto const &items : iso_menu_items) {
    dir_menu_items.push_back(items);
  }
  menu_init(&dir_menu, &dir_menu_items);
  menu_run(&dir_menu, &u8g2);
  return 0;
}

int action_status(std::any arg) {
  std::map<std::string, std::string> ip_map = get_ip_addr();
  DiskSpaceInfo disk_space;
  
  Menu status_menu { .title = "Status" };
  std::vector<MenuItem> status_menu_items = {      
      MenuItem{.name = "Version: " + version}
  };
  if(getDiskSpace("/mnt/sdcard",disk_space)){
    status_menu_items.push_back(MenuItem{.name = "Total: "+roundNumber(disk_space.totalGB,2)+" GB"});
    status_menu_items.push_back(MenuItem{.name = "Free Space: "+roundNumber(disk_space.freeGB,2)+" GB"});
  }
  for (auto const &pair : ip_map) {
    status_menu_items.push_back(MenuItem{.name = pair.first + " IP"});
    status_menu_items.push_back(MenuItem{.name = pair.second});
  }
  if(fs::exists("/etc/floppy.flag")){
    status_menu_items.push_back(MenuItem{
      .name = "Floppy Mode:ON",
      .action = action_floppy_mode
    });
  }else{
    status_menu_items.push_back(MenuItem{
      .name = "Floppy Mode:OFF",
      .action = action_floppy_mode
    });
  }
  if(fs::exists("/etc/ro.flag")){
    status_menu_items.push_back(MenuItem{
      .name = "Read-Only Mode:ON",
      .action = action_ro_mode
    });
  }else{
    status_menu_items.push_back(MenuItem{
      .name = "Read-Only Mode:OFF",
      .action = action_ro_mode
    });
  }
  menu_init(&status_menu, &status_menu_items);
  menu_run(&status_menu, &u8g2);
  return 0;
}

int action_rndis(std::any arg) {
  if(emu_status==MSC){
    system("touch /etc/rndis.flag");
    action_reset(NULL);
    return 0;
  }
  u8g2_ClearBuffer(&u8g2);
  u8g2_SetDrawColor(&u8g2, 1);
  u8g2_DrawStr(&u8g2, 0, 28, "Initializing...");
  u8g2_SendBuffer(&u8g2);
  emu_status=RNDIS;
  usb_gadget_stop();
  rndis_start();
  Menu rndis_menu { .title = "USB RNDIS" };
  std::vector<MenuItem> rndis_menu_items = {      
      MenuItem{.name = "IP: 172.32.0.70", .action = action_do_nothing},      
      MenuItem{.name = "Disconnect",
               .action =
                   [](std::any) {
                     rndis_stop();
                     reset_disc_emu();
                     usleep(500 * 1000);
                     return -1;
                   }},
      MenuItem{.name = "", .action = action_do_nothing},
  };
  menu_init(&rndis_menu, &rndis_menu_items);
  menu_run(&rndis_menu, &u8g2);
  return 0;
}
int action_mtp(std::any arg) {
  if (!fs::exists("/usr/sbin/umtprd") || !fs::exists("/etc/umtprd/umtprd.conf")) {
    action_errmsg(std::string("unable to open MTP."));
    return 0;
  }
  if(emu_status==MSC){
    system("touch /etc/mtp.flag");
    action_reset(NULL);
    return 0;
  }
  u8g2_ClearBuffer(&u8g2);
  u8g2_SetDrawColor(&u8g2, 1);
  u8g2_DrawStr(&u8g2, 0, 28, "Initializing...");
  u8g2_SendBuffer(&u8g2);
  emu_status=MTP;
  usb_gadget_stop();
  usb_gadget_add_mtp();
  usb_gadget_start();

  Menu mtp_menu { .title = "File Transfer" };
  std::vector<MenuItem> mtp_menu_items = {      
      MenuItem{.name = "Now you can transfer file via USB.", .action = action_do_nothing},      
      MenuItem{.name = "Back to menu",
               .action =
                   [](std::any) {
                     usb_gadget_stop();
                     reset_disc_emu();
                     usleep(500 * 1000);
                     return -1;
                   }},
      MenuItem{.name = "", .action = action_do_nothing},
  };
  menu_init(&mtp_menu, &mtp_menu_items);
  menu_run(&mtp_menu, &u8g2);
  return 0;
}
int action_shutdown(std::any arg) {
  u8g2_ClearBuffer(&u8g2);
  u8g2_SetDrawColor(&u8g2, 1);
  u8g2_DrawStr(&u8g2, 0, 28, "Shutting down...");
  u8g2_SendBuffer(&u8g2);

  system("halt");
  sleep(1000);
  return 0;
}

int action_restart(std::any arg) {
  u8g2_ClearBuffer(&u8g2);
  u8g2_SetDrawColor(&u8g2, 1);
  u8g2_DrawStr(&u8g2, 0, 28, "Restarting...");
  u8g2_SendBuffer(&u8g2);

  system("reboot");
  sleep(1000);
  return 0;
}
int action_reset(std::any arg) {
  u8g2_ClearBuffer(&u8g2);
  u8g2_SetDrawColor(&u8g2, 1);
  u8g2_DrawStr(&u8g2, 0, 28, "Please wait...");
  u8g2_SendBuffer(&u8g2);
  system("reboot");
  sleep(1000);
  return 0;
}
int action_floppy_mode(std::any arg) {
  if(fs::exists("/etc/floppy.flag")){
    system("rm -rf /etc/floppy.flag");
  }else{
    system("touch /etc/floppy.flag");
  }
  return action_reset(NULL);
}
int action_ro_mode(std::any arg) {
  if(fs::exists("/etc/ro.flag")){
    system("rm -rf /etc/ro.flag");
  }else{
    system("touch /etc/ro.flag");
  }
  return action_reset(NULL);
}
void runonce(void){
  if(fs::exists("/etc/rndis.flag")){
    system("rm -rf /etc/rndis.flag");
    action_rndis(NULL);
    return;
  }
  if(fs::exists("/etc/mtp.flag")){
    system("rm -rf /etc/mtp.flag");
    action_mtp(NULL);
    return;
  }
  if(fs::exists("/etc/msc.flag")){
    std::string k=readFileIntoString("/etc/msc.flag");
    std::cout << "msc.flag exist,check:" << k << ",done."
              << std::endl;
    if(fs::exists(k)){
      std::cout << "file exists."
              << std::endl;
      //system("rm -rf /etc/msc.flag");
      action_disk_emu(fs::path(k));
      return;
    }
  }
  reset_disc_emu();
  action_main_menu(NULL);
  return;
}

int main(void) {
  //usb_switch_to_device_mode();
  if (!fs::exists(iso_root)) {
    fs::create_directory(iso_root);
  } else if (!fs::is_directory(iso_root)) {
    std::cout << "cannot create isos directory: file already exists"
              << std::endl;
    return 1;
  }

  int success;
  success = input_init();
  if (!success) {
    std::cout << "failed to initialize input" << std::endl;
    return 1;
  }

  u8x8_t *p_u8x8 = u8g2_GetU8x8(&u8g2);
  #ifndef I2C_DISPLAY
  std::cout <<"spi mode,spi number 0" << std::endl;
  const int RES_PIN = 22;
  const int DC_PIN = 21;
  u8g2_Setup_ssd1306_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_arm_linux_hw_spi,
                                     u8x8_arm_linux_gpio_and_delay);

  u8x8_SetPin(p_u8x8, U8X8_PIN_SPI_CLOCK, U8X8_PIN_NONE);
  u8x8_SetPin(p_u8x8, U8X8_PIN_SPI_DATA, U8X8_PIN_NONE);
  u8x8_SetPin(p_u8x8, U8X8_PIN_RESET, RES_PIN);
  u8x8_SetPin(p_u8x8, U8X8_PIN_DC, DC_PIN);
  u8x8_SetPin(p_u8x8, U8X8_PIN_CS, U8X8_PIN_NONE);
  u8x8_SetPin(p_u8x8, U8X8_PIN_MENU_SELECT, 18);

  success = u8g2arm_arm_init_hw_spi(p_u8x8, 0, 0, 10);
  if (!success) {
    std::cout << "failed to initialize display" << std::endl;
    return 1;
  }
  #else
  std::cout <<"i2c mode,i2c number "<< 3 << std::endl;
  u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_arm_linux_hw_i2c,
                                     u8x8_arm_linux_gpio_and_delay);

  u8x8_SetPin(p_u8x8, U8X8_PIN_I2C_CLOCK, U8X8_PIN_NONE);
  u8x8_SetPin(p_u8x8, U8X8_PIN_I2C_DATA, U8X8_PIN_NONE);
  u8x8_SetPin(p_u8x8, U8X8_PIN_RESET, U8X8_PIN_NONE);

  success = u8g2arm_arm_init_hw_i2c(p_u8x8, 3); // I2C 3
  if (!success) {
    std::cout << "failed to initialize display" << std::endl;
    return 1;
  }
  #endif
  u8x8_InitDisplay(p_u8x8);
  #ifndef I2C_DISPLAY
  u8g2_ClearDisplay(&u8g2);
  #endif
  u8x8_SetPowerSave(p_u8x8, 0);
  u8g2_SetFont(&u8g2, u8g2_font_6x13_tf); // choose a suitable font
  
  runonce();

  u8x8_ClearDisplay(p_u8x8);
  input_stop();
  return 0;
}