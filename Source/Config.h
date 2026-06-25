/*
 ==============================================================================
 
 Config.h
 Created: 13 Nov 2018 9:48:57am
 Author:  Sébastien Portrait
 
 */


#pragma once

#include "AppSettings.h"
#include "Themes.h"
#include "ThemeBuilder.h"

//==============================================================================
// Langue de l'UI : applique (ou retire) les mappings LocalisedStrings. Les clés source sont en
// anglais ; "fr" charge Ressources/French.txt (embarqué en BinaryData), "en" repasse aux clés.
// Appelé au démarrage (Main.cpp, depuis la préférence "Language") et au changement dans Settings.
inline void syApplyLanguage (const juce::String& lang)
{
    if (lang == "fr")
        juce::LocalisedStrings::setCurrentMappings (new juce::LocalisedStrings (
            juce::String::createStringFromData (BinaryData::French_txt, BinaryData::French_txtSize), false));
    else
        juce::LocalisedStrings::setCurrentMappings (nullptr); // anglais = clés source, pas de mapping
}

//==============================================================================
// Vue "Setting" : moyeu de configuration (cartes Machine / MIDI / Apparence / Développeur).
// Les anciens sélecteurs de couleur ont été retirés (remplacés par le Theme Builder).
// Les sélecteurs d'interface MIDI In/Out sont la propriété de MidiDemo et injectés via
// attachMidiInterface() (un Component n'a qu'un seul parent).
struct ConfigPage   : public Component, public ComboBox::Listener

{

