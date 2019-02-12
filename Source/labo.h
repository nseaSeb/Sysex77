//
//  labo.h
//  MidiDemo
//
//  Created by Sébastien Portrait on 11/11/2018.
//  Copyright © 2018 JUCE. All rights reserved.
//

#ifndef labo_h
#define labo_h
/*
 ==============================================================================
 
 This file is part of the JUCE tutorials.
 Copyright (c) 2017 - ROLI Ltd.
 
 The code included in this file is provided under the terms of the ISC license
 http://www.isc.org/downloads/software-support-policy/isc-license. Permission
 To use, copy, modify, and/or distribute this software for any purpose with or
 without fee is hereby granted provided that the above copyright notice and
 this permission notice appear in all copies.
 
 THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
 WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
 PURPOSE, ARE DISCLAIMED.
 
 ==============================================================================
 */

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.
 
 BEGIN_JUCE_PIP_METADATA
 
 name:             TableListBoxTutorial
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Explores table list boxes.
 
 dependencies:     juce_core, juce_data_structures, juce_events, juce_graphics,
 juce_gui_basics
 exporters:        xcode_mac, vs2017, linux_make
 
 type:             Component
 mainClass:        MainComponent
 
 useLocalCopy:     1
 
 END_JUCE_PIP_METADATA
 
 *******************************************************************************/


#pragma once

//==============================================================================
class TableTutorialComponent    : public Component,
public TableListBoxModel
{
public:
    TableTutorialComponent()
    {
        loadData();
        
        addAndMakeVisible (table);
        
        table.setColour (ListBox::outlineColourId, Colours::grey);
        table.setOutlineThickness (1);
        
        if (columnList != nullptr)
        {
            forEachXmlChildElement (*columnList, columnXml)
            {
                table.getHeader().addColumn (columnXml->getStringAttribute ("name"),
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
    
private:
    TableListBox table  { {}, this };
    Font font           { 14.0f };
    
    std::unique_ptr<XmlElement> tutorialData;
    XmlElement* columnList = nullptr;
    XmlElement* dataList = nullptr;
    int numRows = 0;
    
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
        auto dir = File::getCurrentWorkingDirectory();
        
        int numTries = 0;
        
        while (! dir.getChildFile ("Resources").exists() && numTries++ < 15)
            dir = dir.getParentDirectory();
        
        auto tableFile = dir.getChildFile ("Resources").getChildFile ("TableData.xml");
        
        if (tableFile.exists())
        {
            tutorialData.reset (XmlDocument::parse (tableFile));
            
            dataList   = tutorialData->getChildByName ("DATA");
            columnList = tutorialData->getChildByName ("HEADERS");
            
            numRows = dataList->getNumChildElements();
        }
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

//==============================================================================
/*
class MainComponent   : public Component
{
public:
    //==============================================================================
    MainComponent()
    {
        addAndMakeVisible (table);
        
        setSize (800, 600);
    }
    
    ~MainComponent() {}
    
    void paint (Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
    }
    
    void resized() override
    {
        table.setBounds (getLocalBounds());
    }
    
private:
    //==============================================================================
    TableTutorialComponent table;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
*/

#endif /* labo_h */
