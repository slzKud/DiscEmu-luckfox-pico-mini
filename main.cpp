#include <algorithm>
#include <any>
#include <cstdarg>
#include <filesystem>
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

#include "input.h"
#include "menu.h"
#include "network.h"
#include "usb.h"
#include "util.h"

const std::string version = "0.1.3";
u8g2_t u8g2 = {};

typedef enum{
  KB = 1,
  MB = 2,
  GB = 3
} ImageSize;

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
int action_do_nothing(std::any arg);

ImageInfo image_info;
std::filesystem::path iso_root = std::filesystem::path("isos");

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
  usb_gadget_stop();
  usb_gadget_add_cdrom();
  usb_gadget_start();
}

int action_main_menu(std::any arg) {
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
  auto path = std::any_cast<std::filesystem::path>(arg);

  std::string ext = path.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  
  usb_gadget_stop();
  if (ext == ".iso") {
    usb_gadget_add_cdrom(path);
  } else if (ext == ".img") {
    usb_gadget_add_msc(path);
  }
  usb_gadget_start();

  Menu emu_menu { .title = "Current image" };
  std::vector<MenuItem> emu_menu_items = {      
      MenuItem{.name = path.filename().c_str(), .action = action_do_nothing},
      MenuItem{.name = "Eject",
               .action =
                  [](std::any) {
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
  auto path = std::any_cast<std::filesystem::path>(arg);
  bool result = false;
  bool file_or_folder=std::filesystem::is_directory(path);
  u8g2_ClearBuffer(&u8g2);
  u8g2_SetDrawColor(&u8g2, 1);
  u8g2_DrawStr(&u8g2, 0, 28, file_or_folder?"Deleting folder....":"Deleting file....");
  u8g2_SendBuffer(&u8g2);
  sleep(1);
  #ifdef USB_ON
  // TODO:Delete File.
  if(!std::filesystem::is_directory(path)){
    if(std::filesystem::remove(path)){
      result=true;
    }
  }else{
    if(std::filesystem::remove_all(path)>0){
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
  auto path = std::any_cast<std::filesystem::path>(arg);
  Menu dir_menu { .title = std::filesystem::is_directory(path)?"Delele folder?":"Delete File?" };
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
    menu_clear_off_on(true);
    #else
    sleep(3);
    #endif
  }
  u8g2_ClearBuffer(&u8g2);
  u8g2_SetDrawColor(&u8g2, 1);
  if(!std::filesystem::exists(std::filesystem::path(image_info.name))){
    u8g2_DrawStr(&u8g2, 0, 28, "Create image Failed.");
  }else{
    u8g2_DrawStr(&u8g2, 0, 28, create_aborted?"Action aborted.":"Create image OK.");
  }
  u8g2_SendBuffer(&u8g2);
  sleep(3);
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
  auto path = std::any_cast<std::filesystem::path>(arg);
  std::cout << "action_create_image_select:path="<< path.string() << std::endl;
  image_info.path=path.string();
  image_info.per_size=GB;
  image_info.size=0;
  image_info.aborted=false;
  DiskSpaceInfo disk_space;
  getDiskSpace("/",disk_space);
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
                      .action_arg = 1});
  if(disk_space.freeGB>4)
    dir_menu_items.push_back(
        MenuItem{.name = "4 GB",
                      .action = action_create_image_confirm,
                      .action_arg = 1});
  if(disk_space.freeGB>8)
    dir_menu_items.push_back(
        MenuItem{.name = "8 GB",
                      .action = action_create_image_confirm,
                      .action_arg = 1});
  if(disk_space.freeGB>16)
    dir_menu_items.push_back(
        MenuItem{.name = "16 GB",
                      .action = action_create_image_confirm,
                      .action_arg = 1});
  menu_init(&dir_menu, &dir_menu_items);
  menu_run(&dir_menu, &u8g2);
  return 0;
}
int action_file_mamager(std::any arg) {
  auto path = std::any_cast<std::filesystem::path>(arg);
  Menu dir_menu { .title = std::filesystem::is_directory(path)?"Folder action":"File action" };
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
                    .action_arg = std::filesystem::is_directory(path)?path:path.parent_path()});
  menu_init(&dir_menu, &dir_menu_items);
  menu_run(&dir_menu, &u8g2);
  return 0;
}
int action_file_browser(std::any arg) {
  auto path = std::any_cast<std::filesystem::path>(arg);
  if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
    action_errmsg(std::string("Failed to open dir"));
    return 0;
  }

  std::vector<MenuItem> dir_menu_items;
  std::vector<MenuItem> iso_menu_items;
  Menu dir_menu { .title = path.filename().string() };

  dir_menu_items.push_back(MenuItem{.name = "[..]", .action = nullptr});
  for (const auto &entry : std::filesystem::directory_iterator(path)) {
    std::string name;
    std::filesystem::path entry_path = entry.path();
    if (entry.is_directory()) {
      dir_menu_items.push_back(
          MenuItem{.name = '[' + entry_path.filename().string() + ']',
                   .action = action_file_browser,
                   .long_action = action_file_mamager,
                   .action_arg = entry.path()});
    } else {
      std::string ext = entry_path.extension().string();
      std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
      if (ext == ".iso" || ext == ".img") {
        iso_menu_items.push_back(MenuItem{.name = entry_path.filename(),
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
  if(getDiskSpace("/",disk_space)){
    status_menu_items.push_back(MenuItem{.name = "Total: "+std::to_string(disk_space.totalGB)+" GB"});
    status_menu_items.push_back(MenuItem{.name = "Free Space: "+std::to_string(disk_space.freeGB)+" GB"});
  }
  for (auto const &pair : ip_map) {
    status_menu_items.push_back(MenuItem{.name = pair.first + " IP"});
    status_menu_items.push_back(MenuItem{.name = pair.second});
  }

  menu_init(&status_menu, &status_menu_items);
  menu_run(&status_menu, &u8g2);
  return 0;
}

int action_rndis(std::any arg) {
  u8g2_ClearBuffer(&u8g2);
  u8g2_SetDrawColor(&u8g2, 1);
  u8g2_DrawStr(&u8g2, 0, 28, "Initializing...");
  u8g2_SendBuffer(&u8g2);

  usb_gadget_stop();
  rndis_start();

  Menu rndis_menu { .title = "USB RNDIS" };
  std::vector<MenuItem> rndis_menu_items = {      
      MenuItem{.name = "IP: 192.168.42.1", .action = action_do_nothing},      
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
  if (!std::filesystem::exists("/usr/bin/umtprd") || !std::filesystem::exists("/etc/umtprd/umtprd.conf")) {
    action_errmsg(std::string("unable to open MTP."));
    return 0;
  }
  u8g2_ClearBuffer(&u8g2);
  u8g2_SetDrawColor(&u8g2, 1);
  u8g2_DrawStr(&u8g2, 0, 28, "Initializing...");
  u8g2_SendBuffer(&u8g2);

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

int main(void) {
  usb_switch_to_device_mode();
  reset_disc_emu();

  if (!std::filesystem::exists(iso_root)) {
    std::filesystem::create_directory(iso_root);
  } else if (!std::filesystem::is_directory(iso_root)) {
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
  std::cout <<"i2c mode,i2c number 0" << std::endl;
  u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_arm_linux_hw_i2c,
                                     u8x8_arm_linux_gpio_and_delay);

  u8x8_SetPin(p_u8x8, U8X8_PIN_I2C_CLOCK, U8X8_PIN_NONE);
  u8x8_SetPin(p_u8x8, U8X8_PIN_I2C_DATA, U8X8_PIN_NONE);
  u8x8_SetPin(p_u8x8, U8X8_PIN_RESET, U8X8_PIN_NONE);

  success = u8g2arm_arm_init_hw_i2c(p_u8x8, 0); // I2C 0
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

  action_main_menu(NULL);

  u8x8_ClearDisplay(p_u8x8);
  input_stop();
  return 0;
}