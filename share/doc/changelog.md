# HexChat ChangeLog


## 2.9.4 (2012-11-10)

 * fix alerts when omit alerts in away option is set
 * fix dialog icon in userlist popup
 * fix opening links on Mac
 * fix default network in the Network List
 * fix initial folder in file dialogs
 * fix positioning the nick change dialog
 * fix error message for busy servers
 * fix filename encoding errors
 * fix Fedora spec file
 * fix Raw Log content being impossible to copy when auto-copy is disabled
 * fix rough icon rendering in most windows on Windows
 * fix config folder when specified with -d argument
 * add built-in support for SASL authentication via CAP
 * add support for identify-msg/multi-prefix server capabilities
 * add text events for CAP related messages
 * add support for the SysInfo plugin on Unix
 * add option to change update check frequency and delay for first check
 * add option to change GUI language on Windows
 * add Ignore entry to userlist popup
 * add Afrikaans, Asturian, Danish, Gujarati, Indonesian, Kinyarwanda and Malayalam translations
 * add ChangeLog and ReadMe links to Start Menu during installation on Windows
 * add manual page on Unix
 * add icon support for 3 levels above op user mode
 * change default colors, text events and user list/channel tree icons
 * make Esc key close the Raw Log window
 * use Consolas as the default font where available
 * open dialog window for double-clicking in the user list by default
 * variable separation, cleanup and renaming
 * check in the installers whether Windows release is supported by HexChat
 * display previous value after /SET
 * reorganize the Settings menu and add new options
 * redesign the About dialog
 * show certain help messages in GTK+ dialogs instead of command line
 * build system cosmetics on Unix
 * reorganize repo file structure
 * rebranding
 * update translations
 * update the network list


## 2.9.3 (2012-10-14)

 * fix various URL detection bugs
 * fix default folders for file transfers in portable mode
 * fix Autotools warnings with recent releases
 * add /ADDSERVER command
 * add option to save URLs to disk on-the-fly
 * add option to omit alerts when marked as being away
 * add default icons for channel tree and option to turn them off
 * change certain default colors
 * enhance Non-BMP filtering performance
 * accept license agreement by default on Windows
 * update the network list


## 2.9.2 (2012-10-05)

 * fix compilation on Red Hat and Fedora
 * fix portable to non-portable migrations on Windows
 * fix ban message in HexTray
 * fix icon in Connection Complete dialog
 * fix determining if the log folder path is full or relative
 * fix desktop notification icons on Unix
 * fix URL grabber saving an unlimited number of URLs by default
 * fix URL grabber memory leaks under certain circumstances
 * fix URL grabber trying to export URL lists to system folders by default
 * fix opening URLs without http(s)://
 * add support for regenerating text events during compilation on Windows
 * add support for the theme manager on Unix
 * add Unifont to the default list of alternative fonts
 * add option to retain colors in the topic
 * allow the installer to preserve custom GTK+ theme settings on Windows
 * use the icons subfolder of the config folder for loading custom icons
 * use port 6697 for SSL connections by default
 * install the SASL plugin by default on Windows
 * /lastlog improvements
 * build system cosmetics on Unix
 * open links with just left click by default
 * enable timestamps and include seconds by default
 * make libproxy an optional dependency on Unix
 * update German translation
 * update the network list


## 2.9.1 (2012-07-27)

 * fix installing/loading plugins on Unix
 * fix restoring the HexChat window via shortcuts on Windows
 * fix HexTray icon rendering for certain events
 * fix the Show marker line option in Preferences
 * fix /lastlog regexp support on Windows
 * add support for the Checksum, Do At, FiSHLiM and SASL plugins on Unix
 * add option to retain colors when displaying scrollback
 * add MS Gothic to the default list of alternative fonts
 * rebranding and cleanup
 * eliminate lots of compiler warnings
 * Unix build system fixes and cosmetics
 * make Git ignore Unix-specific intermediate files
 * use better compression for Windows installers
 * switch to GTK+ file dialogs on Windows
 * restructure the Preferences window
 * use the addons subfolder of the config folder for auto-loading plugins/scripts
 * improve the dialog used for opening plugins/scripts
 * remember user limits in channel list between sessions
 * remember last search pattern during sessions
 * update XChat to r1521


## 2.9.0 (2012-07-14)

 * rebranding
 * migrate code to GitHub
 * update XChat to r1515
 * fix x64 Perl interface installation for Perl 5.16
 * improve URL detection with new TLDs and file extensions


## 1508-3 (2012-06-17)

 * add XChat Theme Manager
 * fix problems with Turkish locale


## 1508-2 (2012-06-15)

 * add support for Perl 5.16
 * update Do At plugin
 * fix drawing of chat area bottom
 * avoid false hits when restoring from tray via shortcut
 * migrate from NMAKE to Visual Studio


