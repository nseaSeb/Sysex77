/*
  ==============================================================================

    VoicesTableModel.h
    Created: 17 Nov 2018 5:25:44pm
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================

class BankATableModel    : public Component

{

public:
    BankATableModel()
    {
        setName ("Bank A");
      //  loadBank();
        sourceListBox.setModel (&sourceModel);
        sourceListBox.setHeaderComponent(new Header(*this));
        addAndMakeVisible (sourceListBox);
    }

    void resized() override
    {
        sourceListBox.setBoundsInset (BorderSize<int> (0));
        int rowHeight =(sourceListBox.getHeight() - 24) /16;
        if (rowHeight < 24)
            rowHeight =24;
        sourceListBox.setRowHeight(rowHeight);
    };

private:
    //==============================================================================
    struct SourceItemListboxContents  : public ListBoxModel
    {
        int getNumRows() override
        {
            
            return 16;
        }
        void listBoxItemDoubleClicked	(int 	row,const MouseEvent &)	override
        {
               Logger::writeToLog("table A double clic");
        }
        void listBoxItemClicked	(int 	row, const MouseEvent &)	 override
        {
            Logger::writeToLog("table A mouse event");
        }
        
        void paintListBoxItem (int rowNumber, Graphics& g,
                               int width, int height, bool rowIsSelected) override
        {
            if (rowIsSelected)
                g.fillAll (SYColSelected);
                else if (rowNumber % 2)
                    g.fillAll (SYColAlt);
                    
                    
                    g.setColour (LookAndFeel::getDefaultLookAndFeel().findColour (Label::textColourId));
                    g.setFont (height * 0.7f);
                    if(arrayListVoices[rowNumber].isNotEmpty())
                    g.drawFittedText(arrayListVoices[rowNumber], 0, 0, width, height, Justification::centred, 1);
                        
                        
                    }
        
        
        var getDragSourceDescription (const SparseSet<int>& selectedRows) override
        {
            // for our drag description, we'll just make a comma-separated list of the selected row
            // numbers - this will be picked up by the drag target and displayed in its box.
            StringArray rows;
            
            for (int i = 0; i < selectedRows.size(); ++i)
                rows.add (String (selectedRows[i] + 1));
                
                return rows.joinIntoString (", ");
                }
    };
    
    struct Header    : public Component
    {
        Header (BankATableModel& o)
        : owner (o)
        {
            setSize (0, 30);
        }
        
        void paint (Graphics& g) override
        {
            g.setColour(SYColLabel);
            g.fillAll();
            g.setColour (findColour (Label::textColourId));
            g.drawFittedText (TRANS("Bank A"), getLocalBounds().reduced (20, 0), Justification::centred, 1);
        }
        
        void mouseDown (const MouseEvent&) override
        {
            Logger::writeToLog("VoicesBankA Mouse event");
        }
        
        BankATableModel& owner;
    };
    
    
    //==============================================================================
    
public: void loadBank()
    {
        repaint();
/*
        if(!appDirPath.exists())
        {
            appDirPath.createDirectory();
            
            Logger::writeToLog("Creation du dossier de presets");
            Logger::writeToLog(appDirPath.getFullPathName());
            
        }
        
        //  appDirPath.findChildFiles(BankFiles, TypeOfFileToFind::findFiles, true, "someName");
        
        appDirPath.findChildFiles(BankFiles, File::TypesOfFileToFind::findFiles
                                  , true,"*.syx");
        numRows=BankFiles.size();
        
        //Verifier si il s'agit bien d'une BANK YAMAHA SY
        
        
        // Construction du XML
        XmlElement bankList ("TABLE_DATA");
        
        //Construction du Headers
        // create an inner element..
        XmlElement* chien = new XmlElement ("HEADERS");
        
        // create an inner element..
        XmlElement* poule = new XmlElement ("COLUMN");
        
        poule->setAttribute ("columnId", 1);
        poule->setAttribute ("name", "ID");
        poule->setAttribute ("width", 120);
        
        chien->addChildElement (poule);
        // create an inner element..
        XmlElement* mpoule = new XmlElement ("COLUMN");
        
        mpoule->setAttribute ("columnId", 2);
        mpoule->setAttribute ("name", "BANK");
        mpoule->setAttribute ("width", 120);
        
        chien->addChildElement (mpoule);
        bankList.addChildElement (chien);
        
        
        //Construction des DATA
        XmlElement* xData = new XmlElement("DATA");
        
        for (int i = 0; i < numRows  ; ++i)
        {
            XmlElement* giraffe = new XmlElement ("ITEM");
            giraffe->setAttribute ("ID", i);
            giraffe->setAttribute ("BANK", BankFiles[i].getFileName());
            arrayBank.add( BankFiles[i].getFileName());
            xData->addChildElement (giraffe);
        }
        bankList.addChildElement(xData);
        // dataList = &BankList;
        
        bankList.writeToFile(appDirPath.getFullPathName() + "/Bank.xml", "");
        
  */
    }
    
    
    
    
    ListBox sourceListBox  { "D+D source", nullptr };
    SourceItemListboxContents sourceModel;
    int numRows;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BankATableModel)
};

