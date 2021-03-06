#include "cert_patches/cert_backup_strategy.hpp"
#include "cert_patches/cert_backup.hpp"
#include "cert_patches/cert_hashes.hpp"
#include "menu/menu_done.hpp"
#include "menu/menu_loading.hpp"
#include "menu/menu_miiverse_confirm.hpp"
#include "menu/menu.hpp"
#include "rpx_patches/rpx_backup_strategy.hpp"
#include "rpx_patches/rpx_backup.hpp"
#include "rpx_patches/rpx_hashes.hpp"
#include "rpx_patches/rpx_patch.hpp"
#include "util/log.h"
#include "util/copy_file.hpp"
#include "util/util.hpp"
#include "util/titles.hpp"
#include "util/iosu_fs.hpp"
#include "version.h"

#include <cstdio>
#include <array>
#include <map>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

#include <whb/log_console.h>
#include <whb/proc.h>
#include <coreinit/mcp.h>
#include <vpad/input.h>

#include <romfs-wiiu.h>

extern "C" {
    //remove when wut adds it
    void OSShutdown();
    void FSAInit();
    int FSAAddClient(void* asyncAttachData);
    int FSAFlushVolume(int handle, const char* path);
    int FSADelClient(int handle);
}

static constexpr int task_percent(int task) { return (task*100)/19; };

