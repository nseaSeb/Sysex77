/*
 ==============================================================================
 
 Config.h
 Created: 13 Nov 2018 9:48:57am
 Author:  Sébastien Portrait
 
 */


#pragma once

#include "AppSettings.h"

//==============================================================================

//==============================================================================
struct ConfigPage   : public Component, public ChangeListener, public Button::Listener, public ComboBox::Listener

{
    
    ConfigPage()
    {
        setOpaque(false);
        addAndMakeVisible(btColBack);
        addAndMakeVisible(btColAlt);
        addAndMakeVisible(comboModel);
        addAndMakeVisible(btColLab);
        addAndMakeVisible(btColSel);
        addAndMakeVisible(labEngine);
        addAndMakeVisible(comboEngine);
        
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
        comboTheme.addItem(TRANS("Dark"), 1);
        comboTheme.addItem(TRANS("Light"), 2);
        comboTheme.setSelectedId(1, dontSendNotification);
        comboTheme.addListener(this);

        btColBack.addListener(this);
        btColAlt.addListener(this);
        btColLab.addListener(this);
        btColSel.addListener(this);
        
// initialisation et ouverture des propriétées
        initProperties();
        loadParams();
        
        
        setState();
        
    }
    ~ConfigPage()
    {
        comboEngine.removeListener(this);
        comboModel.removeListener(this);
        btColBack.removeListener(this);
        btColAlt.removeListener(this);
        btColLab.removeListener(this);
        btColSel.removeListener(this);
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
        xmlData->setAttribute ("Theme", SYTheme);
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
        if (tableFile.exists())
        {
            tutorialData = XmlDocument::parse(tableFile);
            if (tutorialData != nullptr)
                if (auto* xmlData = tutorialData->getChildByName("DATA"))
                    SYTheme = xmlData->getIntAttribute("Theme", 0);
        }

        comboTheme.setSelectedId(SYTheme + 1, dontSendNotification);

        // Le thème pilote la palette de façon déterministe (les anciennes couleurs
        // personnalisées ne sont plus relues : un thème = toujours le même rendu).
        applySyTheme(SYTheme);

        // Restaure les couleurs PERSONNALISÉES (thème custom) si elles ont été sauvées,
        // par-dessus la palette du thème — sinon le réglage serait à refaire à chaque ouverture.
        if (tutorialData != nullptr)
            if (auto* xmlColor = tutorialData->getChildByName ("COLOR"))
            {
                auto col = [&] (const char* a, juce::Colour def)
                { return xmlColor->hasAttribute (a) ? juce::Colour ((juce::uint32) xmlColor->getIntAttribute (a)) : def; };
                SYColBackground = col ("Background", SYColBackground);
                SYColAlt        = col ("Alternate",  SYColAlt);
                SYColLabel      = col ("Label",      SYColLabel);
                SYColSelected   = col ("Selected",   SYColSelected);
                setColour (ResizableWindow::backgroundColourId, SYColBackground);
                setColour (Slider::ColourIds::trackColourId, SYColSelected);
            }

        // Mise à jour des boutons de couleur
        btColAlt.setColour(TextButton::buttonColourId, SYColAlt);
        btColBack.setColour(TextButton::buttonColourId, SYColBackground);
        btColLab.setColour(TextButton::buttonColourId, SYColLabel);
        btColSel.setColour(TextButton::buttonColourId, SYColSelected);
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
            applySyTheme (comboTheme.getSelectedItemIndex());
            btColAlt .setColour (TextButton::buttonColourId, SYColAlt);
            btColBack.setColour (TextButton::buttonColourId, SYColBackground);
            btColLab .setColour (TextButton::buttonColourId, SYColLabel);
            btColSel .setColour (TextButton::buttonColourId, SYColSelected);
            // met à jour le fond des onglets en direct
            if (auto* tabs = findParentComponentOfClass<TabbedComponent>())
                for (int i = 0; i < tabs->getNumTabs(); ++i)
                    tabs->setTabBackgroundColour (i, SYColBackground);
            if (auto* top = getTopLevelComponent())
                top->repaint();
        }

