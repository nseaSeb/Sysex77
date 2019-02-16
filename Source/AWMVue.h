/*
  ==============================================================================

    AWMVue.h
    Created: 10 Feb 2019 11:25:20am
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once
//==============================================================================

class TableTutorialComponent    : public Component, public Value::Listener,
public TableListBoxModel
{
public:
    TableTutorialComponent()
    {
        loadData();
        addAndMakeVisible (table);
        
        table.setColour (ListBox::outlineColourId, Colours::grey);
        table.setOutlineThickness (1);
        //       table.getHeader().addColumn("NAME", 1, getWidth());
        valueSysexIn.addListener(this); //Listen to the sysex in
        if (! sender.connect ("127.0.0.1", 9001)) // [4]
            Logger::writeToLog ("Error: could not connect to UDP port 9001.");
        
        if (columnList != nullptr)
        {
            forEachXmlChildElement (*columnList, columnXml)
            {
                table.getHeader().addColumn (TRANS(columnXml->getStringAttribute ("name")),
                                             columnXml->getIntAttribute ("columnId"),
                                             columnXml->getIntAttribute ("width"),
                                             50,
                                             400,
                                             TableHeaderComponent::defaultFlags);
            }
        }
        
        table.getHeader().setSortColumnId (1, true);
        table.getHeader().setColumnVisible (8, false);
        
        table.setMultipleSelectionEnabled (true);
    }
    ~TableTutorialComponent()
    {
        valueSysexIn.removeListener(this);
    }
    void valueChanged(Value & value) override
    {
        
        Logger::writeToLog("midislider value change");
        int val = value.getValue()[0];
        if(sysexData[3] == val)
        {
            val = value.getValue()[1];
            if(sysexData[4] ==  val)
            {
                val = value.getValue()[2];
                if(sysexData[5] ==  val)
                {
                    val = value.getValue()[3];
                    if(sysexData[6] ==  val)
                    {
                        val = value.getValue()[5] ;
                        val += 1;
                        // if(sysexData[7] ==  val)
                        
                        rowSelected = val;
                        
                        
                        
                        repaint();
                        
                        
                        
                    }
                }
            }
        }
        
        
        
        
    }
    
    int getNumRows() override
    {
        return numRows;
    }
    
    void paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override
    {
        auto alternateColour = getLookAndFeel().findColour (ListBox::backgroundColourId)
        .interpolatedWith (getLookAndFeel().findColour (ListBox::textColourId), 0.03f);
        if (rowIsSelected)
            g.fillAll (Colours::lightblue);
        else if (rowNumber % 2)
            g.fillAll (alternateColour);
    }
    
    void paintCell (Graphics& g, int rowNumber, int columnId,
                    int width, int height, bool rowIsSelected) override
    {
        g.setColour (rowIsSelected ? Colours::darkblue : getLookAndFeel().findColour (ListBox::textColourId));
        g.setFont (font);
        
        if (auto* rowElement = dataList->getChildElement (rowNumber))
        {
            auto text = rowElement->getStringAttribute (getAttributeNameForColumnId (columnId));
            
            g.drawText (text, 2, 0, width - 4, height, Justification::centredLeft, true);
        }
        
        g.setColour (getLookAndFeel().findColour (ListBox::backgroundColourId));
        g.fillRect (width - 1, 0, 1, height);
    }
    
    void sortOrderChanged (int newSortColumnId, bool isForwards) override
    {
        if (newSortColumnId != 0)
        {
            TutorialDataSorter sorter (getAttributeNameForColumnId (newSortColumnId), isForwards);
            dataList->sortChildElements (sorter);
            
            table.updateContent();
        }
    }
    
    Component* refreshComponentForCell (int rowNumber, int columnId, bool /*isRowSelected*/,
                                        Component* existingComponentToUpdate) override
    {
        if (columnId == 9)
        {
            auto* selectionBox = static_cast<SelectionColumnCustomComponent*> (existingComponentToUpdate);
            
            if (selectionBox == nullptr)
                selectionBox = new SelectionColumnCustomComponent (*this);
            
            selectionBox->setRowAndColumn (rowNumber, columnId);
            return selectionBox;
        }
        
        if (columnId == 8)
        {
            auto* textLabel = static_cast<EditableTextCustomComponent*> (existingComponentToUpdate);
            
            if (textLabel == nullptr)
                textLabel = new EditableTextCustomComponent (*this);
            
            textLabel->setRowAndColumn (rowNumber, columnId);
            return textLabel;
        }
        
        jassert (existingComponentToUpdate == nullptr);
        return nullptr;
    }
    
    int getColumnAutoSizeWidth (int columnId) override
    {
        if (columnId == 9)
            return 50;
        
        int widest = 32;
        
        for (auto i = getNumRows(); --i >= 0;)
        {
            if (auto* rowElement = dataList->getChildElement (i))
            {
                auto text = rowElement->getStringAttribute (getAttributeNameForColumnId (columnId));
                
                widest = jmax (widest, font.getStringWidth (text));
            }
        }
        
        return widest + 8;
    }
    void setOperatorNumber( int number)
    {
        if(number ==2)
            sysexData[4] = 0x20;
        if(number ==3)
            sysexData[4] = 0x40;
        if(number ==4)
            sysexData[4] = 0x60;
        
    }
    int getSelection (const int rowNumber) const
    {
        return dataList->getChildElement (rowNumber)->getIntAttribute ("Select");
    }
    
    void setSelection (const int rowNumber, const int newSelection)
    {
        dataList->getChildElement (rowNumber)->setAttribute ("Select", newSelection);
    }
    
    String getText (const int columnNumber, const int rowNumber) const
    {
        return dataList->getChildElement (rowNumber)->getStringAttribute (getAttributeNameForColumnId (columnNumber));
    }
    
    void setText (const int columnNumber, const int rowNumber, const String& newText)
    {
        const auto& columnName = table.getHeader().getColumnName (columnNumber);
        dataList->getChildElement (rowNumber)->setAttribute (columnName, newText);
    }
    
    //==============================================================================
    void resized() override
    {
        table.setBoundsInset (BorderSize<int> (8));
        
    }
    void selectedRowsChanged	(	int 	lastRowSelected	) override
    {
        rowSelected = lastRowSelected;
        
        sysexData[8] = lastRowSelected;
        
        sender.send(oscAddressPatern, (uint8) sysexData[0], sysexData[1], sysexData[2], sysexData[3], sysexData[4], sysexData[5], sysexData[6], sysexData[7], sysexData[8]);
        
    }
    
    int selectedRowChanged ()
    {
        return rowSelected;
    }
