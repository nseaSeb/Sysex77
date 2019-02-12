/*
 ==============================================================================
 
 Config.h
 Created: 13 Nov 2018 9:48:57am
 Author:  Sébastien Portrait
 
 */


#pragma once

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
        
        btColBack.addListener(this);
        btColAlt.addListener(this);
        btColLab.addListener(this);
        btColSel.addListener(this);
        
// initialisation et ouverture des propriétées
        initProperties();
        loadParams();
        
        if (! sender.connect ("127.0.0.1", 9001)) // [4]
            Logger::writeToLog ("Error: could not connect to UDP port 9001.");
        
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
        xml.addChildElement(xmlData);
        
        xml.writeToFile(appDirPath.getFullPathName() + "/SYSEX77.xml", "");
        
        
        if (! sender.send (adresseOscRepaint)) // [5]
            Logger::writeToLog ("OSC erreur");;
        
        
        
    }
    void setState() //Lecture des parametres
    {
        XmlElement* dataList = nullptr;
        auto dir = File::getSpecialLocation (File::userDocumentsDirectory);
        
        int numTries = 0;
        
        while (! dir.getChildFile ("Sysex77").exists() && numTries++ < 15)
            dir = dir.getParentDirectory();
        Logger::writeToLog("dir exist");
        
        auto tableFile = dir.getChildFile ("Sysex77").getChildFile ("SYSEX77.xml");
        Logger::writeToLog(tableFile.getFullPathName());
        
        if (tableFile.exists())
        {
            Logger::writeToLog("file exist");
            tutorialData.reset (XmlDocument::parse (tableFile));
            // Parsing des couleurs
            dataList   = tutorialData->getChildByName ("COLOR");
        }
        if(dataList) //Verifie que les data existent
        {
            Colour color;
            
            color = Colour(dataList->getIntAttribute("Alternate"));
            SYColAlt = color;
            color = Colour(dataList->getIntAttribute("Background"));
            SYColBackground = color;
            
            color = Colour(dataList->getIntAttribute("Label"));
            SYColLabel = color;
            
            color = Colour(dataList->getIntAttribute("Selected"));
            SYColSelected = color;
            //setColour(ResizableWindow::backgroundColourId, color);
            //Parsing des data
            dataList = tutorialData->getChildByName("DATA");
            if(dataList)
            {
           //     SYModel = dataList->getIntAttribute("Model");
                
            }
        }
        else
        {
            SYColAlt = Colours::black;
            SYColLabel = Colours::grey;
            SYColBackground = Colours::black;
            SYColSelected = Colours::darkorange;
        //    SYModel = 1;
        }
     //   comboModel.setSelectedId(SYModel);
        btColAlt.setColour(TextButton::ColourIds::buttonColourId,SYColAlt);
        btColBack.setColour(TextButton::ColourIds::buttonColourId,SYColBackground);
        btColLab.setColour(TextButton::ColourIds::buttonColourId,SYColLabel);
        btColSel.setColour(TextButton::ColourIds::buttonColourId,SYColSelected);
        
    }
    void 	comboBoxChanged (ComboBox *comboBoxThatHasChanged) override
    {
        Logger::writeToLog("ConfigPage: comboBox Listener");
        sysexEngine = comboEngine.getSelectedItemIndex();
        if(sysexEngine == 16)
            sysexEngine = 0x10;

        
        sysexModel = comboModel.getSelectedItemIndex();
        getState();
    }
    void buttonClicked (Button* button) override
    {
        
        Logger::writeToLog("ConfigPage: clicked");
        
        auto* colourSelector = new ColourSelector();
        colourSelector->setName ("Colour");
        colourSelector->addChangeListener (this);
        colourSelector->setCurrentColour (button->findColour (TextButton::buttonColourId));
        colourSelector->setColour (ColourSelector::backgroundColourId, Colours::transparentBlack);
        colourSelector->setSize (300, 400);
        CallOutBox::launchAsynchronously (colourSelector, button->getScreenBounds(), nullptr);
        
        if (button == &btColBack)
        {
            colourSelector->setName("colBack");
        }
        else if (button == &btColAlt)
        {
            colourSelector->setName("colAlt");
        }
        else if (button == &btColLab)
        {
            colourSelector->setName("colLab");
        }
        else if (button == &btColSel)
        {
            colourSelector->setName("colSel");
        }
        //   btColBack.setColour(ColourSelector::backgroundColourId, Colours::transparentBlack);
        
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
        
    }
    void initProperties()
    {
        PropertiesFile::Options options;
        options.applicationName = ProjectInfo::projectName;
        options.filenameSuffix = ".settings";
        options.osxLibrarySubFolder = "Application Support";
        options.folderName = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("SYSEX77").getFullPathName();
        options.storageFormat = PropertiesFile::storeAsXML;
        
        props.setStorageParameters(options);
    }
    void loadParams()
    {
        
     //   mySlider.setValue( props.getUserSettings()->getIntValue("mySlider"));
        Logger::writeToLog("Load parametres:");
        Logger::writeToLog(String(props.getUserSettings()->getIntValue("Model")));
         Logger::writeToLog(String(props.getUserSettings()->getIntValue(IDs::COMMONFOOT)));
        comboModel.setSelectedId(props.getUserSettings()->getIntValue("Model"));
        SYModel = props.getUserSettings()->getIntValue("Model");
        valueTreeVoice.setProperty(IDs::COMMONFOOT, props.getUserSettings()->getIntValue(IDs::COMMONFOOT), nullptr);
        
    }
    void saveParams()
    {
              Logger::writeToLog("Save parametres:");
        Logger::writeToLog(comboModel.getSelectedIdAsValue().toString());
        props.getUserSettings()->setValue("Model", comboModel.getSelectedIdAsValue());
     props.getUserSettings()->setValue("commonFoot", valueTreeVoice.getPropertyAsValue(IDs::COMMONFOOT, nullptr));
        
   
     //   props.getUserSettings()->setValue("mySlider", mySlider.getValue());
    }
    std::unique_ptr<XmlElement> tutorialData;
    TextButton btColBack {TRANS("Background Application color")};
    TextButton btColAlt {TRANS("Alternate background color")};
    TextButton btColLab {TRANS("Label background color")};
    TextButton btColSel {TRANS("Text selected color")};
    Label labEngine {"",TRANS("Machine Engine")};
    ComboBox comboEngine;
    
    ApplicationProperties props;  // object pour sauver les paramètres
    ComboBox    comboModel;
    OSCSender sender;  // [2]
    
};