        getState();
    }
    void buttonClicked (Button* button) override
    {
        Logger::writeToLog("ConfigPage: clicked");
        
        // Crée un unique_ptr pour le ColourSelector
        auto colourSelector = std::make_unique<ColourSelector>();
        colourSelector->setName("Colour");
        colourSelector->addChangeListener(this);
        colourSelector->setCurrentColour(button->findColour(TextButton::buttonColourId));
        colourSelector->setColour(ColourSelector::backgroundColourId, Colours::transparentBlack);
        colourSelector->setSize(300, 400);
        
        // Configure le nom selon le bouton cliqué
        if (button == &btColBack)
            colourSelector->setName("colBack");
        else if (button == &btColAlt)
            colourSelector->setName("colAlt");
        else if (button == &btColLab)
            colourSelector->setName("colLab");
        else if (button == &btColSel)
            colourSelector->setName("colSel");
        
        // Transfère la propriété du unique_ptr à launchAsynchronously
        CallOutBox::launchAsynchronously(std::move(colourSelector),
                                       button->getScreenBounds(),
                                       nullptr);
    }
    
    void changeListenerCallback (ChangeBroadcaster* source) override
    {
        Logger::writeToLog( "ConfigPage: Change Listener");
        if (auto* cs = dynamic_cast<ColourSelector*> (source))
        {
            //  btColBack.setColour(ColourSelector::backgroundColourId, Colours::transparentBlack);
            
            
            //       editor.applyNewValue (getAsString (cs->getCurrentColour(), true));
            
            if (cs->getName() == "colAlt")
            {
                SYColAlt = cs->getCurrentColour();
                
                btColAlt.setColour(TextButton::ColourIds::buttonColourId, SYColAlt);
            }
            else if (cs->getName() == "colBack")
            {
                setColour(ResizableWindow::backgroundColourId, cs->getCurrentColour());
                SYColBackground = cs->getCurrentColour();
                btColBack.setColour(TextButton::ColourIds::buttonColourId, SYColBackground);
                
            }
            else if (cs->getName() == "colLab")
            {
                SYColLabel = cs->getCurrentColour();
                btColLab.setColour(TextButton::ColourIds::buttonColourId, SYColLabel);
                
            }
            else if (cs->getName() == "colSel")
            {
                setColour(Slider::ColourIds::trackColourId, cs->getCurrentColour());
                SYColSelected = cs->getCurrentColour();
                btColSel.setColour(TextButton::ColourIds::buttonColourId, SYColSelected);
                
            }
        }
        
        getState();
        
    }
    void resized() override
    {
        btColBack.setBounds(10,58,getWidth()/2 - 20,24);
        btColAlt.setBounds(10,106, getWidth()/2 - 20, 24 );
        btColLab.setBounds(10,154, getWidth()/2 - 20, 24 );
        btColSel.setBounds(10,202, getWidth()/ 2 - 20, 24 );
        comboModel.setBounds(10, 24, getWidth()/2 - 20, 24);
        labEngine.setBounds(getWidth()/2 +10, 24, getWidth()/2 - 20, 24);
        comboEngine.setBounds(getWidth()/2 +10, 58, getWidth()/2 - 20, 24);
        labTheme.setBounds(getWidth()/2 +10, 92, getWidth()/2 - 20, 20);
        comboTheme.setBounds(getWidth()/2 +10, 112, getWidth()/2 - 20, 24);
        labVersion.setBounds(10, getHeight() - 22, getWidth() - 20, 18);
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
    TextButton btColBack {TRANS("Background Application color")};
    TextButton btColAlt {TRANS("Alternate background color")};
    TextButton btColLab {TRANS("Label background color")};
    TextButton btColSel {TRANS("Text selected color")};
    Label labEngine {"","Canal MIDI (Device 1-16 / ALL)"};
    ComboBox comboEngine;
    Label labTheme {"", TRANS("Theme")};
    ComboBox comboTheme;
    Label labVersion;

    ComboBox    comboModel;
    SysexBusSender sender;  // [2]
    
};