private:
    int rowSelected;
    TableListBox table  { {}, this };
    Font font           { 14.0f };
    String oscAddressPatern {"/SYSEX"};
    std::unique_ptr<XmlElement> tutorialData;
    XmlElement* columnList = nullptr;
    XmlElement* dataList = nullptr;
    int numRows = 0;
    OSCSender sender;
    int sysexData[9] = { 0x43, 0X10, 0x34, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00 };
    
    
    //==============================================================================
    class EditableTextCustomComponent  : public Label
    {
    public:
        EditableTextCustomComponent (TableTutorialComponent& td)
        : owner (td)
        {
            setEditable (false, true, false);
        }
        
        void mouseDown (const MouseEvent& event) override
        {
            owner.table.selectRowsBasedOnModifierKeys (row, event.mods, false);
            
            Label::mouseDown (event);
        }
        
        void textWasEdited() override
        {
            owner.setText (columnId, row, getText());
        }
        
        void setRowAndColumn (const int newRow, const int newColumn)
        {
            row = newRow;
            columnId = newColumn;
            setText (owner.getText(columnId, row), dontSendNotification);
        }
        
    private:
        TableTutorialComponent& owner;
        int row, columnId;
        Colour textColour;
    };
    
    //==============================================================================
    class SelectionColumnCustomComponent    : public Component
    {
    public:
        SelectionColumnCustomComponent (TableTutorialComponent& td)
        : owner (td)
        {
            addAndMakeVisible (toggleButton);
            
            toggleButton.onClick = [this] { owner.setSelection (row, (int) toggleButton.getToggleState()); };
        }
        
        void resized() override
        {
            toggleButton.setBoundsInset (BorderSize<int> (2));
        }
        
        void setRowAndColumn (int newRow, int newColumn)
        {
            row = newRow;
            columnId = newColumn;
            toggleButton.setToggleState ((bool) owner.getSelection (row), dontSendNotification);
        }
        
    private:
        TableTutorialComponent& owner;
        ToggleButton toggleButton;
        int row, columnId;
        
        
    };
    
    //==============================================================================
    class TutorialDataSorter
    {
    public:
        TutorialDataSorter (const String& attributeToSortBy, bool forwards)
        : attributeToSort (attributeToSortBy),
        direction (forwards ? 1 : -1)
        {}
        
        int compareElements (XmlElement* first, XmlElement* second) const
        {
            auto result = first->getStringAttribute (attributeToSort)
            .compareNatural (second->getStringAttribute (attributeToSort));
            
            if (result == 0)
                result = first->getStringAttribute ("ID")
                .compareNatural (second->getStringAttribute ("ID"));
            
            return direction * result;
        }
        
    private:
        String attributeToSort;
        int direction;
    };
    
    //==============================================================================
    void loadData()
    {
        //    auto dir = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory );
        
        //   int numTries = 0;
        
        //  while (! dir.getChildFile ("Resources").exists() && numTries++ < 15)
        //      dir = dir.getParentDirectory();
        
        //  auto tableFile = dir.getChildFile ("SY77Waves.xml");
        auto tableFile = BinaryData::SY77Waves_xml;
        if(SYModel==3)
            tableFile= BinaryData::SY99Waves_xml;
        
        
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
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TableTutorialComponent)
};
/*
 */