//==============================================================================

//==============================================================================

class BankBTableModel    : public Component

{
    
public:
    BankBTableModel()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        setName ("Bank B");
     //   loadBank();
        sourceListBox.setModel (&sourceModel);
        //sourceListBox.setMultipleSelectionEnabled (true);
        sourceListBox.setHeaderComponent(new Header(*this));
        
        addAndMakeVisible (sourceListBox);
        //     addAndMakeVisible (target);
        
    }
    
    ~BankBTableModel()
    {
    }
    
    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
         draws some placeholder text to get you started.
         
         You should replace everything in this method with your own
         drawing code..
         */
        
        //        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background
        
        //        g.setColour (Colours::grey);
        //        g.drawRect (getLocalBounds(), 1);   // draw an outline around the component
        
        //        g.setColour (Colours::white);
        //        g.setFont (14.0f);
        //        g.drawText ("BankTableModel", getLocalBounds(),
        //                    Justification::centred, true);   // draw some placeholder text
    }
    
    void resized() override
    {
        // This method is where you should set the bounds of any child
        // components that your component contains..
        //  auto r = getLocalBounds().reduced (8);
        sourceListBox.setBoundsInset (BorderSize<int> (0));
        int rowHeight =(sourceListBox.getHeight() - 24) /16;
        if (rowHeight < 24)
            rowHeight =24;
        sourceListBox.setRowHeight(rowHeight);
    };
private:
    //==============================================================================
    struct SourceItemListboxContents  : public ListBoxModel
    {
        int getNumRows() override
        {
            
            return 16;
        }
        
        void paintListBoxItem (int rowNumber, Graphics& g,
                               int width, int height, bool rowIsSelected) override
        {
            if (rowIsSelected)
                g.fillAll (SYColSelected);
                else if (rowNumber % 2)
                    g.fillAll (SYColAlt);
                    
                    
                    g.setColour (LookAndFeel::getDefaultLookAndFeel().findColour (Label::textColourId));
                    g.setFont (height * 0.7f);
                    
                    //    auto text = arrayBank[rowNumber] ;
                    if(arrayListVoices[rowNumber+16].isNotEmpty())
                        g.drawFittedText(arrayListVoices[rowNumber+16], 0, 0, width, height, Justification::centred, 1);
                 //   g.drawFittedText(arrayBank[rowNumber], 0, 0, width, height, Justification::centred, 1);
                /*
                 g.drawText ("Aucunes Banques" + String (rowNumber + 1),
                 5, 0, width, height,
                 Justification::centred, true);
                 */
                    }
        
        
        var getDragSourceDescription (const SparseSet<int>& selectedRows) override
        {
            // for our drag description, we'll just make a comma-separated list of the selected row
            // numbers - this will be picked up by the drag target and displayed in its box.
            StringArray rows;
            
            for (int i = 0; i < selectedRows.size(); ++i)
                rows.add (String (selectedRows[i] + 1));
                
                return rows.joinIntoString (", ");
                }
    };
    
    struct Header    : public Component
    {
        Header (BankBTableModel& o)
        : owner (o)
        {
            setSize (0, 30);
        }
        
        void paint (Graphics& g) override
        {
            g.setColour(SYColLabel);
            g.fillAll();
            g.setColour (findColour (Label::textColourId));
            g.drawFittedText (TRANS("Bank B"), getLocalBounds().reduced (20, 0), Justification::centred, 1);
        }
        
        void mouseDown (const MouseEvent&) override
        {
            
        }
        
        BankBTableModel& owner;
    };
    
    
    //==============================================================================
    