int main(int argc, char** argv) {
    int ret;
    bool bret;
    int task = 0;

    LOGInit();
    ramfsInit();
    //WHBLogConsoleInit();
    WHBProcInit();
    OnLeavingScope _log_c([&] {
        WHBProcShutdown();
        //WHBLogConsoleFree();
        ramfsExit();
        LOGShutdown();
    });

    FSAInit();
    int fsaHandle = FSAAddClient(NULL);
    printf("FSA handle: %08x\n", fsaHandle);
    OnLeavingScope _fsa_c([&] {
        if (!(fsaHandle < 0)) {
            FSADelClient(fsaHandle);
            fsaHandle = 0;
        }
    });

    bret = InitMenu();
    if (!bret) {
        printf("failed to set up menu!\n");
        return -1;
    }
    OnLeavingScope _mnu_c([&] { QuitMenu(); });

    printf(APP_NAME " " APP_VERSION "\n");

    RenderMenuLoading(task_percent(task++), "Finding Miiverse title...");
    PresentMenu();

    std::filesystem::path miiverse_path;
    /* Haxchi requires MCP functions to be kept somewhat isolated, so we get
     * them all out of the way early before we set up iosuhax. */
    {
        auto mcp = MCP_Open();
        if (mcp < 0) {
            printf("couldn't open MCP: %d\n", mcp);
            while (WHBProcIsRunning()) { RenderMenuDone(MENU_DONE_MCP_FAIL); PresentMenu(); }
            return -1;
        }
        OnLeavingScope _mcp_c([&] {
            MCP_Close(mcp);
            mcp = -1;
        });

        miiverse_path = GetTitlePath(mcp, MCP_APP_TYPE_MIIVERSE);
        if (miiverse_path.empty()) {
            printf("couldn't find Miiverse title!\n");
            while (WHBProcIsRunning()) { RenderMenuDone(MENU_DONE_NO_MIIVERSE); PresentMenu(); }
            return -1;
        }
        printf("miiverse found at %s\n", miiverse_path.c_str());
    }
    /* No more MCP functions from here on out. OK to set up iosuhax. */

    RenderMenuLoading(task_percent(task++), "Checking CFW...");
    PresentMenu();

    ret = iosu_fs_setup();
    if (ret < 0) {
        printf("failed to set up iosuhax!\n");
        while (WHBProcIsRunning()) { RenderMenuDone(MENU_DONE_NO_CFW); PresentMenu(); }
        return -1;
    }
    OnLeavingScope _ios_c([&] {
        iosu_fs_stop();
    });

    //goal: get a state together; ready to show a menu or something
    //https://marcan.st/2011/01/safe-hacking/

    std::filesystem::path wave_path = miiverse_path/"code/wave.rpx";
    std::filesystem::path wave_bak_path = miiverse_path/"code/wave.rpx.orig";
    std::filesystem::path wave_patched_path = miiverse_path/"code/wave.rpx.patched";

    std::filesystem::path pn_olv_path = miiverse_path/"code/pn_olv.rpl";
    std::filesystem::path nn_olv_path = "iosu:/vol/system_slc/title/00050010/1000400a/code/nn_olv.rpl";
    std::filesystem::path pn_olv_patched_path = miiverse_path/"code/pn_olv.rpl.patched";

    std::filesystem::path cert_path = "iosu:/vol/storage_mlc01/sys/title/0005001b/10054000/content/scerts/THAWTE_PREMIUM_SERVER_CA.der";
    std::filesystem::path cert_bak_path = "iosu:/vol/storage_mlc01/sys/title/0005001b/10054000/content/scerts/THAWTE_PREMIUM_SERVER_CA.der.orig";
    std::filesystem::path cert_patched_path = "resin:/pkcazroot.der";

    std::error_code fserr;

    RenderMenuLoading(task_percent(task++), "Examining current Miiverse state... (wave.rpx)");
    PresentMenu();

    BackupState wave_state = GetBackupStrategy(wave_path, wave_bak_path);
    if (!wave_state.strategy) {
        //couldn't come up with a way to continue
        printf("Could not find a suitable backup strategy!\n");
        while (WHBProcIsRunning()) { RenderMenuDone(MENU_DONE_NO_BACKUP); PresentMenu(); }
        return -1;
    }
    BackupStrategy wave_strategy = *wave_state.strategy;

    RenderMenuLoading(task_percent(task++), "Examining current Miiverse state... (nn_olv.rpl)");
    PresentMenu();

    //we can be a bit less careful with nn_olv, since we always have the stock file on hand
    //if the stock file is bad, the patcher should catch it
    BackupStrategy olv_strategy = {
        .need_confirmation = false,
        .backup_action = B_DO_NOTHING,
        .patch_action = P_PATCH_FROM_BACKUP,
    };

    RenderMenuLoading(task_percent(task++), "Examining current Miiverse state... (NSSL cert)");
    PresentMenu();

    //certs use a different strategy, since we're always overwriting from resin
    auto cert_strategy_o = GetCertBackupStrategy(cert_path, cert_bak_path);
    if (!cert_strategy_o) {
        //couldn't continue
        printf("Could not find a suitable certificate backup strategy!\n");
        while (WHBProcIsRunning()) { RenderMenuDone(MENU_DONE_NO_CERT_BACKUP); PresentMenu(); }
        return -1;
    }
    BackupStrategy cert_strategy = *cert_strategy_o;

    bool proc = true;
    while ((proc = WHBProcIsRunning())) {
        VPADStatus vpad;
        VPADReadError error;
        VPADRead(VPAD_CHAN_0, &vpad, 1, &error);
        if (error == VPAD_READ_SUCCESS) {
            if (vpad.trigger & VPAD_BUTTON_A) break;
        }
        RenderMenuMiiverseConfirm(wave_state);
        PresentMenu();
    }

    if (!proc) {
        //loop broke because user quit from menu overlay
        return 0;
    }
    //otherwise we can continue

    bool error = false;

    RenderMenuLoading(task_percent(task++), "Creating Miiverse backup... (wave.rpx)");
    PresentMenu();

    bret = backup_rpx(wave_strategy.backup_action, wave_path, wave_bak_path);
    if (!bret) {
        printf("Failed to backup wave!\n");
        wave_strategy.patch_action = P_DO_NOTHING;
        while (WHBProcIsRunning()) { RenderMenuDone(MENU_DONE_BACKUP_FAIL); PresentMenu(); }
        return -1;
    }

    RenderMenuLoading(task_percent(task++), "Creating Miiverse backup... (pn_olv.rpl)");
    PresentMenu();

    bret = backup_rpx(olv_strategy.backup_action, pn_olv_path, nn_olv_path);
    if (!bret) {
        printf("Failed to backup olv!\n");
        olv_strategy.patch_action = P_DO_NOTHING;
        while (WHBProcIsRunning()) { RenderMenuDone(MENU_DONE_BACKUP_FAIL); PresentMenu(); }
        return -1;
    }

    RenderMenuLoading(task_percent(task++), "Creating Miiverse backup... (NSSL cert)");
    PresentMenu();

    bret = backup_cert(cert_strategy.backup_action, cert_path, cert_bak_path);
    if (!bret) {
        printf("Failed to backup cert!\n");
        cert_strategy.patch_action = P_DO_NOTHING;
        while (WHBProcIsRunning()) { RenderMenuDone(MENU_DONE_BACKUP_FAIL); PresentMenu(); }
        return -1;
    }

    RenderMenuLoading(task_percent(task++), "Applying Miiverse patches... (wave.rpx)");
    PresentMenu();

    bret = patch_rpx(wave_strategy.patch_action, wave_path, wave_bak_path, wave_patched_path, "resin:/patches/wave.d.v113.p1.bps");
    if (!bret) {
        printf("Failed to patch wave!\n");
        while (WHBProcIsRunning()) { RenderMenuDone(MENU_DONE_PATCH_FAIL); PresentMenu(); }
        return -1;
    }

    RenderMenuLoading(task_percent(task++), "Applying Miiverse patches... (nn_olv.rpl)");
    PresentMenu();

    //DANGER: getting argument order wrong here could cause brickable writes to OSv10!
    bret = patch_rpx(olv_strategy.patch_action, pn_olv_path, nn_olv_path, pn_olv_patched_path, "resin:/patches/nn_olv.d.v15072.p1.bps");
    if (!bret) {
        printf("Failed to patch olv!\n");
        while (WHBProcIsRunning()) { RenderMenuDone(MENU_DONE_PATCH_FAIL); PresentMenu(); }
        return -1;
    }

    RenderMenuLoading(task_percent(task++), "Verifying patched files... (wave.rpx)");
    PresentMenu();

    {
        std::ifstream is(wave_patched_path, std::ios::binary);
        auto hash = rpx_hash(is);
        if (hash.id != CURRENT_PRETENDO_WAVE) {
            printf("Patcher made a corrupt file!\n");
            while (WHBProcIsRunning()) { RenderMenuDone(MENU_DONE_PATCH_BAD); PresentMenu(); }
            return -1;
        }
    }

    RenderMenuLoading(task_percent(task++), "Verifying patched files... (nn_olv.rpl)");
    PresentMenu();

    {
        std::ifstream is(pn_olv_patched_path, std::ios::binary);
        auto hash = rpx_hash(is);
        if (hash.id != CURRENT_PRETENDO_NN_OLV) {
            printf("Patcher made a corrupt file!\n");
            while (WHBProcIsRunning()) { RenderMenuDone(MENU_DONE_PATCH_BAD); PresentMenu(); }
            return -1;
        }
    }

    RenderMenuLoading(task_percent(task++), "Verifying patched files... (NSSL cert)");
    PresentMenu();

    {
        std::ifstream is(cert_patched_path, std::ios::binary);
        auto hash = cert_hash(is);
        if (hash.id != CURRENT_PRETENDO_CERT) {
            printf("New SSL cert is corrupt!\n");
            while (WHBProcIsRunning()) { RenderMenuDone(MENU_DONE_PATCH_BAD); PresentMenu(); }
            return -1;
        }
    }

    //the point of no return
    RenderMenuLoading(task_percent(task++), "Committing changes... (wave.rpx)");
    PresentMenu();

    bret = fast_copy_file(wave_patched_path, wave_path);
    if (!bret) {
        printf("Final file copy failed!\n");
        //the next step is verification. on the off chance the copy 100% failed
        //and the stock wave is still there, we could still quit gracefully, so
        //we don't hard-fail here.
    }

    RenderMenuLoading(task_percent(task++), "Committing changes... (nn_olv.rpl)");
    PresentMenu();

    bret = fast_copy_file(pn_olv_patched_path, pn_olv_path);
    if (!bret) {
        printf("Final olv file copy failed!\n");
    }

    RenderMenuLoading(task_percent(task++), "Committing changes... (NSSL cert)");
    PresentMenu();

    bret = fast_copy_file(cert_patched_path, cert_path);
    if (!bret) {
        printf("Final olv file copy failed!\n");
    }

    RenderMenuLoading(task_percent(task++), "Verifying final patches... (wave.rpx)");
    PresentMenu();

    {
        std::ifstream is(wave_path, std::ios::binary);
        auto hash = rpx_hash(is);
        if (hash.patch == RPX_PATCH_STATE_STOCK) {
            printf("Failed to commit patches - stock wave in place\n");
            while (WHBProcIsRunning()) { RenderMenuDone(MENU_DONE_PATCH_FAIL); PresentMenu(); }
            return -1;
        } else if (hash.id != CURRENT_PRETENDO_WAVE) {
            printf("Failed to commit patches - wave corrupt!\n");
            while (WHBProcIsRunning()) { RenderMenuDone(MENU_DONE_PATCH_FAIL_DANGEROUS); PresentMenu(); }
            return -1;
        }
    }

    RenderMenuLoading(task_percent(task++), "Verifying final patches... (nn_olv.rpl)");
    PresentMenu();

    {
        std::ifstream is(pn_olv_path, std::ios::binary);
        auto hash = rpx_hash(is);
        if (hash.id != CURRENT_PRETENDO_NN_OLV) {
            //we've already modified wave, so this is always dangerous
            printf("Failed to commit patches - pn_olv corrupt!\n");
            while (WHBProcIsRunning()) { RenderMenuDone(MENU_DONE_PATCH_FAIL_DANGEROUS); PresentMenu(); }
            return -1;
        }
    }

    RenderMenuLoading(task_percent(task++), "Verifying final patches... (NSSL cert)");
    PresentMenu();

    {
        std::ifstream is(cert_path, std::ios::binary);
        auto hash = cert_hash(is);
        if (hash.id != CURRENT_PRETENDO_CERT) {
            //we've already modified wave, so this is always dangerous
            printf("Failed to commit patches - nssl cert corrupt!\n");
            //oh shit, this could brick - restore backup
            bret = fast_copy_file(cert_bak_path, cert_path);
            if (!bret) {
                printf("Reverting nssl patches failed!\n");
                while (WHBProcIsRunning()) { RenderMenuDone(MENU_DONE_PATCH_FAIL_CERT_BRICK); PresentMenu(); }
                return -1;
            }

            std::ifstream is2(cert_path, std::ios::binary);
            auto hash2 = cert_hash(is2);
            if (hash2.patch != CERT_PATCH_STATE_STOCK) {
                printf("Reverting nssl patches failed!\n");
                while (WHBProcIsRunning()) { RenderMenuDone(MENU_DONE_PATCH_FAIL_CERT_BRICK); PresentMenu(); }
                return -1;
            } else if (hash2.id != CURRENT_SACRIFICIAL_CERT) {
                printf("Reverting NSSL cert resulted in the wrong cert?\n");
                while (WHBProcIsRunning()) { RenderMenuDone(MENU_DONE_PATCH_FAIL_DANGEROUS); PresentMenu(); }
                return -1;
            }

            while (WHBProcIsRunning()) { RenderMenuDone(MENU_DONE_PATCH_FAIL_DANGEROUS); PresentMenu(); }
            return -1;
        }
    }

    RenderMenuLoading(task_percent(task++), "Flushing volumes...");
    PresentMenu();

    ret = FSAFlushVolume(fsaHandle, "/vol/storage_mlc01");
    printf("FSAFlushVolume: %08x\n", ret);

    //woo!
    while (true) {
        VPADStatus vpad;
        VPADReadError error;
        VPADRead(VPAD_CHAN_0, &vpad, 1, &error);
        if (error == VPAD_READ_SUCCESS) {
            if (vpad.trigger & VPAD_BUTTON_A) break;
        }
        RenderMenuDone(MENU_DONE_NO_ERROR);
        PresentMenu();
    }

    printf("shutting down\n");

    OSShutdown();
    while (WHBProcIsRunning()) {}

    return 0;
}
