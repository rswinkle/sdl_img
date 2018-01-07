!include LogicLib.nsh

!include "MUI2.nsh"

; --------------------------------

; Change these as needed
!define VERSION "0.8"
!define INST_FOLDER "sdl_img"

SetCompressor /SOLID /FINAL lzma
SetCompressorDictSize 64

Name "sdl_img-${VERSION}"
OutFile "sdl_img-${VERSION}-Setup.exe"

; Default install folder
InstallDir "$PROGRAMfILES64\sdl_img"

; Do registry stuff here if wanted

;-----------------------------------
; Interface configuration


; !define MUI_ICON "${INST_FOLDER}\sdl_img.ico"
; !define MUI_UNICON "${INST_FOLDER}\sdl_img.ico"
; 
; ; Recommended to be 150 x 57
; !define MUI_HEADERIMAGE
; !define MUI_HEADERIMAGE_BITMAP "${INST_FOLDER}\sdl_img.bmp"
; !define MUI_HEADERIMAGE_UNBITMAP "${INST_FOLDER}\sdl_img.bmp"
; 
; !define MUI_ABORTWARNING
; 
; ; Recommended to be 164 x 314
; !define MUI_WELCOMEFINISHPAGE_BITMAP "${INST_FOLDER}\sdl_img2.bmp"
; !define MUI_UNWELCOMEFINISHPAGE_BITMAP "${INST_FOLDER}\sdl_img2.bmp"
; 
; !define MUI_WELCOMEPAGE_TITLE "Welcome to the sdl_img Setup"


!define MUI_WELCOMEPAGE_TEXT "http://robertwinkler.com/projects/sdl_img$\r$\n$\r$\nThis will install sdl_img on your computer.$\r$\n$\r$\nsdl_img is an image viewer built using the SDL2 and stb_image libraries.  Thanks to the latter, it has support for 9 different image formats including JPEG, PNG, BMP and GIF.  sdl_img focuses on interesting, unique features like multi-image browsing."


!define MUI_FINISHPAGE_LINK "sdl_img Website"
!define MUI_FINISHPAGE_LINK_LOCATION "http://robertwinkler.com/projects/sdl_img"

!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\README.txt"




;-----------------------------------
; Pages

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${INST_FOLDER}\LICENSE.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH


!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH


;-----------------------------------
; Languages

; Extremely unlikely we'll ever add support for other languages
!insertmacro MUI_LANGUAGE "English"


;-----------------------------------
; Installer Sections


#default section start
Section "-Core"
	SetOutPath $INSTDIR
	File ${INST_FOLDER}\*

	WriteUninstaller $INSTDIR\uninstaller.exe
SectionEnd

SectionGroup /e "sdl_img" sdl_img_id

	Section "Shortcuts" shortcuts_id
		CreateShortcut $DESKTOP\sdl_img.lnk $INSTDIR\sdl_img.exe "" $INSTDIR\sdl_img.ico

		CreateDirectory $STARTMENU\sdl_img
		CreateShortcut $STARTMENU\sdl_img\sdl_img.lnk $INSTDIR\sdl_img.exe "" $INSTDIR\sdl_img.ico
		CreateShortcut $STARTMENU\sdl_img\Uninstall.lnk $INSTDIR\uninstaller.exe
	SectionEnd

SectionGroupEnd



;-----------------------------------
; Descriptions

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${sdl_img_id} "Core components"
	!insertmacro MUI_DESCRIPTION_TEXT ${shortcuts_id} "Installs shortcuts for the Desktop and Start Menu."

!insertmacro MUI_FUNCTION_DESCRIPTION_END


;------------------------------------
; Installer Functions

;Function .onInit
;
;FunctionEnd





;------------------------------------
; Uninstaller section

; NOTE in Uninstall section $INSTDIR contains location of uninstaller
; not necessarily the same value as in the installer section
Section "Uninstall"
	RMDir /r /REBOOTOK $INSTDIR

	RMDir /r $STARTMENU\sdl_img


SectionEnd