## 1508 (2012-06-02)

 * remove Real Name from Network List
 * search window improvements
 * restore XChat-WDK from tray via shortcut if X-Tray is used


## 1507 (2012-05-13)

 * update OpenSSL to 1.0.1c
 * FiSHLiM updates


## 1506 (2012-05-04)

 * update OpenSSL to 1.0.1b
 * update German translation


## 1503 (2012-03-16)

 * update OpenSSL to 1.0.1
 * URL grabber updates
 * FiSHLiM updates


## 1500 (2012-02-16)

 * add option for specifying alternative fonts
 * fix crash due to invalid timestamp format
 * X-Tray cosmetics


## 1499-7 (2012-02-08)

 * fix update notifications
 * fix compilation on Linux
 * add IPv6 support to built-in identd


## 1499-6 (2012-01-20)

 * add DNS plugin


## 1499-5 (2012-01-20)

 * built-in fix for client crashes
 * update OpenSSL to 1.0.0g


## 1499-4 (2012-01-18)

 * add Non-BMP plugin to avoid client crashes


## 1499-3 (2012-01-15)

 * rework and extend plugin config API
 * add ADD/DEL/LIST support to X-SASL


## 1499-2 (2012-01-11)

 * add X-SASL plugin


## 1499 (2012-01-09)

 * fix saving FiSHLiM keys
 * update OpenSSL to 1.0.0f


## 1498-4 (2011-12-05)

 * fix updates not overwriting old files
 * display WinSys output in one line for others
 * use Strawberry Perl for building


## 1498-3 (2011-12-02)

 * add plugin config API
 * add Exec plugin
 * add WinSys plugin
 * perform periodic update checks automatically


## 1498-2 (2011-11-25)

 * add FiSHLiM plugin
 * add option to allow only one instance of XChat to run


## 1498 (2011-11-23)

 * separate x86 and x64 installers (uninstall any previous version!)
 * downgrade GTK+ to 2.16
 * re-enable the transparent background option
 * various X-Tray improvements
 * add WMPA plugin
 * add Do At plugin
 * automatically save set variables to disk by default
 * update OpenSSL to 1.0.0e


## 1496-6 (2011-08-09)

 * add option to auto-open new tab upon /msg
 * fix the update checker to use the git repo
 * disable update checker cache 


## 1496-5 (2011-08-07)

 * fix attach/detach keyboard shortcut
 * add multi-language support to the spell checker 


## 1496-4 (2011-07-27)

 * recognize Windows 8 when displaying OS info
 * update OpenSSL certificate list
 * fix X-Tray blinking on unselected events
 * fix X-Tray keyboard shortcut handling
 * cease support for Perl 5.10
 * use Strawberry Perl for 5.12 DLLs 


## 1496-3 (2011-06-16)

 * add option for changing spell checker color 


## 1496-2 (2011-06-05)

 * add support for custom license text 


## 1496 (2011-05-30)

 * display build type in CTPC VERSION reply
 * add support for Perl 5.14 


## 1494 (2011-04-16)

 * update Visual Studio to 2010 SP1
 * update OpenSSL to 1.0.0d
 * ship MySpell dictionaries in a separate installer 


## 1489 (2011-01-26)

 * fix unloading the Winamp plugin
 * enable the Favorite Networks feature
 * add Channel Message event support to X-Tray
 * add mpcInfo plugin 


## 1486 (2011-01-16)

 * fix a possible memory leak in the update checker
 * fix XChat-Text shortcut creation
 * fix XChat version check via the plugin interface
 * add option for limiting the size of files to be checksummed
 * add X-Tray as an install option
 * disable Plugin-Tray context menu completely 


## 1479-2 (2011-01-10)

 * improve command-line argument support
 * add auto-copy options
 * enable XChat-Text
 * disable faulty tray menu items 


## 1479 (2010-12-29)

 * update GTK+ to 2.22.1
 * update OpenSSL to 1.0.0c
 * update Python to 2.7.1
 * replace X-Tray with Plugin-Tray 


## 1469-3 (2010-10-20)

 * add Checksum plugin
 * menu integration for Update Checker and Winamp 


## 1469-2 (2010-10-09)

 * fix DCC file sending
 * native open/save dialogs
 * make the version info nicer
 * register XChat-WDK as IRC protocol handler
 * add option to run XChat-WDK after installation
 * disable erroneous uninstall warnings
 * disable Plugin-Tray, provide X-Tray only
 * cease support for Perl 5.8
 * replace EasyWinampControl with Winamp 


