# there should be passed on command line via /D
#!define RELEASE_DIR "teeworlds"
#!define SOURCE_DIR "source"
#########

!define NAME "Teeworlds"
!define COMPANY "teeworlds.com"
!define VERSION "0.6.1"


Name "${NAME} ${VERSION} Installer"
Icon "${SOURCE_DIR}\other\icons\Teeworlds.ico"
 
# define name of installer
#outFile "teeworlds-${VERSION}-installer.exe"
outFile "teeworlds-installer.exe"

# define installation directory
installDir "$PROGRAMFILES\${NAME}"

VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName" "${NAME}"
VIAddVersionKey "Comments" ""
VIAddVersionKey "CompanyName" "${COMPANY}"
VIAddVersionKey "FileDescription" "${NAME} ${VERSION} Installer"
VIAddVersionKey "FileVersion" "${VERSION}"

# installer ui stuff
LicenseData "${RELEASE_DIR}\license.txt"

Page license
#Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

setcompressor /SOLID lzma

# default section
section
 
	# set the installation directory as the destination for the following actions
	setOutPath $INSTDIR
	
	file /r "${RELEASE_DIR}\*"
	createShortCut "$SMPROGRAMS\${NAME}\Teeworlds.lnk" "$INSTDIR\teeworlds.exe"
	
	# Uninstall information
	writeUninstaller "$INSTDIR\uninstall.exe"
	createShortCut "$SMPROGRAMS\${NAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe"
	
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" \
				 "DisplayName" "${NAME} ${VERSION}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" \
				 "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
sectionEnd
 
# uninstaller section start
section "uninstall"
 
	# first, delete the uninstaller
	RMDir /r "$INSTDIR"
 
	# second, remove the link from the start menu
	SetShellVarContext all
	RMDir /r "$SMPROGRAMS\${NAME}"
	
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}"
 
# uninstaller section end
sectionEnd
