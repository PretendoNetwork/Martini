#include "menu/menu_done.hpp"
#include "menu/menu.hpp"

#include <coreinit/screen.h>

#include <string>

void RenderMenuDone(MenuDoneError err) {
    RenderMenuBasic();

    std::string message;
    bool error = true;
    switch (err) {
        case MENU_DONE_NO_ERROR:
            message = "Juxtaposition was patched into your Miiverse applet successfully.";
            error = false;
            break;
        case MENU_DONE_MCP_FAIL:
            message = "Could not open MCP. This can sometimes be caused by HaxchiFW. You may need to restart your console.";
            break;
        case MENU_DONE_NO_MIIVERSE:
            message = "Could not find the Miiverse applet installed on your console.";
            break;
        case MENU_DONE_NO_CFW:
            message = "Could not find a compatible CFW on your console.";
            break;
        case MENU_DONE_CFW_FAIL:
            message = "Failed to mount the IOSU. You may need to restart your console.";
            break;
        case MENU_DONE_NO_BACKUP:
            message = "Could not find a clean version of the Miiverse applet.";
            break;
        case MENU_DONE_NO_CERT_BACKUP:
            message = "Could not find a clean version of the NSSL cert to backup.";
            break;
        case MENU_DONE_BACKUP_FAIL:
            message = "Failed to create a backup of the Miiverse applet.";
            break;
        case MENU_DONE_PATCH_FAIL:
            message = "Failed to apply a patch. Your system has not been modified.";
            break;
        case MENU_DONE_PATCH_BAD:
            message = "Patch created a corrupt file. Your system has not been modified.";
            break;
        case MENU_DONE_PATCH_FAIL_DANGEROUS:
            message = "Failed to apply a patch. Your system has been modified and may be in an unstable state.";
            break;
        case MENU_DONE_PATCH_FAIL_CERT_BRICK:
            message = "Failed to replace a cert. This could brick - seek help on our Discord immediately.";
            break;
         case MENU_DONE_RESTORE_DONE:
            message = "Juxtaposition has been uninstalled successfully.";
            error = false;
            break; 
         case MENU_DONE_RESTORE_FAIL:
            message = "Failed to restore Miiverse.";
            break;   
        default:
            message = "An unknown error occured.";
            break;
    }

    int support_code = static_cast<int>(err) + 1000;
    std::string support_code_msg = "Support code: 678-" + std::to_string(support_code);

    OSScreenPutFontEx(SCREEN_TV, 0, 20, message.c_str());
    if (error) {
        OSScreenPutFontEx(SCREEN_TV, 0, 22, "See pretendo.network/docs for details and fixes.");
    } else {
        OSScreenPutFontEx(SCREEN_TV, 0, 22, "Please reboot your console.");
    }
    OSScreenPutFontEx(SCREEN_TV, 0, 23, support_code_msg.c_str());
    if (error) {
        OSScreenPutFontEx(SCREEN_TV, 0, 26, "HOME: Quit");
    } else {
        OSScreenPutFontEx(SCREEN_TV, 0, 26, "A: Reboot");
    }

    OSScreenPutFontEx(SCREEN_DRC, 0, 10, message.c_str());
    if (error) {
        OSScreenPutFontEx(SCREEN_DRC, 0, 11, "See pretendo.network/docs for details and fixes.");
    } else {
        OSScreenPutFontEx(SCREEN_DRC, 0, 11, "Please reboot your console.");
    }
    OSScreenPutFontEx(SCREEN_DRC, 0, 12, support_code_msg.c_str());
    if (error) {
        OSScreenPutFontEx(SCREEN_DRC, 0, 17, "HOME: Quit");
    } else {
        OSScreenPutFontEx(SCREEN_DRC, 0, 17, "A: Reboot");
    }
}