    ConfigPage()
    {
        setOpaque(false);
        addAndMakeVisible(comboModel);
        addAndMakeVisible(labEngine);
        addAndMakeVisible(comboEngine);
        labEngine.setText (TRANS("MIDI channel (Device 1-16 / ALL)"), dontSendNotification);
        labEngine.setColour (Label::textColourId, SYColLabel);

        // Suivre le synthé : quand un paramètre arrive du SY77, ouvre la vue concernée.
        addAndMakeVisible (followBtn);
        followSynth = getAppSettings()->getBoolValue ("FollowSynth", false);
        followBtn.setToggleState (followSynth, dontSendNotification);
        followBtn.setColour (ToggleButton::tickColourId, SYColSelected);
        followBtn.setTooltip (TRANS("Automatically open the view of the parameter received from the SY77"));
        followBtn.onClick = [this]
        {
            followSynth = followBtn.getToggleState();
            getAppSettings()->setValue ("FollowSynth", followSynth);
            getAppSettings()->saveIfNeeded();
        };

        // Test de connexion : envoie un Identity Request au synthé (câblé par MidiDemo) ;
        // la réponse (ou le timeout) met à jour labConnStatus via setConnStatus().
        addAndMakeVisible (btTestConn);
        btTestConn.onClick = [this] { if (onTestConnection) onTestConnection(); };
        addAndMakeVisible (labConnStatus);
        labConnStatus.setColour (Label::textColourId, SYColLabel);
        labConnStatus.setFont (Font (FontOptions (12.0f)));
        labConnStatus.setJustificationType (Justification::centredLeft);

        // Mode développeur : fait apparaître l'onglet "Midi Setting" (moniteur MIDI + outils RE).
        // Le câblage add/remove de l'onglet est fourni par MidiDemo via onDevModeChanged.
        addAndMakeVisible (devBtn);
        devMode = getAppSettings()->getBoolValue ("DevMode", false);
        devBtn.setToggleState (devMode, dontSendNotification);
        devBtn.setColour (ToggleButton::tickColourId, SYColSelected);
        devBtn.onClick = [this]
        {
            devMode = devBtn.getToggleState();
            getAppSettings()->setValue ("DevMode", devMode);
            getAppSettings()->saveIfNeeded();
            if (onDevModeChanged) onDevModeChanged (devMode);
        };
        addAndMakeVisible (labDevHint);
        labDevHint.setText (TRANS("Shows the \"Midi Setting\" tab: MIDI monitor and reverse-engineering tools."),
                            dontSendNotification);
        labDevHint.setColour (Label::textColourId, SYColLabel);
        labDevHint.setFont (Font (FontOptions (12.0f)));
        labDevHint.setMinimumHorizontalScale (1.0f);
        labDevHint.setJustificationType (Justification::topLeft);

        // Langue FR/EN : pilote les mappings LocalisedStrings (clés source = anglais, French.txt
        // traduit en français). Le changement s'applique pleinement au prochain démarrage.
        addAndMakeVisible (labLang);
        addAndMakeVisible (comboLang);
        // Noms de langue NON traduits (ils s'affichent pareil quelle que soit l'UI). Le "ç" passe
        // par un décodage UTF-8 EXPLICITE : dans ce projet les littéraux C++ doivent rester ASCII
        // (String(const char*) ne décode pas l'UTF-8 ici) -> accents uniquement via CharPointer_UTF8.
        comboLang.addItem (String (CharPointer_UTF8 ("Fran\xc3\xa7" "ais")), 1);
        comboLang.addItem ("English",  2);
        {
            const String lang = getAppSettings()->getValue ("Language",
                                    SystemStats::getUserLanguage().startsWith ("fr") ? "fr" : "en");
            comboLang.setSelectedId (lang == "fr" ? 1 : 2, dontSendNotification);
        }
        comboLang.onChange = [this]
        {
            const String lang = (comboLang.getSelectedId() == 1) ? "fr" : "en";
            getAppSettings()->setValue ("Language", lang);
            getAppSettings()->saveIfNeeded();
            syApplyLanguage (lang);
            NativeMessageBox::showMessageBoxAsync (MessageBoxIconType::InfoIcon,
                TRANS("Language"), TRANS("Restart the application to fully apply the language."));
        };

        // Onglet de démarrage : "Dernier utilisé" (suit CurrentTab) ou un onglet fixe.
        // L'id 1 = dernier utilisé ; id 2.. = onglet d'index 0.. (cf. MidiDemo au démarrage).
        addAndMakeVisible (labStartTab);
        addAndMakeVisible (comboStartTab);
        comboStartTab.addItem (TRANS("Last used"), 1);
        comboStartTab.addItem (TRANS("Setting"),   2);
        comboStartTab.addItem (TRANS("Librairie"), 3);
        comboStartTab.addItem (TRANS("Voice"),     4);
        comboStartTab.addItem (TRANS("Effects"),   5);
        comboStartTab.setSelectedId (jlimit (1, 5, getAppSettings()->getIntValue ("StartupTab", 1)),
                                     dontSendNotification);
        comboStartTab.onChange = [this]
        {
            getAppSettings()->setValue ("StartupTab", comboStartTab.getSelectedId());
            getAppSettings()->saveIfNeeded();
        };

        // Réinitialiser : thème + couleurs + canal MIDI aux valeurs par défaut (confirmation).
        addAndMakeVisible (btReset);
        btReset.onClick = [this]
        {
            NativeMessageBox::showOkCancelBox (MessageBoxIconType::QuestionIcon,
                TRANS("Default settings"),
                TRANS("Reset theme, colours and MIDI channel to their defaults?"),
                nullptr,
                ModalCallbackFunction::create ([this] (int r) { if (r == 1) resetDefaults(); }));
        };

        comboEngine.addItem("01",1);
        comboEngine.addItem("02",2);
        comboEngine.addItem("03",3);
        comboEngine.addItem("04",4);
        comboEngine.addItem("05",5);
        comboEngine.addItem("06",6);
        comboEngine.addItem("07",7);
        comboEngine.addItem("08",8);
        comboEngine.addItem("09",9);
        comboEngine.addItem("10",10);
        comboEngine.addItem("11",11);
        comboEngine.addItem("12",12);
        comboEngine.addItem("13",13);
        comboEngine.addItem("14",14);
        comboEngine.addItem("15",15);
        comboEngine.addItem("16",16);
        comboEngine.addItem("ALL",17);
        comboEngine.setSelectedId(1);
        comboEngine.addListener(this);
        comboModel.addItem("SY 77", 1);
        comboModel.addItem("TG 77", 2);
        comboModel.addItem("SY 99", 3);
        comboModel.addListener(this);

        addAndMakeVisible(labVersion);
        labVersion.setText (Sysex77::versionString(), dontSendNotification);
        labVersion.setJustificationType (Justification::centredRight);
        labVersion.setColour (Label::textColourId, SYColLabel);

        addAndMakeVisible(labTheme);
        addAndMakeVisible(comboTheme);
        addAndMakeVisible(btReloadThemes);
        btReloadThemes.onClick = [this] { reloadThemes(); };
        addAndMakeVisible(btThemeBuilder);
        btThemeBuilder.onClick = [this]
        {
            auto* tb = new ThemeBuilder();
            tb->onThemeSaved = [this] { reloadThemes(); };   // liste auto le thème sauvé
            DialogWindow::LaunchOptions o;
            o.content.setOwned (tb);
            o.dialogTitle = TRANS("Theme Builder");
            o.dialogBackgroundColour = SYPal.background;
            o.escapeKeyTriggersCloseButton = true;
            o.useNativeTitleBar = true;
            o.resizable = true;
            o.launchAsync();
        };

        // Système de thèmes : exporte les modèles intégrés (si absents), découvre tous les thèmes
        // (intégrés + dossier utilisateur) et peuple le sélecteur.
        syExportBuiltinThemes();
        themes = syDiscoverThemes();
        rebuildThemeCombo();
        comboTheme.addListener(this);

// initialisation et ouverture des propriétées
        initProperties();
        loadParams();
        
        
        setState();
        
    }
    ~ConfigPage()
    {
        comboEngine.removeListener(this);
        comboModel.removeListener(this);
        comboTheme.removeListener(this);
        saveParams(); //sauvegarde des paramètres
    }
    void getState() //Sauvegarde des Parametres
    {
        // Construction du XML
        XmlElement xml ("CONFIG");
        XmlElement* xmlColor = new XmlElement ("COLOR");
        XmlElement* xmlData = new XmlElement ("DATA");
        
        xmlColor->setAttribute ("Background", int(SYColBackground.getARGB()) );
        xmlColor->setAttribute ("Alternate", int(SYColAlt.getARGB()) );
        xmlColor->setAttribute ("Label", int(SYColLabel.getARGB()) );
        xmlColor->setAttribute ("Selected", int(SYColSelected.getARGB()) );
        xml.addChildElement(xmlColor);
        
        xmlData->setAttribute ("Model", int(comboModel.getSelectedId()));
        xmlData->setAttribute ("Theme", SYTheme);          // compat (0=sombre, 1=clair)
        xmlData->setAttribute ("ThemeName", SYPal.name);   // nom du thème actif (système de thèmes)
        xml.addChildElement(xmlData);
        
        appDirPath.createDirectory(); // s'assure que le dossier existe
        xml.writeTo(appDirPath.getChildFile("SYSEX77.xml"));
        
        
        if (! sender.send (adresseOscRepaint)) // [5]
            Logger::writeToLog ("OSC erreur");;
        
        
        
    }
    void setState() //Lecture des parametres
    {
        // Lit au même endroit que getState() écrit (corrige un bug de chemin :
        // l'ancienne version cherchait dans ~/Documents au lieu de appDirPath).
        auto tableFile = appDirPath.getChildFile("SYSEX77.xml");

        SYTheme = 0;
        String themeName;
        if (tableFile.exists())
        {
            tutorialData = XmlDocument::parse(tableFile);
            if (tutorialData != nullptr)
                if (auto* xmlData = tutorialData->getChildByName("DATA"))
                {
                    SYTheme   = xmlData->getIntAttribute("Theme", 0);
                    themeName = xmlData->getStringAttribute("ThemeName", "");
                }
        }

        // Applique le thème par NOM si connu (système de thèmes) ; sinon repli sur 0=sombre / 1=clair.
        int idx = themeName.isNotEmpty() ? syApplyThemeByName(themes, themeName) : -1;
        if (idx < 0)
        {
            applySyTheme(SYTheme);
            for (int i = 0; i < (int) themes.size(); ++i)   // resélectionne le combo sur la palette appliquée
                if (themes[(size_t) i].name == SYPal.name) { idx = i; break; }
        }
        if (idx >= 0)
            comboTheme.setSelectedItemIndex(idx, dontSendNotification);

        // Thème « Custom » uniquement : ré-applique les couleurs personnalisées sauvées par-dessus
        // (pour un thème nommé connu, on garde la palette propre -> plus de surcharge périmée).
        if (themeName == "Custom" && tutorialData != nullptr)
            if (auto* xmlColor = tutorialData->getChildByName ("COLOR"))
            {
                auto col = [&] (const char* a, juce::Colour def)
                { return xmlColor->hasAttribute (a) ? juce::Colour ((juce::uint32) xmlColor->getIntAttribute (a)) : def; };
                SYColBackground = col ("Background", SYColBackground);
                SYColAlt        = col ("Alternate",  SYColAlt);
                SYColLabel      = col ("Label",      SYColLabel);
                SYColSelected   = col ("Selected",   SYColSelected);
                pushAliasesToPalette();
            }

    }
    void 	comboBoxChanged (ComboBox *comboBoxThatHasChanged) override
    {
        Logger::writeToLog("ConfigPage: comboBox Listener");
        // Device number global (canal). id 1..16 = device, id 17 = "ALL" (réception omni).
        {
            const int id = comboEngine.getSelectedId();
            if (id == 17)
                sysexReceiveOmni = true;
            else if (id >= 1 && id <= 16)
            {
                sysexDeviceNumber = id;
                sysexReceiveOmni  = false;
            }
        }

        sysexModel = comboModel.getSelectedItemIndex();

        if (comboBoxThatHasChanged == &comboTheme)
        {
            const int idx = comboTheme.getSelectedItemIndex();
            if (idx >= 0 && idx < (int) themes.size())
                applySyPalette (themes[(size_t) idx]);
            updateTabsAndRepaint();
        }

        getState();
    }

