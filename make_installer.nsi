!include LogicLib.nsh

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "FileAssociation.nsh"

; --------------------------------

; Change these as needed
!define VERSION "0.101.0"
!define INST_FOLDER "package"

SetCompressor /SOLID /FINAL lzma
SetCompressorDictSize 64

Name "sdl_img-${VERSION}"
OutFile "sdl_img-${VERSION}-Setup.exe"

; This doesn't work if they keep the default install folder
; but on the other hand you can install it anywhere without admin...
; So which is better?  Forcing admin because of the default install location
; or letting some users have to run it again as administrator?
;RequestExecutionLevel user
RequestExecutionLevel admin

; Default install folder
InstallDir "$PROGRAMfILES64\sdl_img"

; Do registry stuff here if wanted
!define REGUNINSTKEY "sdl_img"  ; could use GUID to assure uniqueness
;!define REGHKEY HKCU    ; for execution level user
!define REGHKEY HKLM   ; for execution level admin

; Do I need double backslashes?
!define REGPATH_WINUNINST "Software\Microsoft\Windows\CurrentVersion\Uninstall"


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


!define MUI_WELCOMEPAGE_TEXT "http://robertwinkler.com/projects/sdl_img$\r$\n$\r$\nThis will install sdl_img on your computer.$\r$\n$\r$\nsdl_img is an image viewer built using the SDL2 and stb_image libraries.  Thanks to the latter, it has support for many different image formats including JPEG, PNG, BMP, GIF, and netpbm.  sdl_img focuses on interesting, unique features like multi-image browsing, full control of GIF playback, a vim inspired thumbnail mode, and a list mode."


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
	File /nonfatal ${INST_FOLDER}\*.dll

	; SetOutPath $INSTDIR\lib
	File ${INST_FOLDER}\sdl_img.ico
	File ${INST_FOLDER}\sdl_img.bmp
	File ${INST_FOLDER}\sdl_img2.bmp
	File ${INST_FOLDER}\LICENSE
	File ${INST_FOLDER}\LICENSE.txt
	File ${INST_FOLDER}\README.md
	File ${INST_FOLDER}\ca-bundle.crt

	WriteUninstaller "$INSTDIR\uninstall.exe"

	WriteRegStr ${REGHKEY} "${REGPATH_WINUNINST}\\${REGUNINSTKEY}" "DisplayName" "sdl_img"
	WriteRegStr ${REGHKEY} "${REGPATH_WINUNINST}\\${REGUNINSTKEY}" "UninstallString" "$\"$INSTDIR\\uninstall.exe$\""
	WriteRegStr ${REGHKEY} "${REGPATH_WINUNINST}\\${REGUNINSTKEY}" "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"

	WriteRegStr ${REGHKEY} "${REGPATH_WINUNINST}\\${REGUNINSTKEY}" "InstallLocation" "$INSTDIR"
	WriteRegStr ${REGHKEY} "${REGPATH_WINUNINST}\\${REGUNINSTKEY}" "Publisher" "Robert Winkler"
	WriteRegStr ${REGHKEY} "${REGPATH_WINUNINST}\\${REGUNINSTKEY}" "HelpLink" "https://www.robertwinkler.com"
	WriteRegStr ${REGHKEY} "${REGPATH_WINUNINST}\\${REGUNINSTKEY}" "DisplayVersion" "${VERSION}"


	;; get cumulative size of all files in and under install dir
	;; report the total in KB (decimal)
	;; place the answer into $0  (ignore $1 $2)
	;${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2

	;!echo "installed size estimate $0"

	;; Convert the decimal KB value in $0 to DWORD
	;; put it right back into $0
	;IntFmt $0 "0x$08X" $0

	;!echo "installed size estimate dword $0"

	;; If I do this here it won't count the size of the source code if they include it?
	;; create/update the reg key
	;WriteRegDWORD ${REGHKEY} "${REGPATH_WINUNINST}\\${REGUNINSTKEY}" "EstimatedSize" "$0" ; where $0 is the size in bytes



SectionEnd

; I'll put back the group if I ever add other optional sections
SectionGroup /e "Optional" optional_id
	Section "File Associations" file_assoc_id
		${RegisterExtension} "$INSTDIR\\sdl_img.exe" ".jpg" "JPG_File"
		${RegisterExtension} "$INSTDIR\\sdl_img.exe" ".jpeg" "JPEG_File"
		${RegisterExtension} "$INSTDIR\\sdl_img.exe" ".gif" "GIF_File"
		${RegisterExtension} "$INSTDIR\\sdl_img.exe" ".png" "PNG_File"
		${RegisterExtension} "$INSTDIR\\sdl_img.exe" ".bmp" "BMP_File"
		${RegisterExtension} "$INSTDIR\\sdl_img.exe" ".ppm" "PPM_File"
		${RegisterExtension} "$INSTDIR\\sdl_img.exe" ".pgm" "PGM_File"
		${RegisterExtension} "$INSTDIR\\sdl_img.exe" ".tga" "TGA_File"
		${RegisterExtension} "$INSTDIR\\sdl_img.exe" ".hdr" "HDR_File"
		${RegisterExtension} "$INSTDIR\\sdl_img.exe" ".pic" "PIC_File"
		${RegisterExtension} "$INSTDIR\\sdl_img.exe" ".psd" "PSD_File"
	SectionEnd

	Section "Start Menu Shortuct" start_menu_id
		CreateShortcut /NoWorkingDir "$STARTMENU\sdl_img.lnk" "$INSTDIR\sdl_img.exe" "" "$INSTDIR\sdl_img.ico" 0
	SectionEnd

	Section "Desktop Shortuct" desktop_shortcut_id
		CreateShortcut /NoWorkingDir "$DESKTOP\sdl_img.lnk" "$INSTDIR\sdl_img.exe" "" "$INSTDIR\sdl_img.ico" 0
	SectionEnd

	Section "Source Code" source_id
		SetOutPath "$INSTDIR\src"
		FILE src\*
	SectionEnd


SectionGroupEnd



;-----------------------------------
; Descriptions

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${file_assoc_id} "Make the default program for supported filetypes"
	!insertmacro MUI_DESCRIPTION_TEXT ${start_menu_id} "Create a start menu shortcut."
	!insertmacro MUI_DESCRIPTION_TEXT ${desktop_shortcut_id} "Create a desktop shortcut."
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
	delete "$INSTDIR\sdl_img.exe"
	delete "$INSTDIR\*.dll"
	delete "$INSTDIR\sdl_img.ico"
	delete "$INSTDIR\sdl_img.bmp"
	delete "$INSTDIR\sdl_img2.bmp"
	delete "$INSTDIR\LICENSE"
	delete "$INSTDIR\LICENSE.txt"
	delete "$INSTDIR\README.md"
	delete "$INSTDIR\ca-bundle.crt"

	rmdir /r "$INSTDIR\src"

	; remove uninstaller last
	delete "$INSTDIR\uninstall.exe"

	; will only remove if dir is empty
	rmDir "$INSTDIR"

	; Does this delete all the registry stuff I did?  I guess it's sort of a directory thing
	DeleteRegKey ${REGHKEY} "${REGPATH_WINUNINST}\\${REGUNINSTKEY}"

	; these might not exist...
	${UnRegisterExtension} ".jpg" "JPG_File"
	${UnRegisterExtension} ".jpeg" "JPEG_File"
	${UnRegisterExtension} ".gif" "GIF_File"
	${UnRegisterExtension} ".png" "PNG_File"
	${UnRegisterExtension} ".bmp" "BMP_File"
	${UnRegisterExtension} ".ppm" "PPM_File"
	${UnRegisterExtension} ".pgm" "PGM_File"
	${UnRegisterExtension} ".tga" "TGA_File"
	${UnRegisterExtension} ".hdr" "HDR_File"
	${UnRegisterExtension} ".pic" "PIC_File"
	${UnRegisterExtension} ".psd" "PSD_File"


	; Don't actually have a folder
	;RMDir /r $STARTMENU\sdl_img

	; these might not exist...
	delete $STARTMENU\sdl_img.lnk
	delete $DESKTOP\sdl_img.lnk


SectionEnd


