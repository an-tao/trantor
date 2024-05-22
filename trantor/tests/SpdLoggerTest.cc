#include <spdlog/spdlog.h>
#include <stdlib.h>
#include <thread>
#include <trantor/utils/Logger.h>

int main()
{
    trantor::Logger::enableSpdLog();
    trantor::Logger::enableSpdLog(5);
    int i;
    LOG_COMPACT_DEBUG << "Hello, world!";
    LOG_DEBUG << (float)3.14;
    LOG_DEBUG << (const char)'8';
    LOG_DEBUG << &i;
    LOG_DEBUG << (long double)3.1415;
    LOG_DEBUG << trantor::Fmt("%.3g", 3.1415926);
    LOG_DEBUG << "debug log!" << 1;
    LOG_TRACE << "trace log!" << 2;
    LOG_INFO << "info log!" << 3;
    LOG_WARN << "warning log!" << 4;
    if (1)
        LOG_ERROR << "error log!" << 5;
    std::thread thread_([]() {
        LOG_FATAL << "fatal log!" << 6;
    });

    FILE       *fp = fopen("/notexistfile", "rb");
    if (fp == NULL)
    {
        LOG_SYSERR << "syserr log!" << 7;
    }
    LOG_DEBUG << "long message test:";
    LOG_DEBUG
        << "Applications\n"
           "Developer\n"
           "Library\n"
           "Network\n"
           "System\n"
           "Users\n"
           "Volumes\n"
           "bin\n"
           "cores\n"
           "dev\n"
           "etc\n"
           "home\n"
           "installer.failurerequests\n"
           "net\n"
           "opt\n"
           "private\n"
           "sbin\n"
           "tmp\n"
           "usr\n"
           "var\n"
           "vm\n"
           "\n"
           "/Applications:\n"
           "Adobe\n"
           "Adobe Creative Cloud\n"
           "Adobe Photoshop CC\n"
           "AirPlayer Pro.app\n"
           "AliWangwang.app\n"
           "Android Studio.app\n"
           "App Store.app\n"
           "Autodesk\n"
           "Automator.app\n"
           "Axure RP Pro 7.0.app\n"
           "BaiduNetdisk_mac.app\n"
           "CLion.app\n"
           "Calculator.app\n"
           "Calendar.app\n"
           "Chess.app\n"
           "CleanApp.app\n"
           "Cocos\n"
           "Contacts.app\n"
           "DVD Player.app\n"
           "Dashboard.app\n"
           "Dictionary.app\n"
           "Docs for Xcode.app\n"
           "FaceTime.app\n"
           "FinalShell\n"
           "Firefox.app\n"
           "Font Book.app\n"
           "GitHub.app\n"
           "Google Chrome.app\n"
           "Image Capture.app\n"
           "Lantern.app\n"
           "Launchpad.app\n"
           "License.rtf\n"
           "MacPorts\n"
           "Mail.app\n"
           "Maps.app\n"
           "Messages.app\n"
           "Microsoft Excel.app\n"
           "Microsoft Office 2011\n"
           "Microsoft OneNote.app\n"
           "Microsoft Outlook.app\n"
           "Microsoft PowerPoint.app\n"
           "Microsoft Word.app\n"
           "Mindjet MindManager.app\n"
           "Mission Control.app\n"
           "Mockplus.app\n"
           "MyEclipse 2015\n"
           "Notes.app\n"
           "Numbers.app\n"
           "OmniGraffle.app\n"
           "Pages.app\n"
           "Photo Booth.app\n"
           "Photos.app\n"
           "Preview.app\n"
           "QJVPN.app\n"
           "QQ.app\n"
           "QQMusic.app\n"
           "QuickTime Player.app\n"
           "RAR Extractor Lite.app\n"
           "Reminders.app\n"
           "Remote Desktop Connection.app\n"
           "Renee Undeleter.app\n"
           "Sabaki.app\n"
           "Safari.app\n"
           "ShadowsocksX.app\n"
           "Siri.app\n"
           "SogouCharacterViewer.app\n"
           "SogouInputPad.app\n"
           "Stickies.app\n"
           "SupremePlayer Lite.app\n"
           "System Preferences.app\n"
           "TeX\n"
           "Telegram.app\n"
           "Telnet Lite.app\n"
           "Termius.app\n"
           "Tesumego - How to Make a Professional Go Player.app\n"
           "TextEdit.app\n"
           "Thunder.app\n"
           "Time Machine.app\n"
           "Tunnelblick.app\n"
           "Utilities\n"
           "VPN Shield.appdownload\n"
           "WeChat.app\n"
           "WinOnX2.app\n"
           "Wireshark.app\n"
           "Xcode.app\n"
           "Yose.app\n"
           "YoudaoNote.localized\n"
           "finalshelldata\n"
           "iBooks.app\n"
           "iHex.app\n"
           "iPhoto.app\n"
           "iTools.app\n"
           "iTunes.app\n"
           "pgAdmin 4.app\n"
           "vSSH Lite.app\n"
           "wechatwebdevtools.app\n"
           "\n"
           "/Applications/Adobe:\n"
           "Flash Player\n"
           "\n"
           "/Applications/Adobe/Flash Player:\n"
           "AddIns\n"
           "\n"
           "/Applications/Adobe/Flash Player/AddIns:\n"
           "airappinstaller\n"
           "\n"
           "/Applications/Adobe/Flash Player/AddIns/airappinstaller:\n"
           "airappinstaller\n"
           "digest.s\n"
           "\n"
           "/Applications/Adobe Creative Cloud:\n"
           "Adobe Creative Cloud\n"
           "Icon\n"
           "Uninstall Adobe Creative Cloud\n"
           "\n"
           "/Applications/Adobe Photoshop CC:\n"
           "Adobe Photoshop CC.app\n"
           "Configuration\n"
           "Icon\n"
           "Legal\n"
           "LegalNotices.pdf\n"
           "Locales\n"
           "Plug-ins\n"
           "Presets\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop CC.app:\n"
           "Contents\n"
           "Linguistics\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop CC.app/Contents:\n"
           "Application Data\n"
           "Frameworks\n"
           "Info.plist\n"
           "MacOS\n"
           "PkgInfo\n"
           "Required\n"
           "Resources\n"
           "_CodeSignature\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data:\n"
           "Custom File Info Panels\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info Panels:\n"
           "4.0\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info Panels/4.0:\n"
           "bin\n"
           "custom\n"
           "panels\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/bin:\n"
           "FileInfoFoundation.swf\n"
           "FileInfoUI.swf\n"
           "framework.swf\n"
           "loc\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/bin/loc:\n"
           "FileInfo_ar_AE.dat\n"
           "FileInfo_bg_BG.dat\n"
           "FileInfo_cs_CZ.dat\n"
           "FileInfo_da_DK.dat\n"
           "FileInfo_de_DE.dat\n"
           "FileInfo_el_GR.dat\n"
           "FileInfo_en_US.dat\n"
           "FileInfo_es_ES.dat\n"
           "FileInfo_et_EE.dat\n"
           "FileInfo_fi_FI.dat\n"
           "FileInfo_fr_FR.dat\n"
           "FileInfo_he_IL.dat\n"
           "FileInfo_hr_HR.dat\n"
           "FileInfo_hu_HU.dat\n"
           "FileInfo_it_IT.dat\n"
           "FileInfo_ja_JP.dat\n"
           "FileInfo_ko_KR.dat\n"
           "FileInfo_lt_LT.dat\n"
           "FileInfo_lv_LV.dat\n"
           "FileInfo_nb_NO.dat\n"
           "FileInfo_nl_NL.dat\n"
           "FileInfo_pl_PL.dat\n"
           "FileInfo_pt_BR.dat\n"
           "FileInfo_ro_RO.dat\n"
           "FileInfo_ru_RU.dat\n"
           "FileInfo_sk_SK.dat\n"
           "FileInfo_sl_SI.dat\n"
           "FileInfo_sv_SE.dat\n"
           "FileInfo_tr_TR.dat\n"
           "FileInfo_uk_UA.dat\n"
           "FileInfo_zh_CN.dat\n"
           "FileInfo_zh_TW.dat\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/custom:\n"
           "DICOM.xml\n"
           "Mobile.xml\n"
           "loc\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/custom/loc:\n"
           "DICOM_ar_AE.dat\n"
           "DICOM_bg_BG.dat\n"
           "DICOM_cs_CZ.dat\n"
           "DICOM_da_DK.dat\n"
           "DICOM_de_DE.dat\n"
           "DICOM_el_GR.dat\n"
           "DICOM_en_US.dat\n"
           "DICOM_es_ES.dat\n"
           "DICOM_et_EE.dat\n"
           "DICOM_fi_FI.dat\n"
           "DICOM_fr_FR.dat\n"
           "DICOM_he_IL.dat\n"
           "DICOM_hr_HR.dat\n"
           "DICOM_hu_HU.dat\n"
           "DICOM_it_IT.dat\n"
           "DICOM_ja_JP.dat\n"
           "DICOM_ko_KR.dat\n"
           "DICOM_lt_LT.dat\n"
           "DICOM_lv_LV.dat\n"
           "DICOM_nb_NO.dat\n"
           "DICOM_nl_NL.dat\n"
           "DICOM_pl_PL.dat\n"
           "DICOM_pt_BR.dat\n"
           "DICOM_ro_RO.dat\n"
           "DICOM_ru_RU.dat\n"
           "DICOM_sk_SK.dat\n"
           "DICOM_sl_SI.dat\n"
           "DICOM_sv_SE.dat\n"
           "DICOM_tr_TR.dat\n"
           "DICOM_uk_UA.dat\n"
           "DICOM_zh_CN.dat\n"
           "DICOM_zh_TW.dat\n"
           "Mobile_ar_AE.dat\n"
           "Mobile_bg_BG.dat\n"
           "Mobile_cs_CZ.dat\n"
           "Mobile_da_DK.dat\n"
           "Mobile_de_DE.dat\n"
           "Mobile_el_GR.dat\n"
           "Mobile_en_US.dat\n"
           "Mobile_es_ES.dat\n"
           "Mobile_et_EE.dat\n"
           "Mobile_fi_FI.dat\n"
           "Mobile_fr_FR.dat\n"
           "Mobile_he_IL.dat\n"
           "Mobile_hr_HR.dat\n"
           "Mobile_hu_HU.dat\n"
           "Mobile_it_IT.dat\n"
           "Mobile_ja_JP.dat\n"
           "Mobile_ko_KR.dat\n"
           "Mobile_lt_LT.dat\n"
           "Mobile_lv_LV.dat\n"
           "Mobile_nb_NO.dat\n"
           "Mobile_nl_NL.dat\n"
           "Mobile_pl_PL.dat\n"
           "Mobile_pt_BR.dat\n"
           "Mobile_ro_RO.dat\n"
           "Mobile_ru_RU.dat\n"
           "Mobile_sk_SK.dat\n"
           "Mobile_sl_SI.dat\n"
           "Mobile_sv_SE.dat\n"
           "Mobile_tr_TR.dat\n"
           "Mobile_uk_UA.dat\n"
           "Mobile_zh_CN.dat\n"
           "Mobile_zh_TW.dat\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels:\n"
           "IPTC\n"
           "IPTCExt\n"
           "advanced\n"
           "audioData\n"
           "camera\n"
           "categories\n"
           "description\n"
           "dicom\n"
           "gpsData\n"
           "history\n"
           "mobile\n"
           "origin\n"
           "rawpacket\n"
           "videoData\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/IPTC:\n"
           "bin\n"
           "loc\n"
           "manifest.xml\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/IPTC/bin:\n"
           "iptc.swf\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/IPTC/loc:\n"
           "IPTC_ar_AE.dat\n"
           "IPTC_bg_BG.dat\n"
           "IPTC_cs_CZ.dat\n"
           "IPTC_da_DK.dat\n"
           "IPTC_de_DE.dat\n"
           "IPTC_el_GR.dat\n"
           "IPTC_en_US.dat\n"
           "IPTC_es_ES.dat\n"
           "IPTC_et_EE.dat\n"
           "IPTC_fi_FI.dat\n"
           "IPTC_fr_FR.dat\n"
           "IPTC_he_IL.dat\n"
           "IPTC_hr_HR.dat\n"
           "IPTC_hu_HU.dat\n"
           "IPTC_it_IT.dat\n"
           "IPTC_ja_JP.dat\n"
           "IPTC_ko_KR.dat\n"
           "IPTC_lt_LT.dat\n"
           "IPTC_lv_LV.dat\n"
           "IPTC_nb_NO.dat\n"
           "IPTC_nl_NL.dat\n"
           "IPTC_pl_PL.dat\n"
           "IPTC_pt_BR.dat\n"
           "IPTC_ro_RO.dat\n"
           "IPTC_ru_RU.dat\n"
           "IPTC_sk_SK.dat\n"
           "IPTC_sl_SI.dat\n"
           "IPTC_sv_SE.dat\n"
           "IPTC_tr_TR.dat\n"
           "IPTC_uk_UA.dat\n"
           "IPTC_zh_CN.dat\n"
           "IPTC_zh_TW.dat\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/IPTCExt:\n"
           "bin\n"
           "loc\n"
           "manifest.xml\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/IPTCExt/bin:\n"
           "iptcExt.swf\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/IPTCExt/loc:\n"
           "IPTCExt_bg_BG.dat\n"
           "IPTCExt_cs_CZ.dat\n"
           "IPTCExt_da_DK.dat\n"
           "IPTCExt_de_DE.dat\n"
           "IPTCExt_en_US.dat\n"
           "IPTCExt_es_ES.dat\n"
           "IPTCExt_et_EE.dat\n"
           "IPTCExt_fi_FI.dat\n"
           "IPTCExt_fr_FR.dat\n"
           "IPTCExt_hr_HR.dat\n"
           "IPTCExt_hu_HU.dat\n"
           "IPTCExt_it_IT.dat\n"
           "IPTCExt_ja_JP.dat\n"
           "IPTCExt_ko_KR.dat\n"
           "IPTCExt_lt_LT.dat\n"
           "IPTCExt_lv_LV.dat\n"
           "IPTCExt_nb_NO.dat\n"
           "IPTCExt_nl_NL.dat\n"
           "IPTCExt_pl_PL.dat\n"
           "IPTCExt_pt_BR.dat\n"
           "IPTCExt_ro_RO.dat\n"
           "IPTCExt_ru_RU.dat\n"
           "IPTCExt_sk_SK.dat\n"
           "IPTCExt_sl_SI.dat\n"
           "IPTCExt_sv_SE.dat\n"
           "IPTCExt_tr_TR.dat\n"
           "IPTCExt_uk_UA.dat\n"
           "IPTCExt_zh_CN.dat\n"
           "IPTCExt_zh_TW.dat\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/advanced:\n"
           "bin\n"
           "loc\n"
           "manifest.xml\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/advanced/bin:\n"
           "advanced.swf\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/advanced/loc:\n"
           "Advanced_ar_AE.dat\n"
           "Advanced_bg_BG.dat\n"
           "Advanced_cs_CZ.dat\n"
           "Advanced_da_DK.dat\n"
           "Advanced_de_DE.dat\n"
           "Advanced_el_GR.dat\n"
           "Advanced_en_US.dat\n"
           "Advanced_es_ES.dat\n"
           "Advanced_et_EE.dat\n"
           "Advanced_fi_FI.dat\n"
           "Advanced_fr_FR.dat\n"
           "Advanced_he_IL.dat\n"
           "Advanced_hr_HR.dat\n"
           "Advanced_hu_HU.dat\n"
           "Advanced_it_IT.dat\n"
           "Advanced_ja_JP.dat\n"
           "Advanced_ko_KR.dat\n"
           "Advanced_lt_LT.dat\n"
           "Advanced_lv_LV.dat\n"
           "Advanced_nb_NO.dat\n"
           "Advanced_nl_NL.dat\n"
           "Advanced_pl_PL.dat\n"
           "Advanced_pt_BR.dat\n"
           "Advanced_ro_RO.dat\n"
           "Advanced_ru_RU.dat\n"
           "Advanced_sk_SK.dat\n"
           "Advanced_sl_SI.dat\n"
           "Advanced_sv_SE.dat\n"
           "Advanced_tr_TR.dat\n"
           "Advanced_uk_UA.dat\n"
           "Advanced_zh_CN.dat\n"
           "Advanced_zh_TW.dat\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/audioData:\n"
           "bin\n"
           "loc\n"
           "manifest.xml\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/audioData/bin:\n"
           "audioData.swf\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/audioData/loc:\n"
           "AudioData_ar_AE.dat\n"
           "AudioData_bg_BG.dat\n"
           "AudioData_cs_CZ.dat\n"
           "AudioData_da_DK.dat\n"
           "AudioData_de_DE.dat\n"
           "AudioData_el_GR.dat\n"
           "AudioData_en_US.dat\n"
           "AudioData_es_ES.dat\n"
           "AudioData_et_EE.dat\n"
           "AudioData_fi_FI.dat\n"
           "AudioData_fr_FR.dat\n"
           "AudioData_he_IL.dat\n"
           "AudioData_hr_HR.dat\n"
           "AudioData_hu_HU.dat\n"
           "AudioData_it_IT.dat\n"
           "AudioData_ja_JP.dat\n"
           "AudioData_ko_KR.dat\n"
           "AudioData_lt_LT.dat\n"
           "AudioData_lv_LV.dat\n"
           "AudioData_nb_NO.dat\n"
           "AudioData_nl_NL.dat\n"
           "AudioData_pl_PL.dat\n"
           "AudioData_pt_BR.dat\n"
           "AudioData_ro_RO.dat\n"
           "AudioData_ru_RU.dat\n"
           "AudioData_sk_SK.dat\n"
           "AudioData_sl_SI.dat\n"
           "AudioData_sv_SE.dat\n"
           "AudioData_tr_TR.dat\n"
           "AudioData_uk_UA.dat\n"
           "AudioData_zh_CN.dat\n"
           "AudioData_zh_TW.dat\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/camera:\n"
           "bin\n"
           "loc\n"
           "manifest.xml\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/camera/bin:\n"
           "camera.swf\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/camera/loc:\n"
           "Camera_ar_AE.dat\n"
           "Camera_bg_BG.dat\n"
           "Camera_cs_CZ.dat\n"
           "Camera_da_DK.dat\n"
           "Camera_de_DE.dat\n"
           "Camera_el_GR.dat\n"
           "Camera_en_US.dat\n"
           "Camera_es_ES.dat\n"
           "Camera_et_EE.dat\n"
           "Camera_fi_FI.dat\n"
           "Camera_fr_FR.dat\n"
           "Camera_he_IL.dat\n"
           "Camera_hr_HR.dat\n"
           "Camera_hu_HU.dat\n"
           "Camera_it_IT.dat\n"
           "Camera_ja_JP.dat\n"
           "Camera_ko_KR.dat\n"
           "Camera_lt_LT.dat\n"
           "Camera_lv_LV.dat\n"
           "Camera_nb_NO.dat\n"
           "Camera_nl_NL.dat\n"
           "Camera_pl_PL.dat\n"
           "Camera_pt_BR.dat\n"
           "Camera_ro_RO.dat\n"
           "Camera_ru_RU.dat\n"
           "Camera_sk_SK.dat\n"
           "Camera_sl_SI.dat\n"
           "Camera_sv_SE.dat\n"
           "Camera_tr_TR.dat\n"
           "Camera_uk_UA.dat\n"
           "Camera_zh_CN.dat\n"
           "Camera_zh_TW.dat\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/categories:\n"
           "bin\n"
           "loc\n"
           "manifest.xml\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/categories/bin:\n"
           "categories.swf\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/categories/loc:\n"
           "Categories_ar_AE.dat\n"
           "Categories_bg_BG.dat\n"
           "Categories_cs_CZ.dat\n"
           "Categories_da_DK.dat\n"
           "Categories_de_DE.dat\n"
           "Categories_el_GR.dat\n"
           "Categories_en_US.dat\n"
           "Categories_es_ES.dat\n"
           "Categories_et_EE.dat\n"
           "Categories_fi_FI.dat\n"
           "Categories_fr_FR.dat\n"
           "Categories_he_IL.dat\n"
           "Categories_hr_HR.dat\n"
           "Categories_hu_HU.dat\n"
           "Categories_it_IT.dat\n"
           "Categories_ja_JP.dat\n"
           "Categories_ko_KR.dat\n"
           "Categories_lt_LT.dat\n"
           "Categories_lv_LV.dat\n"
           "Categories_nb_NO.dat\n"
           "Categories_nl_NL.dat\n"
           "Categories_pl_PL.dat\n"
           "Categories_pt_BR.dat\n"
           "Categories_ro_RO.dat\n"
           "Categories_ru_RU.dat\n"
           "Categories_sk_SK.dat\n"
           "Categories_sl_SI.dat\n"
           "Categories_sv_SE.dat\n"
           "Categories_tr_TR.dat\n"
           "Categories_uk_UA.dat\n"
           "Categories_zh_CN.dat\n"
           "Categories_zh_TW.dat\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/description:\n"
           "bin\n"
           "loc\n"
           "manifest.xml\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/description/bin:\n"
           "description.swf\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/description/loc:\n"
           "description_ar_AE.dat\n"
           "description_bg_BG.dat\n"
           "description_cs_CZ.dat\n"
           "description_da_DK.dat\n"
           "description_de_DE.dat\n"
           "description_el_GR.dat\n"
           "description_en_US.dat\n"
           "description_es_ES.dat\n"
           "description_et_EE.dat\n"
           "description_fi_FI.dat\n"
           "description_fr_FR.dat\n"
           "description_he_IL.dat\n"
           "description_hr_HR.dat\n"
           "description_hu_HU.dat\n"
           "description_it_IT.dat\n"
           "description_ja_JP.dat\n"
           "description_ko_KR.dat\n"
           "description_lt_LT.dat\n"
           "description_lv_LV.dat\n"
           "description_nb_NO.dat\n"
           "description_nl_NL.dat\n"
           "description_pl_PL.dat\n"
           "description_pt_BR.dat\n"
           "description_ro_RO.dat\n"
           "description_ru_RU.dat\n"
           "description_sk_SK.dat\n"
           "description_sl_SI.dat\n"
           "description_sv_SE.dat\n"
           "description_tr_TR.dat\n"
           "description_uk_UA.dat\n"
           "description_zh_CN.dat\n"
           "description_zh_TW.dat\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/dicom:\n"
           "bin\n"
           "loc\n"
           "manifest.xml\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/dicom/bin:\n"
           "dicom.swf\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/dicom/loc:\n"
           "DICOM_ar_AE.dat\n"
           "DICOM_bg_BG.dat\n"
           "DICOM_cs_CZ.dat\n"
           "DICOM_da_DK.dat\n"
           "DICOM_de_DE.dat\n"
           "DICOM_el_GR.dat\n"
           "DICOM_en_US.dat\n"
           "DICOM_es_ES.dat\n"
           "DICOM_et_EE.dat\n"
           "DICOM_fi_FI.dat\n"
           "DICOM_fr_FR.dat\n"
           "DICOM_he_IL.dat\n"
           "DICOM_hr_HR.dat\n"
           "DICOM_hu_HU.dat\n"
           "DICOM_it_IT.dat\n"
           "DICOM_ja_JP.dat\n"
           "DICOM_ko_KR.dat\n"
           "DICOM_lt_LT.dat\n"
           "DICOM_lv_LV.dat\n"
           "DICOM_nb_NO.dat\n"
           "DICOM_nl_NL.dat\n"
           "DICOM_pl_PL.dat\n"
           "DICOM_pt_BR.dat\n"
           "DICOM_ro_RO.dat\n"
           "DICOM_ru_RU.dat\n"
           "DICOM_sk_SK.dat\n"
           "DICOM_sl_SI.dat\n"
           "DICOM_sv_SE.dat\n"
           "DICOM_tr_TR.dat\n"
           "DICOM_uk_UA.dat\n"
           "DICOM_zh_CN.dat\n"
           "DICOM_zh_TW.dat\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/gpsData:\n"
           "bin\n"
           "loc\n"
           "manifest.xml\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/gpsData/bin:\n"
           "gpsData.swf\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/gpsData/loc:\n"
           "GPSData_ar_AE.dat\n"
           "GPSData_bg_BG.dat\n"
           "GPSData_cs_CZ.dat\n"
           "GPSData_da_DK.dat\n"
           "GPSData_de_DE.dat\n"
           "GPSData_el_GR.dat\n"
           "GPSData_en_US.dat\n"
           "GPSData_es_ES.dat\n"
           "GPSData_et_EE.dat\n"
           "GPSData_fi_FI.dat\n"
           "GPSData_fr_FR.dat\n"
           "GPSData_he_IL.dat\n"
           "GPSData_hr_HR.dat\n"
           "GPSData_hu_HU.dat\n"
           "GPSData_it_IT.dat\n"
           "GPSData_ja_JP.dat\n"
           "GPSData_ko_KR.dat\n"
           "GPSData_lt_LT.dat\n"
           "GPSData_lv_LV.dat\n"
           "GPSData_nb_NO.dat\n"
           "GPSData_nl_NL.dat\n"
           "GPSData_pl_PL.dat\n"
           "GPSData_pt_BR.dat\n"
           "GPSData_ro_RO.dat\n"
           "GPSData_ru_RU.dat\n"
           "GPSData_sk_SK.dat\n"
           "GPSData_sl_SI.dat\n"
           "GPSData_sv_SE.dat\n"
           "GPSData_tr_TR.dat\n"
           "GPSData_uk_UA.dat\n"
           "GPSData_zh_CN.dat\n"
           "GPSData_zh_TW.dat\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/history:\n"
           "bin\n"
           "loc\n"
           "manifest.xml\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/history/bin:\n"
           "history.swf\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/history/loc:\n"
           "History_ar_AE.dat\n"
           "History_bg_BG.dat\n"
           "History_cs_CZ.dat\n"
           "History_da_DK.dat\n"
           "History_de_DE.dat\n"
           "History_el_GR.dat\n"
           "History_en_US.dat\n"
           "History_es_ES.dat\n"
           "History_et_EE.dat\n"
           "History_fi_FI.dat\n"
           "History_fr_FR.dat\n"
           "History_he_IL.dat\n"
           "History_hr_HR.dat\n"
           "History_hu_HU.dat\n"
           "History_it_IT.dat\n"
           "History_ja_JP.dat\n"
           "History_ko_KR.dat\n"
           "History_lt_LT.dat\n"
           "History_lv_LV.dat\n"
           "History_nb_NO.dat\n"
           "History_nl_NL.dat\n"
           "History_pl_PL.dat\n"
           "History_pt_BR.dat\n"
           "History_ro_RO.dat\n"
           "History_ru_RU.dat\n"
           "History_sk_SK.dat\n"
           "History_sl_SI.dat\n"
           "History_sv_SE.dat\n"
           "History_tr_TR.dat\n"
           "History_uk_UA.dat\n"
           "History_zh_CN.dat\n"
           "History_zh_TW.dat\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/mobile:\n"
           "bin\n"
           "loc\n"
           "manifest.xml\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/mobile/bin:\n"
           "mobile.swf\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/mobile/loc:\n"
           "Mobile_ar_AE.dat\n"
           "Mobile_bg_BG.dat\n"
           "Mobile_cs_CZ.dat\n"
           "Mobile_da_DK.dat\n"
           "Mobile_de_DE.dat\n"
           "Mobile_el_GR.dat\n"
           "Mobile_en_US.dat\n"
           "Mobile_es_ES.dat\n"
           "Mobile_et_EE.dat\n"
           "Mobile_fi_FI.dat\n"
           "Mobile_fr_FR.dat\n"
           "Mobile_he_IL.dat\n"
           "Mobile_hr_HR.dat\n"
           "Mobile_hu_HU.dat\n"
           "Mobile_it_IT.dat\n"
           "Mobile_ja_JP.dat\n"
           "Mobile_ko_KR.dat\n"
           "Mobile_lt_LT.dat\n"
           "Mobile_lv_LV.dat\n"
           "Mobile_nb_NO.dat\n"
           "Mobile_nl_NL.dat\n"
           "Mobile_pl_PL.dat\n"
           "Mobile_pt_BR.dat\n"
           "Mobile_ro_RO.dat\n"
           "Mobile_ru_RU.dat\n"
           "Mobile_sk_SK.dat\n"
           "Mobile_sl_SI.dat\n"
           "Mobile_sv_SE.dat\n"
           "Mobile_tr_TR.dat\n"
           "Mobile_uk_UA.dat\n"
           "Mobile_zh_CN.dat\n"
           "Mobile_zh_TW.dat\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/origin:\n"
           "bin\n"
           "loc\n"
           "manifest.xml\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/origin/bin:\n"
           "origin.swf\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/origin/loc:\n"
           "origin_ar_AE.dat\n"
           "origin_bg_BG.dat\n"
           "origin_cs_CZ.dat\n"
           "origin_da_DK.dat\n"
           "origin_de_DE.dat\n"
           "origin_el_GR.dat\n"
           "origin_en_US.dat\n"
           "origin_es_ES.dat\n"
           "origin_et_EE.dat\n"
           "origin_fi_FI.dat\n"
           "origin_fr_FR.dat\n"
           "origin_he_IL.dat\n"
           "origin_hr_HR.dat\n"
           "origin_hu_HU.dat\n"
           "origin_it_IT.dat\n"
           "origin_ja_JP.dat\n"
           "origin_ko_KR.dat\n"
           "origin_lt_LT.dat\n"
           "origin_lv_LV.dat\n"
           "origin_nb_NO.dat\n"
           "origin_nl_NL.dat\n"
           "origin_pl_PL.dat\n"
           "origin_pt_BR.dat\n"
           "origin_ro_RO.dat\n"
           "origin_ru_RU.dat\n"
           "origin_sk_SK.dat\n"
           "origin_sl_SI.dat\n"
           "origin_sv_SE.dat\n"
           "origin_tr_TR.dat\n"
           "origin_uk_UA.dat\n"
           "origin_zh_CN.dat\n"
           "origin_zh_TW.dat\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/rawpacket:\n"
           "bin\n"
           "loc\n"
           "manifest.xml\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/rawpacket/bin:\n"
           "rawpacket.swf\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/rawpacket/loc:\n"
           "Rawpacket_ar_AE.dat\n"
           "Rawpacket_bg_BG.dat\n"
           "Rawpacket_cs_CZ.dat\n"
           "Rawpacket_da_DK.dat\n"
           "Rawpacket_de_DE.dat\n"
           "Rawpacket_el_GR.dat\n"
           "Rawpacket_en_US.dat\n"
           "Rawpacket_es_ES.dat\n"
           "Rawpacket_et_EE.dat\n"
           "Rawpacket_fi_FI.dat\n"
           "Rawpacket_fr_FR.dat\n"
           "Rawpacket_he_IL.dat\n"
           "Rawpacket_hr_HR.dat\n"
           "Rawpacket_hu_HU.dat\n"
           "Rawpacket_it_IT.dat\n"
           "Rawpacket_ja_JP.dat\n"
           "Rawpacket_ko_KR.dat\n"
           "Rawpacket_lt_LT.dat\n"
           "Rawpacket_lv_LV.dat\n"
           "Rawpacket_nb_NO.dat\n"
           "Rawpacket_nl_NL.dat\n"
           "Rawpacket_pl_PL.dat\n"
           "Rawpacket_pt_BR.dat\n"
           "Rawpacket_ro_RO.dat\n"
           "Rawpacket_ru_RU.dat\n"
           "Rawpacket_sk_SK.dat\n"
           "Rawpacket_sl_SI.dat\n"
           "Rawpacket_sv_SE.dat\n"
           "Rawpacket_tr_TR.dat\n"
           "Rawpacket_uk_UA.dat\n"
           "Rawpacket_zh_CN.dat\n"
           "Rawpacket_zh_TW.dat\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/videoData:\n"
           "bin\n"
           "loc\n"
           "manifest.xml\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/videoData/bin:\n"
           "videoData.swf\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Application Data/Custom File Info "
           "Panels/4.0/panels/videoData/loc:\n"
           "VideoData_ar_AE.dat\n"
           "VideoData_bg_BG.dat\n"
           "VideoData_cs_CZ.dat\n"
           "VideoData_da_DK.dat\n"
           "VideoData_de_DE.dat\n"
           "VideoData_el_GR.dat\n"
           "VideoData_en_US.dat\n"
           "VideoData_es_ES.dat\n"
           "VideoData_et_EE.dat\n"
           "VideoData_fi_FI.dat\n"
           "VideoData_fr_FR.dat\n"
           "VideoData_he_IL.dat\n"
           "VideoData_hr_HR.dat\n"
           "VideoData_hu_HU.dat\n"
           "VideoData_it_IT.dat\n"
           "VideoData_ja_JP.dat\n"
           "VideoData_ko_KR.dat\n"
           "VideoData_lt_LT.dat\n"
           "VideoData_lv_LV.dat\n"
           "VideoData_nb_NO.dat\n"
           "VideoData_nl_NL.dat\n"
           "VideoData_pl_PL.dat\n"
           "VideoData_pt_BR.dat\n"
           "VideoData_ro_RO.dat\n"
           "VideoData_ru_RU.dat\n"
           "VideoData_sk_SK.dat\n"
           "VideoData_sl_SI.dat\n"
           "VideoData_sv_SE.dat\n"
           "VideoData_tr_TR.dat\n"
           "VideoData_uk_UA.dat\n"
           "VideoData_zh_CN.dat\n"
           "VideoData_zh_TW.dat\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Frameworks:\n"
           "AdbeScriptUIFlex.framework\n"
           "AdobeACE.framework\n"
           "AdobeAGM.framework\n"
           "AdobeAXE8SharedExpat.framework\n"
           "AdobeAXEDOMCore.framework\n"
           "AdobeBIB.framework\n"
           "AdobeBIBUtils.framework\n"
           "AdobeCoolType.framework\n"
           "AdobeCrashReporter.framework\n"
           "AdobeExtendScript.framework\n"
           "AdobeLinguistic.framework\n"
           "AdobeMPS.framework\n"
           "AdobeOwl.framework\n"
           "AdobePDFSettings.framework\n"
           "AdobePIP.framework\n"
           "AdobeScCore.framework\n"
           "AdobeUpdater.framework\n"
           "AdobeXMP.framework\n"
           "AdobeXMPFiles.framework\n"
           "AdobeXMPScript.framework\n"
           "AlignmentLib.framework\n"
           "CIT\n"
           "CIT.framework\n"
           "CITThreading.framework\n"
           "Cg.framework\n"
           "FileInfo.framework\n"
           "ICUConverter.framework\n"
           "ICUData.framework\n"
           "IMSLib.dylib\n"
           "LogSession.framework\n"
           "PlugPlugOwl.framework\n"
           "WRServices.framework\n"
           "adbeape.framework\n"
           "adobe_caps.framework\n"
           "adobejp2k.framework\n"
           "adobepdfl.framework\n"
           "ahclient.framework\n"
           "aif_core.framework\n"
           "aif_ocl.framework\n"
           "aif_ogl.framework\n"
           "amtlib.framework\n"
           "boost_date_time.framework\n"
           "boost_signals.framework\n"
           "boost_system.framework\n"
           "boost_threads.framework\n"
           "dvaaudiodevice.framework\n"
           "dvacore.framework\n"
           "dvamarshal.framework\n"
           "dvamediatypes.framework\n"
           "dvaplayer.framework\n"
           "dvatransport.framework\n"
           "dvaunittesting.framework\n"
           "dynamiclink.framework\n"
           "filter_graph.framework\n"
           "libtbb.dylib\n"
           "libtbbmalloc.dylib\n"
           "mediacoreif.framework\n"
           "patchmatch.framework\n"
           "updaternotifications.framework\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Frameworks/AdbeScriptUIFlex.framework:\n"
           "AdbeScriptUIFlex\n"
           "Resources\n"
           "Versions\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Frameworks/AdbeScriptUIFlex.framework/Versions:\n"
           "A\n"
           "Current\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Frameworks/AdbeScriptUIFlex.framework/Versions/A:\n"
           "AdbeScriptUIFlex\n"
           "CodeResources\n"
           "_CodeSignature\n"
           "resources\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Frameworks/AdbeScriptUIFlex.framework/Versions/A/"
           "_CodeSignature:\n"
           "CodeResources\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Frameworks/AdbeScriptUIFlex.framework/Versions/A/"
           "resources:\n"
           "Info.plist\n"
           "english.lproj\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Frameworks/AdbeScriptUIFlex.framework/Versions/A/"
           "resources/english.lproj:\n"
           "InfoPlist.strings\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Frameworks/AdobeACE.framework:\n"
           "AdobeACE\n"
           "Versions\n"
           "resources\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Frameworks/AdobeACE.framework/Versions:\n"
           "A\n"
           "current\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Frameworks/AdobeACE.framework/Versions/A:\n"
           "AdobeACE\n"
           "CodeResources\n"
           "_CodeSignature\n"
           "resources\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Frameworks/AdobeACE.framework/Versions/A/"
           "_CodeSignature:\n"
           "CodeResources\n"
           "\n"
           "/Applications/Adobe Photoshop CC/Adobe Photoshop "
           "CC.app/Contents/Frameworks/AdobeACE.framework/Versions/A/"
           "resources:\n"
           "Info.plist\n"
           "english.lproj\n"
           ""
        << 123 << 123.123 << "haha" << '\n'
        << std::string("12356");
    LOG_RAW << "Testing finished\n";
    LOG_RAW_TO(5) << "Testing finished\n";
    thread_.join();
    spdlog::shutdown();
}