public:    void loadBank()
    {
        repaint();
        
    }
    
    
    
    
    ListBox sourceListBox  { "D+D source", nullptr };
    SourceItemListboxContents sourceModel;
    int numRows;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BankBTableModel)
};

//==============================================================================


//==============================================================================

class BankCTableModel    : public Component

{
    
public:
    BankCTableModel()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        setName ("Bank C");
    //    loadBank();
        sourceListBox.setModel (&sourceModel);
        //sourceListBox.setMultipleSelectionEnabled (true);
        sourceListBox.setHeaderComponent(new Header(*this));
        
        addAndMakeVisible (sourceListBox);
        //     addAndMakeVisible (target);
        
    }
    
    ~BankCTableModel()
    {
    }
    
    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
         draws some placeholder text to get you started.
         
         You should replace everything in this method with your own
         drawing code..
         */
        
        //        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background
        
        //        g.setColour (Colours::grey);
        //        g.drawRect (getLocalBounds(), 1);   // draw an outline around the component
        
        //        g.setColour (Colours::white);
        //        g.setFont (14.0f);
        //        g.drawText ("BankTableModel", getLocalBounds(),
        //                    Justification::centred, true);   // draw some placeholder text
    }
    
    void resized() override
    {
        // This method is where you should set the bounds of any child
        // components that your component contains..
        //  auto r = getLocalBounds().reduced (8);
        sourceListBox.setBoundsInset (BorderSize<int> (0));
        int rowHeight =(sourceListBox.getHeight() - 24) /16;
        if (rowHeight < 24)
            rowHeight =24;
        sourceListBox.setRowHeight(rowHeight);
    };
private:
    //==============================================================================
    struct SourceItemListboxContents  : public ListBoxModel
    {
        int getNumRows() override
        {
            
            return 16;
        }
        
        void paintListBoxItem (int rowNumber, Graphics& g,
                               int width, int height, bool rowIsSelected) override
        {
            if (rowIsSelected)
                g.fillAll (SYColSelected);
                else if (rowNumber % 2)
                    g.fillAll (SYColAlt);
                    
                    
                    g.setColour (LookAndFeel::getDefaultLookAndFeel().findColour (Label::textColourId));
                    g.setFont (height * 0.7f);
                    
                    //    auto text = arrayBank[rowNumber] ;
                    
                    if(arrayListVoices[rowNumber+32].isNotEmpty())
                        g.drawFittedText(arrayListVoices[rowNumber+32], 0, 0, width, height, Justification::centred, 1);
                        
                /*
                 g.drawText ("Aucunes Banques" + String (rowNumber + 1),
                 5, 0, width, height,
                 Justification::centred, true);
                 */
                    }
        
        
        var getDragSourceDescription (const SparseSet<int>& selectedRows) override
        {
            // for our drag description, we'll just make a comma-separated list of the selected row
            // numbers - this will be picked up by the drag target and displayed in its box.
            StringArray rows;
            
            for (int i = 0; i < selectedRows.size(); ++i)
                rows.add (String (selectedRows[i] + 1));
                
                return rows.joinIntoString (", ");
                }
    };
    
    struct Header    : public Component
    {
        Header (BankCTableModel& o)
        : owner (o)
        {
            setSize (0, 30);
        }
        
        void paint (Graphics& g) override
        {
            g.setColour(SYColLabel);
            g.fillAll();
            g.setColour (findColour (Label::textColourId));
            g.drawFittedText (TRANS("Bank C"), getLocalBounds().reduced (20, 0), Justification::centred, 1);
        }
        
        void mouseDown (const MouseEvent&) override
        {
            
        }
        
        BankCTableModel& owner;
    };
    
    
    //==============================================================================
    
