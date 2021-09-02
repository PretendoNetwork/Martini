# Martini - Miiverse applet patcher

[![Pretendo network logo](https://camo.githubusercontent.com/b98196c2347796d66fa669e8510749665ec80ba0/68747470733a2f2f692e696d6775722e636f6d2f5839346237574b2e706e67)](https://pretendo.network)

Martini lets you patch a Wii U's Miiverse/wave applet to use [Juxtaposition](https://github.com/PretendoNetwork/juxt-web), Pretendo's replacement social networking service. Note that the modifications Martini makes are "permanent" - they can be reverted, but unlike our other patches, they write to a console's NAND and persist across reboots.

## Dependencies

The recommended configuration for Martini is a console with CBHC.

Mocha CFW or a compatible `/dev/iosuhax` CFW is required at installation time - notably CBHC's HaxchiFW does *not* meet this requirement, though CBHC can be configured to boot Mocha. After patching is complete, any CFW with signature patching should allow the Juxt applet to run - though CBHC is strongly recommended for brick protection, see [Safety](#Safety) below.

### Build Dependencies

 - [**wut**](https://github.com/devkitPro/wut/)
 - **devkitPPC**
 - **ppc-zlib** - available from devkitPro pacman

We suggest using Docker to compile.

## Patch strategy
The current version of Martini will:
 - Find, decompress, and BPS patch `nn_olv.rpl`, before recompressing and writing it into the Miiverse applet's code directory as `pn_olv.rpl` - changing the API and discovery URLs to `pretendo.cc`,
 - Overwrite an expired system CA certificate (`THWATE_PREMIUM_SERVER_CA.der`) with DST Root X3, for Let's Encrypt,
 - Find, decompress, BPS patch, recompress and replace the Miiverse applet's `wave.rpx` - changing the built-in domain whitelist to include `pretendo.cc`, changing the `nn_olv` import to `pn_olv`, and adding Thwate Premium Server (now overwritten with DST Root X3) as a trusted CA.

While the modifications of system titles on NAND is regrettable, at the time of writing it's the best option available for applying patches to an overlay applet like Miiverse. The inclusion of a new CA also allows SSL to remain enabled and enforced for Juxtaposition - the applet and SSL module expect a valid certificate for `pretendo.cc` after the URL patches.

(Note that the account server patches currently disable SSL systemwide, including for Juxt - please see [Nimble's readme](https://github.com/PretendoNetwork/Nimble) for more information.)

## Safety
Martini is extremely paranoid about the safety of your console, with hash checks upon hash checks for each stage of each file's patching, only writing to sensitive locations on NAND once it is completely satisfied with the result. Unfortunately, code does still have bugs, and despite our best efforts this patching approach does bring some level of risk.

While writing to the Miiverse applet appears to be safe, with the worst situation being a Miiverse applet that fails to open, Martini also writes to the system CA certificate folder. If done improperly, this **can brick a console** by causing the Wii U Menu to freeze. Martini will attempt to detect and recover from this condition, but if it fails, **the only known way to recover is with CBHC**. For this reason, it is strongly recommended that any non-technical users of this patcher have CBHC installed on their console. The Aroma beta should also allow for recovery from this type of softbrick, though no cases have been recorded yet.

As of September 2021, no consoles are known to have been bricked by using Martini in its default configuration.

## TODO
See [Issues](https://github.com/PretendoNetwork/Martini/issues).