    // Injection des contrôles MIDI propriété de MidiDemo (un Component n'a qu'un seul parent) :
    // sélecteurs d'interface In/Out + bouton "Bulk Protect". Appelé une fois après construction.
    void attachMidiInterface (Label& inLabel, Component& inSel, Label& outLabel, Component& outSel,
                              TextButton& bulk)
    {
        midiInLab = &inLabel; midiInSel = &inSel; midiOutLab = &outLabel; midiOutSel = &outSel;
        bulkBtn = &bulk;
        addAndMakeVisible (inLabel);  addAndMakeVisible (inSel);
        addAndMakeVisible (outLabel); addAndMakeVisible (outSel);
        addAndMakeVisible (bulk);
        resized();
    }

    // Helpers d'UI partagés -------------------------------------------------
    void rebuildThemeCombo()
    {
        comboTheme.clear (dontSendNotification);
        for (int i = 0; i < (int) themes.size(); ++i)
            comboTheme.addItem (themes[(size_t) i].name, i + 1);
    }
    void updateTabsAndRepaint()
    {
        if (auto* tabs = findParentComponentOfClass<TabbedComponent>())
            for (int i = 0; i < tabs->getNumTabs(); ++i)
                tabs->setTabBackgroundColour (i, SYColBackground);
        if (auto* top = getTopLevelComponent())
            top->sendLookAndFeelChange();   // repeint + déclenche lookAndFeelChanged (couleurs figées)
    }
    // Re-scanne le dossier de thèmes (hot-reload) en conservant le thème courant si possible.
    void reloadThemes()
    {
        const auto current = SYPal.name;
        themes = syDiscoverThemes();
        rebuildThemeCombo();
        int idx = 0;
        for (int i = 0; i < (int) themes.size(); ++i)
            if (themes[(size_t) i].name == current) { idx = i; break; }
        comboTheme.setSelectedItemIndex (idx, dontSendNotification);
        if (! themes.empty())
            applySyPalette (themes[(size_t) idx]);
        updateTabsAndRepaint();
        getState();
    }

