!include LogicLib.nsh

!include "MUI2.nsh"

; --------------------------------

; Change these as needed
!define VERSION "0.98"
!define INST_FOLDER "package"

SetCompressor /SOLID /FINAL lzma
SetCompressorDictSize 64

Name "sdl_img-${VERSION}"
OutFile "sdl_img-${VERSION}-Setup.exe"

; Not sure how this works if they keep the default install folder
RequestExecutionLevel user

; Default install folder
InstallDir "$PROGRAMfILES64\sdl_img"

; Do registry stuff here if wanted

;-----------------------------------
; Interface configuration


!define MUI_ICON "${INST_FOLDER}\sdl_img.ico"
!define MUI_UNICON "${INST_FOLDER}\sdl_img.ico"
; 
; Recommended to be 150 x 57
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${INST_FOLDER}\sdl_img.bmp"
!define MUI_HEADERIMAGE_UNBITMAP "${INST_FOLDER}\sdl_img.bmp"
; 
!define MUI_ABORTWARNING
; 
; Recommended to be 164 x 314
!define MUI_WELCOMEFINISHPAGE_BITMAP "${INST_FOLDER}\sdl_img2.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${INST_FOLDER}\sdl_img2.bmp"
; 
; !define MUI_WELCOMEPAGE_TITLE "Welcome to the sdl_img Setup"


!define MUI_WELCOMEPAGE_TEXT "http://robertwinkler.com/projects/sdl_img$\r$\n$\r$\nThis will install sdl_img on your computer.$\r$\n$\r$\nsdl_img is an image viewer built using the SDL2 and stb_image libraries.  Thanks to the latter, it has support for many different image formats including JPEG, PNG, BMP, GIF, and netpbm.  sdl_img focuses on interesting, unique features like multi-image browsing."


!define MUI_FINISHPAGE_LINK "sdl_img Website"
!define MUI_FINISHPAGE_LINK_LOCATION "http://robertwinkler.com/projects/sdl_img"

!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\README.md"




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

; Extremely unlikely I'll ever add support for other languages
!insertmacro MUI_LANGUAGE "English"


;-----------------------------------
; Installer Sections


#default section start
Section "-Core"
	SetOutPath $INSTDIR
	File ${INST_FOLDER}\sdl_img.exe

	;SetOutPath $INSTDIR\lib
	File ${INST_FOLDER}\*.dll

	; SetOutPath $INSTDIR\lib
	File ${INST_FOLDER}\sdl_img.ico
	File ${INST_FOLDER}\sdl_img.bmp
	File ${INST_FOLDER}\sdl_img2.bmp
	File ${INST_FOLDER}\LICENSE
	File ${INST_FOLDER}\LICENSE.txt
	File ${INST_FOLDER}\README.md
	File ${INST_FOLDER}\ca-bundle.crt

	WriteUninstaller $INSTDIR\uninstall.exe

	;WriteRegStr SHCTX "Software\sdl_img " "" $INSTDIR
	WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\sdl_img" "DisplayName" "sdl_img"
	WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\sdl_img" "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
	WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\sdl_img" "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"



SectionEnd

; I'll put back the group if I ever add other optional sections
;SectionGroup /e "Optional" optional_id
	Section "Source Code" source_id
		SetOutPath $INSTDIR\src
		FILE src\*
	SectionEnd

;SectionGroupEnd



;-----------------------------------
; Descriptions

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${source_id} "Include the sdl_img source code."

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
	; RMDir /r /REBOOTOK $INSTDIR
	;
	delete $INSTDIR\sdl_img.exe
	delete $INSTDIR\*.dll
	delete $INSTDIR\sdl_img.ico
	delete $INSTDIR\sdl_img.bmp
	delete $INSTDIR\sdl_img2.bmp
	delete $INSTDIR\LICENSE
	delete $INSTDIR\LICENSE.txt
	delete $INSTDIR\README.md
	delete $INSTDIR\ca-bundle.crt

	rmdir /r $INSTDIR\src

	; remove uninstaller last
	delete $INSTDIR\uninstall.exe

	; will only remove if dir is empty
	rmDir $INSTDIR

	DeleteRegKey SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\sdl_img"


	; don't forget this 
	RMDir /r $STARTMENU\sdl_img


SectionEnd