## 1469 (2010-10-08)

 * use Visual C++ 2010 for all WDK builds
 * build Enchant with WDK and update it to 1.6.0
 * fix SSL validation
 * fix opening the config folder from GUI in portable mode
 * further improve dialog placement for closing network tabs 


## 1468-2 (2010-10-02)

 * update GTK+ to 2.22
 * spelling support
 * more config compatibility with official build
 * improve dialog placement for closing network tabs
 * remove themes from the installer
 * disable toggle for favorite networks until it's usable
 * disable transparent backgrounds
 * hide mnemonic underlines until Alt key pressed
 * fix XP lagometer and throttlemeter rendering 


## 1468 (2010-09-19)

 * update Perl to 5.12.2
 * update Tcl to 8.5.9
 * fix scrollback shrinking
 * enable advanced settings pane
 * retain emoticon settings
 * add /IGNALL command 


## 1464-6 (2010-09-06)

 * fix Perl interface breakage
 * update checker plugin 


## 1464-5 (2010-08-30)

 * primitive update checker 


## 1464-4 (2010-08-30)

 * selectable tray icon
 * selectable theme for portable
 * selectable plugins 


## 1464-3 (2010-08-29)

 * black theme for portable 


## 1464-2 (2010-08-29)

 * make Perl version selectable during install 


## 1464 (2010-08-26)

 * Perl interface updates 


## 1462 (2010-08-25)

 * update XChat to r1462
 * build system cleanup 


## 1459-3 (2010-08-23)

 * more installer changes (uninstall any previous version!) 


## 1459-2 (2010-08-23)

 * universal installer
 * update build dependencies 


## 1459 (2010-08-19)

 * portable mode and installer fixes 


## 1457 (2010-08-17)

 * disable GUI warnings 


## 1455-2 (2010-08-17)

 * unified installer for standard and portable 


## 1455 (2010-08-15)

 * support for gtkwin_ptr in the Perl interface 


## 1454 (2010-08-14)

 * gtkwin_ptr for plugins introduced 


## 1452 (2010-08-14)

 * fix taskbar alerts on x86
 * upgrade Perl to 5.12 and make 5.8/5.10 builds available separately 


## 1451-6 (2010-08-12)

 * include Lua-WDK with the installer 


## 1451-5 (2010-08-12)

 * switch to Inno Setup (uninstall any previous version!)
 * add Lua support 


## 1451-4 (2010-08-11)

 * enable the XDCC plugin 


## 1451-3 (2010-08-11)

 * enable Python support 


## 1451-2 (2010-08-11)

 * enable SSL support
 * fix simultaneous connections
 * re-enable identd by default 


## 1451 (2010-08-10)

 * update XChat to r1451
 * disable identd by default
 * remove DNS plugin 


## 1444 (2010-07-30)

 * update XChat to r1444
 * downgrade Tcl to 8.5
 * add Tcl support to the x64 build 


## 1441 (2010-06-15)

 * update XChat to r1441
 * enable transfer of files bigger than 4 GB 


## 1439 (2010-05-30)

 * update XChat to r1439 (2.8.8) 


## 1431-6 (2010-05-30)

 * re-enable the transparent background option
 * add branding to Plugin-Tray
 * installer updates 


## 1431-5 (2010-05-29)

 * fix installer
 * add DNS plugin status messages 


## 1431-4 (2010-05-28)

 * disable the transparent background option
 * downgrade GTK+ to more stable 2.16 


## 1431-3 (2010-05-23)

 * add portable build support 


## 1431-2 (2010-05-22)

 * replace X-Tray with Plugin-Tray 


## 1431 (2010-05-21)

 * update XChat to r1431
 * include a lot of XChat translations added since 2.8.6 


## 1412-3 (2010-05-02)

 * fix GTK function call 


## 1412-2 (2010-05-02)

 * re-enable taskbar alerts on x64 


## 1412 (2010-05-02)

 * update XChat to r1412
 * update GTK+ and friends
 * update Visual Studio to 2010
 * fix Perl warning message
 * include GTK L10n with the installer 


## 1409-9 (2010-04-18)

 * fix loading of scrollback 


## 1409-8 (2010-04-03)

 * fix X-Tray on x64 


## 1409-7 (2010-04-02)

 * disable taskbar notification options 


## 1409-6 (2010-03-31)

 * display version numbers everywhere 


## 1409-5 (2010-03-31)

 * add DNS plugin
 * add EasyWinampControl plugin
 * disable Plugin-Tray settings 


## 1409-4 (2010-03-30)

 * add X-Tray 


## 1409-3 (2010-03-29)

 * plugin linkage fixes 


## 1409-2 (2010-03-29)

 * enable IPv6 support
 * enable NLS support
 * enable Perl support
 * enable Tcl support 


## 1409 (2010-03-29)

 * initial release