    // Affiche le résultat du test de connexion (appelé par MidiDemo : réponse identité ou timeout).
    // ok = vert (succès) ; sinon couleur de label neutre. Sûr : appelé depuis le message loop.
    void setConnStatus (const String& text, bool ok)
    {
        labConnStatus.setColour (Label::textColourId, ok ? SYColSelected : SYColLabel);
        labConnStatus.setText (text, dontSendNotification);
    }

    // Réinitialise thème + couleurs + canal MIDI aux valeurs par défaut (Dark Orange, device 1).
    void resetDefaults()
    {
        applySyTheme (0);                       // palette par défaut (Dark Orange historique)
        for (int i = 0; i < (int) themes.size(); ++i)
            if (themes[(size_t) i].name == SYPal.name)
            { comboTheme.setSelectedItemIndex (i, dontSendNotification); break; }

        sysexDeviceNumber = 1;
        sysexReceiveOmni  = false;
        comboEngine.setSelectedId (1, dontSendNotification);

        followSynth = false;
        followBtn.setToggleState (false, dontSendNotification);
        getAppSettings()->setValue ("FollowSynth", false);
        getAppSettings()->saveIfNeeded();

        updateTabsAndRepaint();
        getState();
        saveParams();
    }

    // Carte : fond surface + contour panelBorder + titre accentué. Cohérent avec ModernLookAndFeel.
    void paintCard (Graphics& g, Rectangle<int> r, const String& title)
    {
        if (r.isEmpty()) return;
        auto rf = r.toFloat().reduced (0.5f);
        g.setColour (SYPal.surface.withAlpha (SYPal.dark ? 0.55f : 0.85f));
        g.fillRoundedRectangle (rf, SyMetrics::cornerLg);
        g.setColour (SYPal.panelBorder);
        g.drawRoundedRectangle (rf, SyMetrics::cornerLg, SyMetrics::stroke);
        g.setColour (SYPal.accent);
        g.setFont (Font (FontOptions (12.0f, Font::bold)));
        g.drawText (title.toUpperCase(), r.reduced (14, 0).withTrimmedTop (9).withHeight (16),
                    Justification::topLeft, false);
    }
    void paint (Graphics& g) override
    {
        paintCard (g, cardMachine, TRANS("Machine"));
        paintCard (g, cardMidi,    "MIDI");
        paintCard (g, cardTheme,   TRANS("Appearance"));
        paintCard (g, cardGeneral, TRANS("General"));
        paintCard (g, cardDev,     TRANS("Developer"));
    }
    void resized() override
    {
        auto area = getLocalBounds().reduced (16);
        labVersion.setBounds (area.removeFromBottom (18));
        area.removeFromBottom (8);

        const int gap = 16, pad = 14, rowH = 26, labH = 18, vgap = 8, titleH = 26;
        const int colW = (area.getWidth() - gap) / 2;
        auto left  = area.removeFromLeft (colW);
        area.removeFromLeft (gap);
        auto right = area.removeFromLeft (colW);

        // ----- Colonne gauche : Machine + MIDI -----
        cardMachine = left.removeFromTop (titleH + rowH + pad);
        {
            auto c = cardMachine.reduced (pad);
            c.removeFromTop (titleH - pad);
            comboModel.setBounds (c.removeFromTop (rowH));
        }
        left.removeFromTop (gap);

        const int selH = 104;
        const int midiH = titleH + labH + rowH + vgap + (labH + selH) + vgap
                          + rowH + rowH + vgap + rowH + labH + pad;   // +Bulk +Test +statut
        cardMidi = left.removeFromTop (midiH);
        {
            auto c = cardMidi.reduced (pad);
            c.removeFromTop (titleH - pad);
            labEngine.setBounds (c.removeFromTop (labH));
            comboEngine.setBounds (c.removeFromTop (rowH));
            c.removeFromTop (vgap);
            auto io = c.removeFromTop (labH + selH);
            auto inCol  = io.removeFromLeft ((io.getWidth() - 8) / 2);
            io.removeFromLeft (8);
            auto outCol = io;
            if (midiInLab)  midiInLab->setBounds  (inCol.removeFromTop (labH));
            if (midiInSel)  midiInSel->setBounds  (inCol);
            if (midiOutLab) midiOutLab->setBounds (outCol.removeFromTop (labH));
            if (midiOutSel) midiOutSel->setBounds (outCol);
            c.removeFromTop (vgap);
            followBtn.setBounds (c.removeFromTop (rowH));
            if (bulkBtn) bulkBtn->setBounds (c.removeFromTop (rowH));
            c.removeFromTop (vgap);
            btTestConn.setBounds (c.removeFromTop (rowH));
            labConnStatus.setBounds (c.removeFromTop (labH));
        }

        // ----- Colonne droite : Apparence + General + Développeur -----
        const int themeH = titleH + labH + rowH + vgap + rowH + vgap + labH + rowH + pad; // +langue
        cardTheme = right.removeFromTop (themeH);
        {
            auto c = cardTheme.reduced (pad);
            c.removeFromTop (titleH - pad);
            labTheme.setBounds (c.removeFromTop (labH));
            comboTheme.setBounds (c.removeFromTop (rowH));
            c.removeFromTop (vgap);
            auto btnRow = c.removeFromTop (rowH);
            btReloadThemes.setBounds (btnRow.removeFromLeft ((btnRow.getWidth() - 8) / 2));
            btnRow.removeFromLeft (8);
            btThemeBuilder.setBounds (btnRow);
            c.removeFromTop (vgap);
            labLang.setBounds (c.removeFromTop (labH));
            comboLang.setBounds (c.removeFromTop (rowH));
        }
        right.removeFromTop (gap);

        const int genH = titleH + labH + rowH + vgap + rowH + pad;
        cardGeneral = right.removeFromTop (genH);
        {
            auto c = cardGeneral.reduced (pad);
            c.removeFromTop (titleH - pad);
            labStartTab.setBounds (c.removeFromTop (labH));
            comboStartTab.setBounds (c.removeFromTop (rowH));
            c.removeFromTop (vgap);
            btReset.setBounds (c.removeFromTop (rowH));
        }
        right.removeFromTop (gap);

        const int devH = titleH + rowH + vgap + 36 + pad;
        cardDev = right.removeFromTop (devH);
        {
            auto c = cardDev.reduced (pad);
            c.removeFromTop (titleH - pad);
            devBtn.setBounds (c.removeFromTop (rowH));
            c.removeFromTop (vgap);
            labDevHint.setBounds (c.removeFromTop (36));
        }
    }
    void initProperties()
    {
        getAppProps(); // initialise le stockage partagé (cf. AppSettings.h)
    }
    void loadParams()
    {
        
     //   mySlider.setValue( getAppProps().getUserSettings()->getIntValue("mySlider"));
        Logger::writeToLog("Load parametres:");
        Logger::writeToLog(String(getAppProps().getUserSettings()->getIntValue("Model")));
         Logger::writeToLog(String(getAppProps().getUserSettings()->getIntValue(IDs::COMMONFOOT)));
        comboModel.setSelectedId(getAppProps().getUserSettings()->getIntValue("Model"));
        SYModel = getAppProps().getUserSettings()->getIntValue("Model");
        valueTreeVoice.setProperty(IDs::COMMONFOOT, getAppProps().getUserSettings()->getIntValue(IDs::COMMONFOOT), nullptr);

        // Device number (canal) global
        sysexDeviceNumber = jlimit (1, 16, getAppProps().getUserSettings()->getIntValue ("Device", 1));
        sysexReceiveOmni  = getAppProps().getUserSettings()->getBoolValue ("Omni", false);
        comboEngine.setSelectedId (sysexReceiveOmni ? 17 : sysexDeviceNumber, dontSendNotification);
        
    }
    void saveParams()
    {
              Logger::writeToLog("Save parametres:");
        Logger::writeToLog(comboModel.getSelectedIdAsValue().toString());
        getAppProps().getUserSettings()->setValue("Model", comboModel.getSelectedIdAsValue());
     getAppProps().getUserSettings()->setValue("commonFoot", valueTreeVoice.getPropertyAsValue(IDs::COMMONFOOT, nullptr));
        getAppProps().getUserSettings()->setValue ("Device", sysexDeviceNumber);
        getAppProps().getUserSettings()->setValue ("Omni",   sysexReceiveOmni);
        getAppProps().getUserSettings()->saveIfNeeded();
        
   
     //   getAppProps().getUserSettings()->setValue("mySlider", mySlider.getValue());
    }
    std::unique_ptr<XmlElement> tutorialData;
    Label labEngine {"","MIDI channel (Device 1-16 / ALL)"};
    ComboBox comboEngine;
    Label labTheme {"", TRANS("Theme")};
    ComboBox comboTheme;
    TextButton btReloadThemes {TRANS("Reload themes")};
    TextButton btThemeBuilder {TRANS("Theme Builder")};
    std::vector<SyPalette> themes;
    Label labVersion;

