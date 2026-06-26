/*
  ==============================================================================

    VoicesTableModel.h
    Created: 17 Nov 2018 5:25:44pm
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "LibraryIndex.h"

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
        : baseOffset (baseOffset), sourceModel (baseOffset, *this), label (std::move (bankLabel))
    {
        setName (label);
        sourceListBox.setModel (&sourceModel);
        sourceListBox.setHeaderComponent (std::make_unique<Header> (*this));
        addAndMakeVisible (sourceListBox);
    }

    /** Appelé quand une voix est sélectionnée dans CETTE colonne (pour que
        LibrairiePage désélectionne les autres → une seule voix active parmi les 64). */
    std::function<void (BankVoicesTable*)> onRowSelected;

    /** Retire la sélection de cette colonne (sans déclencher onRowSelected). */
    void deselectAllRows() { sourceListBox.deselectAllRows(); }

    /** Index global (0..63) de la voix sélectionnée dans cette colonne, ou < 0 si aucune. */
    int getSelectedGlobalIndex() const
    {
        const int row = sourceListBox.getSelectedRow();
        return row < 0 ? -1 : baseOffset + row;
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
        SourceItemListboxContents (int off, BankVoicesTable& o) : offset (off), owner (o) {}

        int getNumRows() override { return 16; }

        // Sélection unique parmi les 64 voix : prévient le parent pour qu'il
        // désélectionne les 3 autres colonnes. (lastRowSelected = -1 lors d'une
        // désélection -> on n'émet rien, ce qui évite toute récursion.)
        void selectedRowsChanged (int lastRowSelected) override
        {
            if (lastRowSelected >= 0 && owner.onRowSelected != nullptr)
                owner.onRowSelected (&owner);
        }

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
            const int glob = rowNumber + offset;
            if (arrayListVoices[glob].isNotEmpty())
            {
                g.drawFittedText (arrayListVoices[glob], 0, 0, width, height,
                                  Justification::centred, 1);

                // Indicateur métadonnées : ★ favori, • si tags (clé = banque courante#slot).
                if (currentBankRelPath.isNotEmpty())
                {
                    const auto meta = LibraryIndex::get().getMeta (
                        LibraryIndex::keyFor (currentBankRelPath, glob));
                    if (meta.fav || ! meta.tags.isEmpty())
                    {
                        g.setColour (meta.fav ? Colours::gold
                                              : SYColLabel.withAlpha (0.55f));
                        g.setFont (height * 0.6f);
                        g.drawText (meta.fav ? String::fromUTF8 ("\xE2\x98\x85")   // ★
                                             : String::fromUTF8 ("\xE2\x80\xA2"),  // •
                                    3, 0, 14, height, Justification::centredLeft);
                    }
                }
            }
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
        BankVoicesTable& owner;
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
    int baseOffset;
    ListBox sourceListBox  { "D+D source", nullptr };
    SourceItemListboxContents sourceModel;
    String label;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BankVoicesTable)
};
