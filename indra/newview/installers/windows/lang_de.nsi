; First is default
LoadLanguageFile "${NSISDIR}\Contrib\Language files\German.nlf"

; Language selection dialog
LangString InstallerLanguageTitle  ${LANG_GERMAN} "Installationssprache"
LangString SelectInstallerLanguage  ${LANG_GERMAN} "Bitte wählen Sie die Installationssprache"

; subtitle on license text caption (setup new version or update current one
LangString LicenseSubTitleUpdate ${LANG_GERMAN} " Update"
LangString LicenseSubTitleSetup ${LANG_GERMAN} " Setup"

; description on license page
LangString LicenseDescUpdate ${LANG_GERMAN} "Dieses Paket wird das bereits installierte Dayturn mit Version ${VERSION_LONG}. ersetzen."
LangString LicenseDescSetup ${LANG_GERMAN} "Dieses Paket wird Dayturn auf Ihrem Computer installieren."
LangString LicenseDescNext ${LANG_GERMAN} "Weiter"

; installation directory text
LangString DirectoryChooseTitle ${LANG_GERMAN} "Installations-Ordner"
LangString DirectoryChooseUpdate ${LANG_GERMAN} "Wählen Sie den Dayturn Ordner für dieses Update:"
LangString DirectoryChooseSetup ${LANG_GERMAN} "Pfad in dem Dayturn installiert werden soll:"

; CheckStartupParams message box
LangString CheckStartupParamsMB ${LANG_GERMAN} "Konnte Programm '$INSTPROG' nicht finden. Stilles Update fehlgeschlagen."

; installation success dialog
LangString InstSuccesssQuestion ${LANG_GERMAN} "Dayturn starten?"

; remove old NSIS version
LangString RemoveOldNSISVersion ${LANG_GERMAN} "Überprüfe alte Version ..."

; check windows version
LangString CheckWindowsVersionDP ${LANG_GERMAN} "Überprüfung der Windows Version ..."
LangString CheckWindowsVersionMB ${LANG_GERMAN} 'Dayturn unterstützt nur Windows Vista.$\n$\nDer Versuch es auf Windows $R0 zu installieren, könnte zu unvorhersehbaren Abstürzen und Datenverlust führen.$\n$\nTrotzdem installieren?'
LangString CheckWindowsServPackMB ${LANG_GERMAN} "Es wird empfohlen, das aktuellste Service Pack des Betriebssystems für Dayturn zu verwenden.$\nEs ist hilftreich für Performance und Stabilität des Programms."
LangString UseLatestServPackDP ${LANG_GERMAN} "Bitte Windows Update benutzen, um das aktuellste Service Pack zu installieren."

; checkifadministrator function (install)
LangString CheckAdministratorInstDP ${LANG_GERMAN} "Überprüfung der Installations-Berechtigungen ..."
LangString CheckAdministratorInstMB ${LANG_GERMAN} 'Sie besitzen ungenügende Berechtigungen.$\nSie müssen ein "administrator" sein, um Dayturn installieren zu können.'

; checkifadministrator function (uninstall)
LangString CheckAdministratorUnInstDP ${LANG_GERMAN} "Überprüfung der Entfernungs-Berechtigungen ..."
LangString CheckAdministratorUnInstMB ${LANG_GERMAN} 'Sie besitzen ungenügende Berechtigungen.$\nSie müssen ein "administrator" sein, um Dayturn entfernen zu können..'

; checkifalreadycurrent
LangString CheckIfCurrentMB ${LANG_GERMAN} "Anscheinend ist Dayturn ${VERSION_LONG} bereits installiert.$\n$\nWürden Sie es gerne erneut installieren?"

; checkcpuflags
LangString MissingSSE2 ${LANG_GERMAN} "Dieser PC besitzt möglicherweise keinen Prozessor mit SSE2-Unterstützung, die für die Ausführung von Dayturn ${VERSION_LONG} benötigt wird. Trotzdem installieren?"

; closesecondlife function (install)
LangString CloseSecondLifeInstDP ${LANG_GERMAN} "Warten auf die Beendigung von Dayturn ..."
LangString CloseSecondLifeInstMB ${LANG_GERMAN} "Dayturn kann nicht installiert oder ersetzt werden, wenn es bereits läuft.$\n$\nBeenden Sie, was Sie gerade tun und klicken Sie OK, um Dayturn zu beenden.$\nKlicken Sie CANCEL, um die Installation abzubrechen."

; closesecondlife function (uninstall)
LangString CloseSecondLifeUnInstDP ${LANG_GERMAN} "Warten auf die Beendigung von Dayturn ..."
LangString CloseSecondLifeUnInstMB ${LANG_GERMAN} "Dayturn kann nicht entfernt werden, wenn es bereits läuft.$\n$\nBeenden Sie, was Sie gerade tun und klicken Sie OK, um Dayturn zu beenden.$\nKlicken Sie CANCEL, um abzubrechen."

; CheckNetworkConnection
LangString CheckNetworkConnectionDP ${LANG_GERMAN} "Prüfe Netzwerkverbindung..."

; removecachefiles
LangString RemoveCacheFilesDP ${LANG_GERMAN} "Löschung aller Cache Dateien in Dokumente und Einstellungen."

; delete program files
LangString DeleteProgramFilesMB ${LANG_GERMAN} "Es existieren weiterhin Dateien in Ihrem SecondLife Programm Ordner.$\n$\nDies sind möglicherweise Dateien, die sie modifiziert oder bewegt haben:$\n$INSTDIR$\n$\nMöchten Sie diese ebenfalls löschen?"

; uninstall text
LangString UninstallTextMsg ${LANG_GERMAN} "Dies wird Dayturn ${VERSION_LONG} von Ihrem System entfernen."
