// MidiGlobals.h - etat partage de l'application (statics de portee fichier, adresses du bus
// interne). Inclus UNE fois, par MidiDemo.h, au meme endroit qu'avant (build unity).
#pragma once

//==============================================================================
// État partagé de l'application.
//
// Architecture : build « unity » (tout transite par Main.cpp -> MidiDemo.h), donc
// cet état vit en variables de portée fichier. Les `static const String` ci-dessous
// sont des CONSTANTES (adresses du bus interne, cf. SysexBus.h) ; le bloc « état de
// session » plus bas regroupe l'état MUTABLE d'exécution (sélection courante, device,
// thème via LookAndFeel.h…). NB : Declarations.h (anciens globals d'UI) a été supprimé,
// c'était du code mort — les vrais objets d'UI sont des membres de MidiDemo.
//==============================================================================

// --- Adresses du bus interne (constantes) ---
static const String adresseOscFoot = "/77Foot";
static const String adresseOscMod = "/77Mod";
static const String adresseOpMode = "/77OpMode";
static const String adresseOscSendBank ="/77SendBank";
static const String adresseOscRequestDump ="/77RequestDump";
static const String adresseOscRequestMulti ="/77RequestMulti";
static const String adresseOscSendMulti ="/77SendMulti";   // bulk Multi -> edit buffer (clic + confirmation)   // dump request d'un Multi INT (lecture)
static const String adresseOscRepaint = "/77Repaint";
static const String adresseOscSendVoice = "/77SendVoice";
static const String oscTotalVoiceVolume = "/77TotalVoiceVolume";


static const String oscVoiceGrp1 = "/77VoiceGroupe1";
static const String oscVoiceGrp2 = "/77VoiceGroupe2";
static const String oscVoiceGrp3 = "/77VoiceGroupe3";
static const String oscVoiceGrp4 = "/77VoiceGroupe4";
static const String oscVoicePitch1 = "/77VoicePitch1";
static const String oscVoicePitch2 = "/77VoicePitch2";
static const String oscVoicePitch3 = "/77VoicePitch3";
static const String oscVoicePitch4 = "/77VoicePitch4";
static const String oscVoiceFine1 = "/77VoiceFine1";
static const String oscVoiceFine2 = "/77VoiceFine2";
static const String oscVoiceFine3 = "/77VoiceFine3";
static const String oscVoiceFine4 = "/77VoiceFine4";
static const String oscVoiceFixe1 = "/77VoiceFixe1";
static const String oscVoiceFixe2 = "/77VoiceFixe2";
static const String oscVoiceFixe3 = "/77VoiceFixe3";
static const String oscVoiceFixe4 = "/77VoiceFixe4";
static Array<MidiMessage>    oscMidiMessage; // tampon des 10 messages du nom de voix (cf. Voice.h)
static const String oscSendMidiMessage = "/77MidiMessage";

static   StringArray  arrayBank;    //la liste des banques
static  StringArray arrayListVoices; // liste des voices
static MemoryBlock currentBankData;  // octets bruts de la banque sélectionnée (pour envoi d'une voix)
static Array<SyVoice::SlotInfo> currentBankSlots; // badge/drapeau par slot (cf. computeSlotInfo)
static String currentBankRelPath;    // chemin relatif de la banque sélectionnée (clé LibraryIndex)
static const int maxFiles = 512;

static        Array<File> BankFiles; //les fichiers des banques
Array<MidiMessage> messages;
Array<MidiMessage> arraySysex;
int SYModel =1;

// --- État de session (mutable, runtime : sélection courante, flags, tampons) ---
static bool requestSysex;
static bool doubleClickBank = false;
static    bool bankDeleteKey = false;
static int rowSelectedBank;
static bool loadBankRequest = false;
static String bankSelected;

static Value valueSysexIn; //values from sysex midi in
static Value valueMultiOpen;   // [libelle, bloc 8101MU] : Multi ouvert depuis la librairie -> MultiPage
static Value valueMultiBulk; // bloc bulk 8101MU recu (MemoryBlock) -> MultiPage
static Value valueMultiIn; // param-change Multi (groupes 0x00/0x01) : PAS sur valueSysexIn (widgets de voix)

// Device number sysex SY77 (canal) : un seul réglage global, piloté par ConfigPage.
static int  sysexDeviceNumber = 1;     // 1..16 ; octet émis = 0x10 | (n-1)
static bool sysexReceiveOmni  = false; // "ALL" : accepte les messages entrants de tout device
// Channel of the notes played by the on-screen keyboard and the sequencer.
// 0 = follow the device number (channel 1 when the device is ALL), 1..16 = fixed.
static int  playChannelSetting = 0;
inline int currentPlayChannel()
{
    if (playChannelSetting >= 1 && playChannelSetting <= 16) return playChannelSetting;
    return sysexReceiveOmni ? 1 : juce::jlimit (1, 16, sysexDeviceNumber);
}
static bool followSynth       = false; // "Suivre le synthé" : ouvre la vue du paramètre reçu
static bool monitorRaw        = false; // moniteur : false = décodé (G/AH/AL/P/val), true = octets bruts hex
static bool devMode           = false; // Mode développeur : affiche le moniteur MIDI + outils RE (onglet "Midi Setting")
// Interrupteurs rapides écoute/envoi (bandeau bas) — diagnostic hardware.
// midiListen OFF : l'app IGNORE tout MIDI entrant (gate handleIncomingMidiMessage).
// midiSend   OFF : l'app n'émet RIEN vers les sorties (gate sendToOutputs, coupe aussi le Thru).
static bool midiListen        = true;  // ON par défaut
static bool midiSend          = true;  // ON par défaut
static juce::String lastMonitorKey;    // moniteur compact : dédup des param-changes par adresse
File appDirPath = File::getSpecialLocation(File::userApplicationDataDirectory ).getChildFile("Application Support/Sysex77");

static Path pathFilter1;
