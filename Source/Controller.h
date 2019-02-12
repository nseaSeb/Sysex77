/*
  ==============================================================================

    Controller.h
    Created: 13 Nov 2018 9:49:27am
    Author:  Sébastien Portrait

  ==============================================================================
 MidiMessage m (MidiMessage::noteOn (midiChannel, midiNoteNumber, velocity));
 m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
 sendToOutputs (m);
*/

#pragma once

//==============================================================================
struct ControllerPage   : public Component, public ComboBox::Listener
{
    ControllerPage()
    {
        loadData();
        Logger::writeToLog("XML Num" + String(numRows));
        addAndMakeVisible(labelFoot);
        addAndMakeVisible(labelMod);
        
        addAndMakeVisible (comboFoot);
        
        comboFoot.setEditableText (false);
        comboFoot.setJustificationType (Justification::left);
        comboFoot.addListener(this);
        
        addAndMakeVisible (comboMod);
        
        comboMod.setEditableText (false);
        comboMod.setJustificationType (Justification::left);
        comboMod.addListener(this);
        
        for (int i = 1; i < numRows; ++i)
            //  comboBox.addItem ("combo box item " + String (i), i);
            //   forEachXmlChildElement (*dataList, "name")
        {
            if (auto* rowElement = dataList->getChildElement(i))
            {
                auto text = rowElement->getStringAttribute (getAttributeNameForColumnId (1));
                text = text + " " + rowElement->getStringAttribute (getAttributeNameForColumnId (2));
                comboFoot.addItem (text,i);
                comboMod.addItem (text,i);
            }
            
            
        }
        
        
        comboFoot.setSelectedId (1);
        comboMod.setSelectedId(2);
        // specify here where to send OSC messages to: host URL and UDP port number
        if (! sender.connect ("127.0.0.1", 9001)) // [4]
            Logger::writeToLog ("Error: could not connect to UDP port 9001.");

    }
    
    ~ControllerPage()
    {
        comboMod.removeListener(this);
    }
    void comboBoxChanged	(	ComboBox * 	comboBoxThatHasChanged	) override
    {
        Logger::writeToLog ("combo changed");
        //MidiMessage m (MidiMessage::noteOn (1, 64, 120));
        // create and send an OSC message with an address and a float value:
        if (! sender.send (adresseOscFoot, (int) comboFoot.getSelectedItemIndex()+1)) // [5]
                    Logger::writeToLog ("OSC erreur");;
        if (! sender.send (adresseOscMod, (int) comboMod.getSelectedItemIndex()+1)) // [5]
            Logger::writeToLog ("OSC erreur");;
    }

    void resized() override
    {
        labelFoot.setBounds(10, 24, getWidth()/2, 24);
        labelMod.setBounds(10, getHeight()/2 + 24, getWidth()/2, 24);
        comboFoot.setBounds (10, 48, getWidth()/2, 24);
        comboMod.setBounds (10, getHeight()/2 + 48, getWidth()/2, 24);
    }

    
    ComboBox comboFoot { "Pedale de sustain"};
    ComboBox comboMod  { "Molette de modulation" };
    Label   labelFoot { "","Pedale de sustain :"};
    Label   labelMod { "","Molette de modulation :"};
  OSCSender sender;  // [2]
    
    //==============================================================================

    void loadData()
    {
       
        
        auto tableFile = BinaryData::TableData_xml;
        
        tutorialData.reset (XmlDocument::parse (tableFile));
            
            dataList   = tutorialData->getChildByName ("DATA");
            columnList = tutorialData->getChildByName ("HEADERS");
            
            numRows = dataList->getNumChildElements();
   
    }
    
    String getAttributeNameForColumnId (const int columnId) const
    {
        forEachXmlChildElement (*columnList, columnXml)
        {
            if (columnXml->getIntAttribute ("columnId") == columnId)
                return columnXml->getStringAttribute ("name");
        }
        
        return {};
    }
public:
    std::unique_ptr<XmlElement> tutorialData;
    XmlElement* columnList = nullptr;
    XmlElement* dataList = nullptr;
    int numRows = 0;

};
//==============================================================================