    // Carte MIDI : toggle "Suivre le synthé" + sélecteurs d'interface et bouton Bulk Protect
    // injectés par MidiDemo (cf. attachMidiInterface). Test de connexion + statut.
    ToggleButton followBtn { TRANS("Follow synth") };
    Label*      midiInLab  = nullptr;
    Component*  midiInSel   = nullptr;
    Label*      midiOutLab = nullptr;
    Component*  midiOutSel  = nullptr;
    TextButton* bulkBtn     = nullptr;
    TextButton  btTestConn { TRANS("Test connection") };
    Label       labConnStatus;
    std::function<void()> onTestConnection;       // câblé par MidiDemo : envoie un Identity Request

    // Carte Apparence : langue FR/EN
    Label    labLang { "", TRANS("Language") };
    ComboBox comboLang;

    // Carte General : onglet de démarrage + réinitialisation
    Label      labStartTab { "", TRANS("Startup tab") };
    ComboBox   comboStartTab;
    TextButton btReset { TRANS("Default settings") };

    // Carte Développeur
    ToggleButton devBtn { TRANS("Developer mode") };
    Label labDevHint;
    std::function<void(bool)> onDevModeChanged;   // câblé par MidiDemo : ajoute/retire l'onglet "Midi Setting"

    // Cadres des cartes (calculés dans resized, dessinés dans paint).
    Rectangle<int> cardMachine, cardMidi, cardTheme, cardGeneral, cardDev;

    ComboBox    comboModel;
    SysexBusSender sender;  // [2]

};

