/*
  ==============================================================================

    BankTableModel.h
    Created: 17 Nov 2018 6:02:48pm
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SysexUtils.h"
#include "LibraryIndex.h"

//==============================================================================
/*
*/

class BankTableModel    : public Component, public ChangeListener,public ChangeBroadcaster, public FileDragAndDropTarget, public TextEditor::Listener
{
public:
    BankTableModel()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        setName ("BANK");
        loadBank();
   
        sourceListBox.setModel (&sourceModel);
       // sourceListBox.getVerticalScrollBar().setColour(ListBox::outlineColourId, SYCol);
        //sourceListBox.setMultipleSelectionEnabled (true);
        sourceListBox.setHeaderComponent(std::make_unique<Header>(*this));
        
        addAndMakeVisible (sourceListBox);
        addAndMakeVisible(groupDrop);
        addAndMakeVisible(editText);
        editText.setVisible(false);
        editText.setInputRestrictions(10);
        editText.addListener(this);
        editText.setSelectAllWhenFocused(true);
        
        groupDrop.setVisible(false);
        sourceModel.addChangeListener(this);
 
   //     addAndMakeVisible (target);

    }

    ~BankTableModel()
    {
        sourceModel.removeChangeListener(this);
        editText.removeListener(this);
    }
    void textEditorReturnKeyPressed	(	TextEditor & 		) override
    {
                 doubleClickBank = false;
        editText.setVisible(false);
        String str;
        str = editText.getText();
        if(!str.endsWithIgnoreCase (".syx"))
        str = str + ".SYX";
        arrayBank.set(rowSelectedBank,str);
 
        Logger::writeToLog( BankFiles[rowSelectedBank].getFullPathName());
        
    
        BankFiles[rowSelectedBank].moveFileTo (BankFiles[rowSelectedBank].getSiblingFile (str));
        
        loadBank();
        repaint();
    
    }
    
    void textEditorFocusLost	(	TextEditor & 		) override
    {
            editText.setVisible(false);
          doubleClickBank = false;
    }

    void mouseDown (const MouseEvent&) override
    {
        Logger::writeToLog("Bank Mouse event");
    }

    void changeListenerCallback (ChangeBroadcaster* source) override
    {
      if(loadBankRequest)
      {
          loadBankRequest = false; //make a better code later
          loadBank();
      }
        Logger::writeToLog("BankModel: changebrodcaster");
        if (requestSysex == true)
        {
            loadBank();
            repaint();
            requestSysex = false;
            newMessage = false;
        }
        else if(doubleClickBank)
        {
          
            Logger::writeToLog("Change double click");
            editText.setVisible(true);
            editText.setBounds(sourceListBox.getRowPosition(rowSelectedBank, false));
            editText.setText(arrayBank[rowSelectedBank]);
 //           editText.insertTextAtCaret(arrayBank[rowSelectedBank]);
            
        }
        else if(bankDeleteKey) //message delete (touche Suppr)
        {
            bankDeleteKey = false;
            deleteSelectedBank (rowSelectedBank);
        }
        else
        {
        arrayListVoices.clear();
        currentBankData.setSize (0); // pas de banque valide chargée par défaut
        currentBankRelPath = String();
        MemoryBlock mb;
        auto file = BankFiles[sourceListBox.getSelectedRow()];
        if (file.exists() && file.loadFileAsData (mb)
            && SyVoice::looksLikeYamahaSysex ((const uint8*) mb.getData(), mb.getSize()))
        {
            currentBankData = mb; // mémorise pour pouvoir envoyer une voix au synthé
            currentBankRelPath = LibraryIndex::get().relPathOf (file); // clé pour tags/métadonnées
            // Extraction des noms de voix (logique pure et testée, cf. SysexUtils.h / Tests.h)
            arrayListVoices.addArray (SyVoice::extractVoiceNames ((const uint8*) mb.getData(), mb.getSize()));
        }
        else
        {
            Logger::writeToLog ("Banque ignorée (pas un dump Sysex Yamaha valide)");
        }
        sendChangeMessage();
        }
    }

    void paint (Graphics& g) override
    {
        
       }
    
    void resized() override
    {
    
            sourceListBox.setBoundsInset (BorderSize<int> (0));
        int rowHeight =(sourceListBox.getHeight() - 24) /16;
        if (rowHeight < 24)
            rowHeight =24;
        sourceListBox.setRowHeight(rowHeight);
        groupDrop.setBoundsInset(BorderSize<int>(10));
        editText.setBounds(0, 10, getWidth(), 24);
        
    };
    
//private:
/*
    struct  ItemTextEditor : public TextEditor
    {

    };
*/
 //==============================================================================