public:    void loadBank()
    {
        repaint();
        
        
    }
    
    
    
    
    ListBox sourceListBox  { "D+D source", nullptr };
    SourceItemListboxContents sourceModel;
    int numRows;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BankCTableModel)
};

//==============================================================================

//==============================================================================

class BankDTableModel    : public Component

{
    
public:
    BankDTableModel()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        setName ("Bank D");
//        loadBank();
        sourceListBox.setModel (&sourceModel);
        //sourceListBox.setMultipleSelectionEnabled (true);
        sourceListBox.setHeaderComponent(new Header(*this));
        
        addAndMakeVisible (sourceListBox);
        //     addAndMakeVisible (target);
        
    }
    
    ~BankDTableModel()
    {
    }
    
    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
         draws some placeholder text to get you started.
         
         You should replace everything in this method with your own
         drawing code..
         */
        
        //        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background
        
        //        g.setColour (Colours::grey);
        //        g.drawRect (getLocalBounds(), 1);   // draw an outline around the component
        
        //        g.setColour (Colours::white);
        //        g.setFont (14.0f);
        //        g.drawText ("BankTableModel", getLocalBounds(),
        //                    Justification::centred, true);   // draw some placeholder text
    }
    
    void resized() override
    {
        // This method is where you should set the bounds of any child
        // components that your component contains..
        //  auto r = getLocalBounds().reduced (8);
        sourceListBox.setBoundsInset (BorderSize<int> (0));
        int rowHeight =(sourceListBox.getHeight() - 24) /16;
        if (rowHeight < 24)
            rowHeight =24;
        sourceListBox.setRowHeight(rowHeight);
    };
private:
    //==============================================================================
    struct SourceItemListboxContents  : public ListBoxModel
    {
        int getNumRows() override
        {
            
            return 16;
        }
        void paintListBoxItem (int rowNumber, Graphics& g,
                               int width, int height, bool rowIsSelected) override
        {
            if (rowIsSelected)
                g.fillAll (SYColSelected);
                else if (rowNumber % 2)
                    g.fillAll (SYColAlt);
                    
                    
                    g.setColour (LookAndFeel::getDefaultLookAndFeel().findColour (Label::textColourId));
                    g.setFont (height * 0.7f);
                    
                    //    auto text = arrayBank[rowNumber] ;
                    
                    if(arrayListVoices[rowNumber+48].isNotEmpty())
                        g.drawFittedText(arrayListVoices[rowNumber+48], 0, 0, width, height, Justification::centred, 1);
                        
                /*
                 g.drawText ("Aucunes Banques" + String (rowNumber + 1),
                 5, 0, width, height,
                 Justification::centred, true);
                 */
                    }
        
        
        var getDragSourceDescription (const SparseSet<int>& selectedRows) override
        {
            // for our drag description, we'll just make a comma-separated list of the selected row
            // numbers - this will be picked up by the drag target and displayed in its box.
            StringArray rows;
            
            for (int i = 0; i < selectedRows.size(); ++i)
                rows.add (String (selectedRows[i] + 1));
                
                return rows.joinIntoString (", ");
                }
    };
    
    struct Header    : public Component
    {
        Header (BankDTableModel& o)
        : owner (o)
        {
            setSize (0, 30);
        }
        
        void paint (Graphics& g) override
        {
            g.setColour(SYColLabel);
            g.fillAll();
            g.setColour (findColour (Label::textColourId));
            
            g.drawFittedText (TRANS("Bank D"), getLocalBounds().reduced (20, 0), Justification::centred, 1);
        }
        
        void mouseDown (const MouseEvent&) override
        {
            
        }
        
        BankDTableModel& owner;
    };
    
    
    //==============================================================================
    
public:    void loadBank()
    {
        repaint();
        
    }
    
    
    
    
    ListBox sourceListBox  { "D+D source", nullptr };
    SourceItemListboxContents sourceModel;
    int numRows;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BankDTableModel)
};

//==============================================================================
    //==============================================================================