class AWMVue    : public ElementComponent, public TextButton::Listener
{
public:
    AWMVue()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
  
    
        addAndMakeVisible(labelFine);
        addAndMakeVisible(labelFixed);
        labelFine.setJustificationType(Justification::centred);
        labelFixed.setJustificationType(Justification::centred);
        labelFine.attachToComponent(&sliderFine, false);
        labelFixed.attachToComponent(&btFixed, false);
        addAndMakeVisible(wavesTable);
        wavesTable.setSelection(2, 1);
        
        
    }
    
    ~AWMVue()
    {

    }
    void setElementNumber ( int number, UndoManager& um) override
    {

        intElement = number;
        // Need to initialise here if we wanna have the element number
        int sysexdata[9] = { 0x43, 0X10, 0x34, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00 };
        if(intElement == 2)
        {
            wavesTable.setOperatorNumber(2);
            sysexdata[4] = 0x20;
        }
        if(intElement == 3)
        {
            wavesTable.setOperatorNumber(3);
            sysexdata[4] = 0x40;
        }
        if(intElement == 4)
        {
            wavesTable.setOperatorNumber(4);
            sysexdata[4] = 0x60;
        }
        
        sliderWaveForm.setMidiSysex(sysexdata);
        sliderWaveForm.setRangeAndRound(0, 127,0);
        sliderWaveForm.setPopupDisplayEnabled(true, true, this);
        addAndMakeVisible(sliderWaveForm);
        
        sysexdata[6] = 0x04;
        sliderFine.setMidiSysex(sysexdata);
        sliderFine.setRangeAndRound(-64, 63,0);
        sliderFine.setBoolInvert(true);
        sliderFine.setPopupDisplayEnabled(true, true, this);
        addAndMakeVisible(sliderFine);
        
        sysexdata[6] = 0x02;
        btFixed.setMidiSysex(sysexdata);
        btFixed.setTextOnOff("Fixed", "Normal");
        addAndMakeVisible(btFixed);
    }
    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
         draws some placeholder text to get you started.
         
         You should replace everything in this method with your own
         drawing code..
         */
        
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background
        
    }
    void buttonClicked (Button* button) override
    {
        setVisible(false);
    }
    
    void resized() override
    {

        sliderFine.setBoundsRelative(0.1f, 0.3f, 0.2f, 0.3f);
        btFixed.setBoundsRelative(0.1f, 0.7f, 0.2f, 0.1f);
        wavesTable.setBoundsRelative(0.48f, 0.0f, 0.52f, 1.0f);
        //  int wgrid = getWidth()/5;
        sliderWaveForm.setBounds(0, 30, 20, getHeight()-40);
        
        // This method is where you should set the bounds of any child
        // components that your component contains..
  
    }
    
private:
    
    int intElement;
    Label labelFine {"","Fine tune"};
    Label labelFixed {"","Fixed tune"};
    MidiSlider sliderWaveForm;
    MidiSlider  sliderFine;
    MidiButton  btFixed;

    TableTutorialComponent  wavesTable;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AWMVue)
};