struct SourceItemListboxContents  : public ListBoxModel, public ChangeBroadcaster
    {
        // The following methods implement the necessary virtual functions from ListBoxModel,
        // telling the listbox how many rows there are, painting them, etc.
  
        
        int getNumRows() override
        {
            return maxFiles;
        }
        void change()
        {
            
        }
        void deleteKeyPressed (int lastRowSelected	) override
        {
            Logger::writeToLog("Bank Delete key pressed");
            bankDeleteKey = true;
            rowSelectedBank = lastRowSelected;
            sendChangeMessage();
        }

        void listBoxItemDoubleClicked	(int 	row,const MouseEvent &)	override
        {
            doubleClickBank = true;
            rowSelectedBank = row;
            sendChangeMessage();
            Logger::writeToLog("table A double clic");
        }
        // La sélection (chargement de la banque) se fait dans selectedRowsChanged,
        // PAS dans le paint : l'ancien code rechargeait le fichier .syx à chaque
        // repaint d'une ligne sélectionnée (I/O fichier dans paint).
        void selectedRowsChanged (int lastRowSelected) override
        {
            if (lastRowSelected < 0)
                return;
            rowSelectedBank = lastRowSelected;
            if (! doubleClickBank)
            {
                bankSelected = arrayBank[lastRowSelected];
                sendChangeMessage(); // -> BankTableModel::changeListenerCallback charge la banque
            }
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
            if (arrayBank[rowNumber].isNotEmpty())
                g.drawFittedText (arrayBank[rowNumber], 0, 0, width, height, Justification::centred, 1);
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
        Header (BankTableModel& o)
        : owner (o)
        {
            setSize (0, 30);
        }
        
        void paint (Graphics& g) override
        {
            g.setColour(SYColLabel);
            g.fillAll();
            g.setColour (SYColLabel.contrasting());
            g.setFont(16.0f);
            g.drawFittedText (TRANS("Librairie"), getLocalBounds().reduced (20, 0), Justification::centred, 1);
        }
        
        void mouseDown (const MouseEvent&) override
        {
          Logger::writeToLog("BankTableModel -> clicked");
        }

        
        BankTableModel& owner;
    };
    
    //==============================================================================
    // These methods implement the FileDragAndDropTarget interface, and allow our component
    // to accept drag-and-drop of files..
    
    bool isInterestedInFileDrag (const StringArray& /*files*/) override
    {
        Logger::writeToLog("isInterested");
        // normally you'd check these files to see if they're something that you're
        // interested in before returning true, but for the demo, we'll say yes to anything..
        return true;
    }
    
    void fileDragEnter (const StringArray& /*files*/, int /*x*/, int /*y*/) override
    {
        Logger::writeToLog("fileDragEnter");
        setMouseCursor(MouseCursor::DraggingHandCursor);
        groupDrop.setVisible(true);
        
        repaint();
    }
    
    void fileDragMove (const StringArray& /*files*/, int /*x*/, int /*y*/) override
    {
        Logger::writeToLog("fileDragMove");
    }
    
    void fileDragExit (const StringArray& /*files*/) override
    {
        Logger::writeToLog("fileDragExit");
        groupDrop.setVisible(false);
        repaint();
    }
    
    void filesDropped (const StringArray& files, int /*x*/, int /*y*/) override
    {
        Logger::writeToLog("fileDropped");
        Logger::writeToLog( "Files dropped: " + files.joinIntoString ("\n"));
        groupDrop.setVisible(false);
        File file(files.joinIntoString("\n"));
        String str = appDirPath.getFullPathName();
        str = str + "/";
        file.copyFileTo(str);
            Logger::writeToLog("FilesDropped: copy files OK");
 //       void MyClass::importAudioFile(File audioFile)
        File audioFile;
        for(int i = 0; i < files.size(); i++)
        {
            audioFile = files[i];
            if (audioFile.exists())
            {
                //create a new file based on the imported file's name and the path of the working directory
                File audioFileCopy (appDirPath.getFullPathName() + "/" + audioFile.getFileName());
                
                if (audioFileCopy.existsAsFile() == false) //if it doesn't yet exist...
                {
                    //...copy the imported audio file into the newly created file
                    audioFile.copyFileTo(audioFileCopy);
                }
                else if (audioFileCopy.existsAsFile() == true && audioFile.hasIdenticalContentTo(audioFileCopy) == false)
                    //if the file already exists (in terms of file name) but they aren't the same file in terms of content...
                {
                    //... copy the imported file but with different name
                    audioFileCopy = audioFileCopy.getNonexistentSibling();
                    audioFile.copyFileTo(audioFileCopy);
                }
                
            }
                //else, the imported file already exists in the directory so no new files need to be added
        }
         loadBank();
        repaint();
    }

//==============================================================================
    
    void loadBank()
    {
        if(!appDirPath.exists())
        {
            appDirPath.createDirectory();
            Logger::writeToLog("Creation du dossier de presets");
            Logger::writeToLog(appDirPath.getFullPathName());
        }

        // Liste MAÎTRE complète (tous les .syx, sous-dossiers compris). arrayBank/BankFiles
        // (la liste AFFICHÉE) en est une VUE FILTRÉE construite par applyFilter() -> le mapping
        // ligne→fichier reste 1:1, la sélection/renommage/suppression restent corrects.
        bankFilesAll.clear();
        bankNamesAll.clear();
        appDirPath.findChildFiles(bankFilesAll, File::TypesOfFileToFind::findFiles, true, "*.syx");
        sortMaster();    // remplit bankNamesAll (trié selon sortMode)
        applyFilter();
    }

    // Tri de la liste maître : 0 = nom A→Z, 1 = synthé, 2 = date (récent d'abord).
    void sortMaster()
    {
        std::vector<File> v (bankFilesAll.begin(), bankFilesAll.end());
        const int mode = sortMode;
        std::sort (v.begin(), v.end(), [mode] (const File& a, const File& b)
        {
            if (mode == 2)
                return a.getLastModificationTime() > b.getLastModificationTime();
            if (mode == 1)
            {
                auto& idx = LibraryIndex::get();
                const auto sa = idx.synthOfBank (idx.relPathOf (a));
                const auto sb = idx.synthOfBank (idx.relPathOf (b));
                if (sa != sb) return sa < sb;
            }
            return a.getFileName().compareIgnoreCase (b.getFileName()) < 0;
        });
        bankFilesAll = Array<File> (v.data(), (int) v.size());
        bankNamesAll.clear();
        for (auto& f : v) bankNamesAll.add (f.getFileName());
    }

    void setSortMode (int m) { sortMode = m; sortMaster(); applyFilter(); }

    // Charge une banque par chemin relatif (vue résultats) : remplit currentBankData/RelPath,
    // peuple les noms de voix, et notifie (recharge colonnes / inspecteur).
    void loadBankByRelPath (const String& rel)
    {
        File f = appDirPath.getChildFile (rel);
        arrayListVoices.clear();
        currentBankData.setSize (0);
        currentBankRelPath = String();
        MemoryBlock mb;
        if (f.existsAsFile() && f.loadFileAsData (mb)
            && SyVoice::looksLikeYamahaSysex ((const uint8*) mb.getData(), mb.getSize()))
        {
            currentBankData = mb;
            currentBankRelPath = rel;
            arrayListVoices.addArray (SyVoice::extractVoiceNames ((const uint8*) mb.getData(), mb.getSize()));
        }
        sendChangeMessage();
    }

    // Construit la vue filtrée (arrayBank/BankFiles) à partir de la liste maître + filterText.
    void applyFilter()
    {
        arrayBank.clear();
        BankFiles.clear();
        for (int i = 0; i < bankFilesAll.size(); ++i)
        {
            if (filterText.isNotEmpty() && ! bankNamesAll[i].containsIgnoreCase (filterText))
                continue;
            if (synthFilter.isNotEmpty()
                && LibraryIndex::get().synthOfBank (LibraryIndex::get().relPathOf (bankFilesAll[i])) != synthFilter)
                continue;
            BankFiles.add (bankFilesAll[i]);
            arrayBank.add (bankNamesAll[i]);
        }
        numRows = BankFiles.size();
        sourceListBox.updateContent();
        repaint();
    }

    void setSynthFilter (const String& synth)
    {
        synthFilter = synth;
        sourceListBox.deselectAllRows();
        applyFilter();
    }

    File getSelectedBankFile() const
    {
        const int r = sourceListBox.getSelectedRow();
        return (r >= 0 && r < BankFiles.size()) ? BankFiles[r] : File();
    }

    // Filtre de recherche (appelé par le champ de recherche de LibrairiePage).
    void setFilter (const String& f)
    {
        filterText = f.trim();
        sourceListBox.deselectAllRows();   // pas de sélection résiduelle sur une ligne masquée
        applyFilter();
    }

    int getSelectedBankRow() const { return sourceListBox.getSelectedRow(); }

    // Supprime la banque de la ligne `row` (index dans la VUE filtrée). Confirmation,
    // suppression disque, désindexation ciblée (LibraryIndex), puis re-scan propre.
    void deleteSelectedBank (int row)
    {
        if (row < 0 || row >= BankFiles.size())
            return;

        auto file = BankFiles[row];
        if (! AlertWindow::showOkCancelBox (AlertWindow::WarningIcon,
                TRANS("Supprimer la banque"),
                TRANS("Supprimer definitivement cette banque ?") + String ("\n") + file.getFileName(),
                TRANS("Supprimer"), TRANS("Annuler")))
            return;

        const auto rel = LibraryIndex::get().relPathOf (file);
        file.deleteFile();
        LibraryIndex::get().removeBank (rel);   // désindexation ciblée + save

        loadBank();            // re-scan disque -> vue rafraîchie proprement
        sendChangeMessage();   // notifie LibrairiePage (recharge colonnes voix / info-line)
        repaint();
    }

        bool somethingIsBeingDraggedOver = false;
    Array<File> bankFilesAll;   // liste maître (non filtrée)
    StringArray bankNamesAll;
    String      filterText;
    String      synthFilter;    // "" = tous synthés
    int         sortMode = 0;   // 0=nom, 1=synthé, 2=date
    ListBox sourceListBox  { "D+D source", nullptr };
    GroupComponent  groupDrop;
    SourceItemListboxContents sourceModel;
    TextEditor editText;

    int numRows;
 
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BankTableModel)
    };
    
    //==============================================================================
