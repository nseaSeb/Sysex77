/*
  ==============================================================================

    VoicesTableModel.h
    Created: 17 Nov 2018 5:25:44pm
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

// Demande l'envoi au synthé de la voix d'index global donné (via le bus interne -> MidiSysex).
inline void requestSendVoice (int globalVoiceIndex)
{
    static SysexBusSender voiceSender;
    voiceSender.send ("/77SendVoice", globalVoiceIndex);
}

//==============================================================================
/** Une colonne de 16 voix d'une banque (A/B/C/D), paramétrée par son décalage
    global dans `arrayListVoices` (0/16/32/48) et son libellé d'en-tête.

    Remplace les 4 classes BankA/B/C/DTableModel auparavant copiées-collées
    (elles ne différaient que par ce décalage et ce libellé). */
class BankVoicesTable    : public Component
{
public:
    BankVoicesTable (int baseOffset, String bankLabel)
        : sourceModel (baseOffset), label (std::move (bankLabel))
    {
        setName (label);
        sourceListBox.setModel (&sourceModel);
        sourceListBox.setHeaderComponent (std::make_unique<Header> (*this));
        addAndMakeVisible (sourceListBox);
    }

    void resized() override
    {
        sourceListBox.setBoundsInset (BorderSize<int> (0));
        int rowHeight = (sourceListBox.getHeight() - 24) / 16;
        if (rowHeight < 24)
            rowHeight = 24;
        sourceListBox.setRowHeight (rowHeight);
    }

    void loadBank() { repaint(); }

private:
    //==============================================================================
    struct SourceItemListboxContents  : public ListBoxModel
    {
        explicit SourceItemListboxContents (int off) : offset (off) {}

        int getNumRows() override { return 16; }

        void paintListBoxItem (int rowNumber, Graphics& g,
                               int width, int height, bool rowIsSelected) override
        {
            if (rowIsSelected)
                g.fillAll (SYColSelected);
            else if (rowNumber % 2)
                g.fillAll (SYColAlt);

            g.setColour (rowIsSelected ? SYColSelected.contrasting() : SYColLabel);
            g.setFont (height * 0.7f);

            // arrayListVoices est une StringArray : un index hors plage renvoie {} (sûr).
            if (arrayListVoices[rowNumber + offset].isNotEmpty())
                g.drawFittedText (arrayListVoices[rowNumber + offset], 0, 0, width, height,
                                  Justification::centred, 1);
        }

        void listBoxItemDoubleClicked (int row, const MouseEvent&) override
        {
            requestSendVoice (row + offset);
        }

        var getDragSourceDescription (const SparseSet<int>& selectedRows) override
        {
            StringArray rows;
            for (int i = 0; i < selectedRows.size(); ++i)
                rows.add (String (selectedRows[i] + 1));
            return rows.joinIntoString (", ");
        }

        int offset;
    };

    //==============================================================================
    struct Header    : public Component
    {
        Header (BankVoicesTable& o) : owner (o) { setSize (0, 30); }

        void paint (Graphics& g) override
        {
            g.setColour (SYColLabel);
            g.fillAll();
            g.setColour (SYColLabel.contrasting());
            g.drawFittedText (owner.label, getLocalBounds().reduced (20, 0), Justification::centred, 1);
        }

        BankVoicesTable& owner;
    };

    //==============================================================================
    ListBox sourceListBox  { "D+D source", nullptr };
    SourceItemListboxContents sourceModel;
    String label;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BankVoicesTable)
};
