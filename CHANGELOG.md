# Changelog

Toutes les modifications notables de **Sysex77** (éditeur MIDI pour Yamaha SY77 / SY99 / TG77)
sont documentées dans ce fichier. / All notable changes to this project are documented here.

Le format s'inspire de [Keep a Changelog](https://keepachangelog.com/fr/1.1.0/)
et le projet suit un versionnage de type [SemVer](https://semver.org/lang/fr/).

> Note : l'historique détaillé du développement initial (2019) est conservé dans `Historique.rtf`.

## [Unreleased]

### Ajouté / Added
- Build **universelle macOS** (arm64 + x86_64) : fonctionne sur Mac Apple Silicon
  ET Intel. Configuré durablement dans `Sysex77.jucer` (Release :
  `ARCHS=x86_64 arm64`, `ONLY_ACTIVE_ARCH=NO`).
  Commande de distribution :
  `xcodebuild -scheme "Sysex77 - App" -configuration Release -destination 'generic/platform=macOS' build`.

### Modifié / Changed
- Renommage de la cible de build `MidiDemo` → `Sysex77` (l'exécutable du bundle
  s'appelle désormais `Sysex77`).
- **API MIDI migrée des index vers les identifiants de périphériques**
  (`MidiDeviceInfo`) : `MidiInput::getAvailableDevices()` /
  `openDevice(identifier, …)` au lieu de `getDevices()` / `openDevice(index, …)`.
  Plus robuste lorsque l'ordre des ports MIDI change (`MidiDemo.h`).
- Suppression des derniers warnings de dépréciation JUCE :
  `forEachXmlChildElement` → `getChildIterator()` (`AWMVue.h`, `Controller.h`,
  `MidiDemo.h`) ; `Font(...)` → `FontOptions(...)` ; `XmlElement::writeToFile`
  → `writeTo` ; `Font::getStringWidth` → `GlyphArrangement::getStringWidth` ;
  correction d'une perte de précision `long` → `int` (`MidiSysex.h`).
- App signée ad-hoc (`codesign -s -`). Note : pas de certificat Developer ID
  disponible → l'app reste non notarisée (avertissement Gatekeeper côté
  utilisateur).

### Supprimé / Removed
- Code mort : `Source/FilterVue copie.h`, `Source/labo.h`,
  `Source/WidgetsDemo.h.pdf`.

## [0.0.5] - 2026-06-21

### Corrigé / Fixed
- Accolade fermante en trop dans `Source/Config.h` (`setState()`) qui empêchait la compilation.

### Modifié / Changed
- **Migration vers JUCE 8** (depuis JUCE 5.4.2). Le projet compile à nouveau sous Xcode :
  - `XmlDocument::parse()` renvoie désormais un `std::unique_ptr` — assignation directe
    au lieu de `.reset()` (`AWMVue.h`, `Controller.h`, `MidiDemo.h`).
  - `MidiInput::openDevice()` / `MidiOutput::openDevice()` renvoient un `std::unique_ptr`
    (`MidiDemo.h`).
  - `ListBox::setHeaderComponent()` attend un `std::make_unique<Header>` (`BankTableModel.h`,
    `VoicesTableModel.h`).
  - `File::createInputStream()` renvoie un `std::unique_ptr` (`Main.cpp`).
  - `SystemTrayIconComponent::setIconImage()` utilise la nouvelle signature à deux images
    (couleur + template) (`Main.cpp`).
  - Remplacement de `float_Pi` (déprécié) par `juce::MathConstants<float>::pi`
    (`Element.h`, `LookAndFeel.h`, `MidiDemo.h`).
  - Modernisation de `ConfigPage::buttonClicked()` : `ColourSelector` géré via `std::unique_ptr`
    et `CallOutBox::launchAsynchronously(std::move(...))` (`Config.h`).
  - Activation de `JUCE_MODAL_LOOPS_PERMITTED` pour conserver les boîtes de dialogue modales
    synchrones (`AlertWindow::showOkCancelBox`).
- Projet `.jucer` : `cppLanguageStandard="latest"`, format de projet mis à jour.

### Supprimé / Removed
- Ressources pointant vers des chemins absolus inexistants
  (`/Applications/JKnobMan133-mac/...`) dans `Sysex77.jucer`.

## [0.0.4] - 2019

- Versions de développement initiales : éditeur AFM/AWM, vues filtres, oscillateurs,
  tableaux de banques/voix, gestion MIDI Sysex et dump de banques SY77/SY99.
- Voir `Historique.rtf` pour le journal détaillé (février 2019).

[Unreleased]: https://github.com/nseaSeb/Sysex77/compare/v0.0.5...HEAD
[0.0.5]: https://github.com/nseaSeb/Sysex77/compare/v0.0.4...v0.0.5
[0.0.4]: https://github.com/nseaSeb/Sysex77/releases/tag/v0.0.4
</content>
</invoke>
