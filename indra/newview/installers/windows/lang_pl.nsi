; First is default
LoadLanguageFile "${NSISDIR}\Contrib\Language files\Polish.nlf"

; Language selection dialog
LangString InstallerLanguageTitle  ${LANG_POLISH} "Język instalatora"
LangString SelectInstallerLanguage  ${LANG_POLISH} "Proszę wybrać język instalatora"

; subtitle on license text caption
LangString LicenseSubTitleUpdate ${LANG_POLISH} " Aktualizacja"
LangString LicenseSubTitleSetup ${LANG_POLISH} " Instalacja"

; installation directory text
LangString DirectoryChooseTitle ${LANG_POLISH} "Katalog instalacji" 
LangString DirectoryChooseUpdate ${LANG_POLISH} "Wybierz katalog instalacji Kokua w celu aktualizacji wersji ${VERSION_LONG}.(XXX):"
LangString DirectoryChooseSetup ${LANG_POLISH} "Wybierz katalog instalacji Kokua w:"

; CheckStartupParams message box
LangString CheckStartupParamsMB ${LANG_POLISH} "Nie można odnaleźć programu '$INSTPROG'. Cicha aktualizacja zakończyła się niepowodzeniem."

; installation success dialog
LangString InstSuccesssQuestion ${LANG_POLISH} "Czy uruchomić Kokua teraz?"

; remove old NSIS version
LangString RemoveOldNSISVersion ${LANG_POLISH} "Poszukiwanie starszej wersji..."

; check windows version
LangString CheckWindowsVersionDP ${LANG_POLISH} "Sprawdzanie wersji Windows..."
LangString CheckWindowsVersionMB ${LANG_POLISH} 'Kokua obsługuje tylko Windows XP.$\n$\nPróba zainstalowania na Windows $R0 może spowodować awarie i utratę danych.$\n$\n'
LangString CheckWindowsServPackMB ${LANG_POLISH} "Zalecane jest uruchamianie Kokua z najnowszym dostępnym Service Packiem zainstalowanym w systemie.$\nPomaga on w podniesieniu wydajności i stabilności programu."
LangString UseLatestServPackDP ${LANG_POLISH} "Użyj usługi Windows Update, aby zainstalować najnowszy Service Pack."

; checkifadministrator function (install)
LangString CheckAdministratorInstDP ${LANG_POLISH} "Sprawdzanie zezwolenia na instalację..."
LangString CheckAdministratorInstMB ${LANG_POLISH} 'Używasz "ograniczonego" konta.$\nMusisz być zalogowany jako "administrator" aby zainstalować Kokua.'

; checkifadministrator function (uninstall)
LangString CheckAdministratorUnInstDP ${LANG_POLISH} "Sprawdzanie zezwolenia na odinstalowanie..."
LangString CheckAdministratorUnInstMB ${LANG_POLISH} 'Używasz "ograniczonego" konta.$\nMusisz być być zalogowany jako "administrator" aby zainstalować Kokua.'

; checkifalreadycurrent
LangString CheckIfCurrentMB ${LANG_POLISH} "Kokua ${VERSION_LONG} jest już zainstalowane.$\n$\nCzy chcesz zainstalować Kokua ponownie?"

; checkcpuflags
LangString MissingSSE2 ${LANG_POLISH} "Ten komputer może nie mieć procesora z obsługą SSE2, który jest wymagany aby uruchomić Firestorma w wersji ${VERSION_LONG}. Chcesz kontynuować?"

; closesecondlife function (install)
LangString CloseSecondLifeInstDP ${LANG_POLISH} "Oczekiwanie na zamknięcie Kokua..."
LangString CloseSecondLifeInstMB ${LANG_POLISH} "Kokua nie może zostać zainstalowane, ponieważ jest już włączone.$\n$\nZakończ swoje działania i wybierz OK aby zamknąć Kokua i kontynuować.$\nWybierz CANCEL aby anulować instalację."

; closesecondlife function (uninstall)
LangString CloseSecondLifeUnInstDP ${LANG_POLISH} "Oczekiwanie na zamknięcie Kokua..."
LangString CloseSecondLifeUnInstMB ${LANG_POLISH} "Kokua nie może zostać zainstalowane, ponieważ jest już włączone.$\n$\nZakończ swoje działania i wybierz OK aby zamknąć Kokua i kontynuować.$\nWybierz CANCEL aby anulować."

; CheckNetworkConnection
LangString CheckNetworkConnectionDP ${LANG_POLISH} "Sprawdzanie połączenia sieciowego..."

; removecachefiles
LangString RemoveCacheFilesDP ${LANG_POLISH} "Kasowanie plików pamięci podręcznej (cache) w folderze Documents and Settings"

; delete program files
LangString DeleteProgramFilesMB ${LANG_POLISH} "Nadal istnieją pliki w katalogu instalacyjnym Kokua.$\n$\nMożliwe, że są to pliki, które stworzyłeś/stworzyłaś lub przeniosłeś/przeniosłaś do:$\n$INSTDIR$\n$\nCzy chcesz je usunąć?"

; uninstall text
LangString UninstallTextMsg ${LANG_POLISH} "To spowoduje odinstalowanie Kokua ${VERSION_LONG} z Twojego systemu."
