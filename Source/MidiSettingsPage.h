// MidiSettingsPage.h - onglet "Midi Setting" (mode developpeur) : moniteur, outils RE, banc
// de test. Inclus par MidiDemo.h a l'endroit de l'ancienne definition (build unity).
#pragma once

//==============================================================================
/** Vue "Midi Setting" (MODE DÉVELOPPEUR uniquement) : moniteur MIDI + outils de
    rétro-ingénierie (fingerprint, lecture valeurs, diff de dumps), Bluetooth et le
    "MIDI Thru" (gardé ici car il écrit vers le synthé / risque d'écho). Le choix des
    interfaces In/Out, "Suivre le synthé", le canal et "Bulk Protect" ont été rapatriés
    dans l'onglet "Setting" (ConfigPage). Cet onglet n'est ajouté à la barre que si
    devMode == true (toggle dans Settings). Les composants restent la propriété de
    MidiDemo ; cette page ne fait que les re-parenter et les disposer. */
struct MidiSettingsPage : public Component
{
    MidiSettingsPage (TextButton& pair, Label& monLabel, TextEditor& monitor)
        : pairBtn (pair), monLab (monLabel), monitorRef (monitor)
    {
        setOpaque (false);
        addAndMakeVisible (pairBtn);
        addAndMakeVisible (bench);   // banc de test hardware (cable par MidiDemo)
        addAndMakeVisible (monLab);
        addAndMakeVisible (monitorRef);

        // Outil RE : comparer deux dumps .syx (avant/après un changement d'UN paramètre)
        // pour localiser l'octet -> bâtir la carte d'offsets bulk.
        addAndMakeVisible (diffBtn);
        diffBtn.onClick = [this]
        {
            auto* v = new DumpDiffView();
            v->setSize (740, 540);
            juce::DialogWindow::LaunchOptions o;
            o.content.setOwned (v);
            o.dialogTitle = "Diff 2 dumps (.syx)";
            o.dialogBackgroundColour = SYColBackground;
            o.resizable = true;
            o.useNativeTitleBar = true;
            o.launchAsync();
        };

        // Outil RE (jetable) : « empreinte » — envoie à chaque contrôle câblé de l'éditeur
        // une valeur brute distinctive + journalise (addr -> valeur) dans re_fingerprint.csv.
        // Workflow : dump AVANT -> clic -> dump APRÈS ; le diff isole nos octets, le CSV les nomme.
        addAndMakeVisible (reBtn);
        reBtn.onClick = [this] { if (onFingerprint) onFingerprint(); };

        // RE : exporte la valeur ACTUELLE de tous les contrôles câblés (sans rien envoyer).
        // Corréler avec un dump de la même voix = carte d'offsets, sans toucher au synthé.
        addAndMakeVisible (reReadBtn);
        reReadBtn.onClick = [this] { if (onReadValues) onReadValues(); };

        // Moniteur : effacer + copier (pratique pour la rétro-ingénierie).
        addAndMakeVisible (clearBtn);
        clearBtn.onClick = [this] { monitorRef.clear(); lastMonitorKey = {}; if (onClearHistory) onClearHistory(); };
        addAndMakeVisible (copyBtn);
        copyBtn.onClick = [this] { juce::SystemClipboard::copyTextToClipboard (monitorRef.getText()); };

        // Toggle "Raw / Interprété" : bascule l'affichage du moniteur entre octets bruts (hex)
        // et forme décodée (G/AH/AL/P/val). Persisté. Re-rend l'historique -> s'applique aussi
        // aux lignes déjà listées (cf. MidiDemo::rebuildMonitorFromHistory).
        addAndMakeVisible (rawBtn);
        monitorRaw = getAppSettings()->getBoolValue ("MonitorRaw", false);
        rawBtn.setToggleState (monitorRaw, dontSendNotification);
        rawBtn.onClick = [this]
        {
            monitorRaw = rawBtn.getToggleState();
            getAppSettings()->setValue ("MonitorRaw", monitorRaw);
            getAppSettings()->saveIfNeeded();
            if (onRawToggled) onRawToggled();   // re-rend le moniteur dans le nouveau mode
        };
    }

    // Câblés par MidiDemo (qui possède le moniteur et l'historique des messages).
    std::function<void()> onRawToggled;    // re-rend le moniteur (changement Raw/Interprété)
    std::function<void()> onClearHistory;  // vide l'historique borné (bouton Clear)

    void resized() override
    {
        const int m = 10;
        auto r = getLocalBounds().reduced (m);
        const int half = (r.getWidth() - m) / 2;

        // Rangée Bluetooth (pleine largeur)
        pairBtn.setBounds (r.removeFromTop (24));
        r.removeFromTop (8);

        // Outils RE : Diff | Fingerprint, puis Lire valeurs (pleine largeur)
        auto row2 = r.removeFromTop (24);
        diffBtn.setBounds (row2.removeFromLeft (half));
        reBtn  .setBounds (row2.removeFromRight (half));
        r.removeFromTop (8);
        reReadBtn.setBounds (r.removeFromTop (24));
        r.removeFromTop (10);
        bench.setBounds (r.removeFromTop (juce::jmax (200, r.getHeight() / 2)));
        r.removeFromTop (10);

        // En-tête du moniteur : libellé + toggle Raw à gauche, Copier/Clear à droite.
        auto head = r.removeFromTop (24);
        copyBtn .setBounds (head.removeFromRight (80));
        head.removeFromRight (6);
        clearBtn.setBounds (head.removeFromRight (80));
        head.removeFromRight (10);
        rawBtn.setBounds (head.removeFromLeft (220));
        monLab.setBounds (head);
        r.removeFromTop (4);

        monitorRef.setBounds (r);
    }

    // Déclenché par le bouton RE ; câblé par MidiDemo, qui possède les onglets et peut
    // parcourir TOUS les contenus (même non-courants — un TabbedComponent ne garde dans
    // l'arbre que l'onglet actif). Cf. la lambda onFingerprint dans MidiDemo.
    std::function<void()> onFingerprint;
    std::function<void()> onReadValues;

    TextButton& pairBtn;
    Label&      monLab;
    TextEditor& monitorRef;
    ToggleButton rawBtn    { "Moniteur : octets bruts (hex) au lieu de la forme decodee" };
    TextButton   diffBtn   { "Diff 2 dumps (.syx)..." };
    TextButton   reBtn     { "RE fingerprint -> CSV" };
    TextButton   reReadBtn { "RE lire valeurs -> CSV" };
    TextButton   copyBtn   { "Copier" };
    TextButton   clearBtn  { "Clear" };
    TestBenchPanel bench;
};
