@echo off

cd %~dp0






copy ..\..\anc_coeff.bin .
copy ..\..\anc_gains.bin .
copy ..\..\br28loader.bin .
copy ..\..\ota.bin .
if not %KEY_FILE_PATH%A==A set KEY_FILE=-key %KEY_FILE_PATH%

if %PROJ_DOWNLOAD_PATH%A==A set PROJ_DOWNLOAD_PATH=..\..\..\..\..\..\output
copy %PROJ_DOWNLOAD_PATH%\*.bin .	
if exist %PROJ_DOWNLOAD_PATH%\tone_en.cfg copy %PROJ_DOWNLOAD_PATH%\tone_en.cfg .	
if exist %PROJ_DOWNLOAD_PATH%\tone_zh.cfg copy %PROJ_DOWNLOAD_PATH%\tone_zh.cfg .
if exist sdk_config.h del sdk_config.h
if exist sdk_config.c del sdk_config.c

if %TONE_EN_ENABLE%A==1A (
    if not exist tone_en.cfg copy ..\..\tone.cfg tone_en.cfg
    set TONE_FILES=tone_en.cfg
)
if %TONE_ZH_ENABLE%A==1A (
    set TONE_FILES=%TONE_FILES% tone_zh.cfg
)

if %FORMAT_VM_ENABLE%A==1A set FORMAT=-format vm
if %FORMAT_ALL_ENABLE%A==1A set FORMAT=-format all

if not %RCSP_EN%A==A (
   ..\..\json_to_res.exe ..\..\json.txt
    set CONFIG_DATA=config.dat
)

@echo on
..\..\isd_download.exe ..\..\isd_config.ini -tonorflash -dev br28 -boot 0x120000 -div8 -wait 300 -uboot ..\..\uboot.boot -app ..\..\app.bin  -tone %TONE_FILES% -res ALIGN_DIR cfg_tool.bin ..\..\p11_code.bin stream.bin %CONFIG_DATA% %KEY_FILE% -uboot_compress %FORMAT% -output-fw jl_isd.fw -output-ufw update.ufw
@echo off





:: -format all
::-reboot 2500

@rem ɾ����ʱ�ļ�-format all
if exist *.mp3 del *.mp3 
if exist *.PIX del *.PIX
if exist *.TAB del *.TAB
if exist *.res del *.res
if exist *.sty del *.sty

copy update.ufw %PROJ_DOWNLOAD_PATH%\update.ufw
copy jl_isd.bin %PROJ_DOWNLOAD_PATH%\jl_isd.bin
copy jl_isd.fw %PROJ_DOWNLOAD_PATH%\jl_isd.fw

@rem ��������˵��
@rem -format vm        //����VM ����
@rem -format cfg       //����BT CFG ����
@rem -format 0x3f0-2   //��ʾ�ӵ� 0x3f0 �� sector ��ʼ�������� 2 �� sector(��һ������Ϊ16���ƻ�10���ƶ��ɣ��ڶ�������������10����)

ping /n 2 127.1>null
IF EXIST null del null
