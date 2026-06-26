/*
 ==============================================================================

 Updater.h

 Mise à jour in-app depuis les GitHub Releases (nseaSeb/Sysex77) :
   - vérif au démarrage (non bloquante, fail-silent, throttlée 1×/24 h, opt-out),
   - téléchargement de l'asset .app zippé (barre de progression),
   - auto-install macOS : un helper détaché échange le bundle (.bak réversible) puis relance.

 100 % réseau sortant + fichiers locaux ; ne touche JAMAIS au synthé. L'auto-install
 est macOS-spécifique (le reste fonctionne partout où juce::URL est dispo).

 ==============================================================================
 */

#pragma once
#include <JuceHeader.h>
#if JUCE_MAC
 #include <unistd.h>   // getpid() pour le helper d'auto-install
#endif
#include "SysexUtils.h"     // SyVoice::isNewerVersion (pur + testé)
#include "AppSettings.h"    // getAppSettings()
#include "Version.h"        // Sysex77::kVersion

class Updater
{
public:
    struct Info
    {
        juce::String tag, name, notes, htmlUrl, assetUrl;
    };

    static Updater& get() { static Updater instance; return instance; }

    // Callbacks (branchés par l'UI, exécutés sur le thread message). Mis à null en fin de vie.
    std::function<void (Info)>          onUpdateAvailable;
    std::function<void (juce::String)>  onMessage;        // retour en mode manuel (à jour / erreur)

    static constexpr const char* kApiUrl =
        "https://api.github.com/repos/nseaSeb/Sysex77/releases/latest";

    //==============================================================================
    /** Vérifie la dernière release. `manual` = déclenché par l'utilisateur (ignore le throttle
        et la version « ignorée », et affiche un retour même si à jour). */
    void checkAsync (bool manual)
    {
        if (! manual)
        {
            const auto last = getAppSettings()->getValue ("LastUpdateCheck", "0").getLargeIntValue();
            const auto now  = juce::Time::getCurrentTime().toMilliseconds();
            if (now - last < (juce::int64) 24 * 3600 * 1000)   // throttle 24 h
                return;
            if (! getAppSettings()->getBoolValue ("CheckUpdatesOnStartup", true))
                return;
        }

        juce::Thread::launch ([this, manual]
        {
            int status = 0;
            auto opts = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                            .withConnectionTimeoutMs (8000)
                            .withStatusCode (&status)
                            .withExtraHeaders ("User-Agent: Sysex77\r\nAccept: application/vnd.github+json");

            std::unique_ptr<juce::InputStream> in (juce::URL (kApiUrl).createInputStream (opts));
            const juce::String body = in != nullptr ? in->readEntireStreamAsString() : juce::String();

            Info info;
            bool parsed = false;
            if (body.isNotEmpty() && status >= 200 && status < 300)
            {
                auto json = juce::JSON::parse (body);
                info.tag     = json.getProperty ("tag_name", {}).toString();
                info.name    = json.getProperty ("name", {}).toString();
                info.notes   = json.getProperty ("body", {}).toString();
                info.htmlUrl = json.getProperty ("html_url", {}).toString();
                if (auto* assets = json.getProperty ("assets", {}).getArray())
                    for (auto& a : *assets)
                    {
                        const auto n = a.getProperty ("name", {}).toString();
                        if (n.endsWithIgnoreCase (".zip"))
                        {
                            info.assetUrl = a.getProperty ("browser_download_url", {}).toString();
                            break;
                        }
                    }
                parsed = info.tag.isNotEmpty();
            }

            juce::MessageManager::callAsync ([this, info, manual, parsed]
            {
                getAppSettings()->setValue ("LastUpdateCheck",
                    juce::String (juce::Time::getCurrentTime().toMilliseconds()));
                getAppSettings()->saveIfNeeded();

                if (! parsed)
                {
                    if (manual && onMessage) onMessage ("Verification impossible (reseau ?)");
                    return;
                }

                const bool newer = SyVoice::isNewerVersion (info.tag, Sysex77::kVersion);
                const auto skip  = getAppSettings()->getValue ("SkipUpdateVersion", "");

                if (newer && info.assetUrl.isNotEmpty() && (manual || info.tag != skip))
                {
                    if (onUpdateAvailable) onUpdateAvailable (info);
                }
                else if (manual && onMessage)
                {
                    onMessage (newer ? "Mise a jour " + info.tag + " ignoree"
                                     : juce::String ("Vous avez la derniere version (")
                                         + Sysex77::kVersion + ")");
                }
            });
        });
    }

    /** Marque une version comme « ignorée » (ne plus proposer au démarrage). */
    void skipVersion (const juce::String& tag)
    {
        getAppSettings()->setValue ("SkipUpdateVersion", tag);
        getAppSettings()->saveIfNeeded();
    }

    //==============================================================================
    /** Télécharge l'asset (barre de progression modale) puis, sur succès, installe + relance.
        Renvoie après lancement du helper (l'app va se fermer). */
    void downloadAndInstall (const Info& info)
    {
        auto zip = juce::File::getSpecialLocation (juce::File::tempDirectory)
                       .getChildFile ("Sysex77-update.zip");

        DownloadWindow dl (info.assetUrl, zip);
        if (! dl.runThread() || ! dl.ok)      // annulé / échec
        {
            if (onMessage) onMessage ("Telechargement annule ou echoue");
            return;
        }
        installAndRelaunch (zip);
    }

private:
    Updater() = default;

    // Fenêtre de progression du téléchargement (lecture WebInputStream -> fichier).
    struct DownloadWindow : public juce::ThreadWithProgressWindow
    {
        juce::String url;
        juce::File   dest;
        bool         ok = false;

        DownloadWindow (juce::String u, juce::File d)
            : juce::ThreadWithProgressWindow ("Telechargement de la mise a jour...", true, true),
              url (std::move (u)), dest (std::move (d)) {}

        void run() override
        {
            juce::WebInputStream in (juce::URL (url), false);
            if (! in.connect (nullptr)) return;

            const auto total = in.getTotalLength();
            dest.deleteFile();
            juce::FileOutputStream out (dest);
            if (out.failedToOpen()) return;

            juce::HeapBlock<char> buffer (1 << 16);
            juce::int64 done = 0;
            while (! in.isExhausted() && ! threadShouldExit())
            {
                const int n = in.read (buffer, 1 << 16);
                if (n <= 0) break;
                out.write (buffer, (size_t) n);
                done += n;
                if (total > 0) setProgress (done / (double) total);
                setStatusMessage (juce::String (done / (1024 * 1024)) + " Mo telecharges...");
            }
            out.flush();
            ok = (! threadShouldExit()) && done > 0;
        }
    };

    // Installe la nouvelle version et relance (macOS). Ailleurs : ouvre la page release.
    void installAndRelaunch (const juce::File& zip)
    {
       #if JUCE_MAC
        auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Sysex77-update");
        tmp.deleteRecursively();
        tmp.createDirectory();

        // ditto préserve la structure du bundle (signature/symlinks).
        juce::ChildProcess ditto;
        ditto.start (juce::StringArray { "/usr/bin/ditto", "-x", "-k",
                                         zip.getFullPathName(), tmp.getFullPathName() });
        ditto.waitForProcessToFinish (60000);

        auto newApp = tmp.getChildFile ("Sysex77.app");
        auto oldApp = juce::File::getSpecialLocation (juce::File::currentApplicationFile); // .app en cours
        if (! newApp.exists() || ! oldApp.exists())
        {
            if (onMessage) onMessage ("Mise a jour : archive invalide");
            return;
        }

        // Emplacement non inscriptible (ex. /Applications sans droits) -> repli Finder.
        if (! oldApp.getParentDirectory().hasWriteAccess())
        {
            newApp.revealToUser();
            if (onMessage) onMessage ("Glissez la nouvelle app dans Applications (emplacement protege)");
            return;
        }

        // Helper détaché : attend la fermeture, échange (.bak réversible), nettoie la quarantaine, relance.
        auto script = tmp.getChildFile ("swap.sh");
        script.replaceWithText (
            "#!/bin/bash\n"
            "PID=\"$1\"; OLD=\"$2\"; NEW=\"$3\"\n"
            "while kill -0 \"$PID\" 2>/dev/null; do sleep 0.3; done\n"
            "sleep 0.5\n"
            "if mv \"$OLD\" \"$OLD.bak\"; then\n"
            "  if mv \"$NEW\" \"$OLD\"; then rm -rf \"$OLD.bak\"; else mv \"$OLD.bak\" \"$OLD\"; fi\n"
            "fi\n"
            "xattr -dr com.apple.quarantine \"$OLD\" 2>/dev/null\n"
            "open \"$OLD\"\n");

        const juce::String inner =
            "nohup /bin/bash " + script.getFullPathName().quoted()
            + " " + juce::String ((int) getpid())
            + " " + oldApp.getFullPathName().quoted()
            + " " + newApp.getFullPathName().quoted()
            + " >/dev/null 2>&1 &";

        juce::ChildProcess launcher;
        launcher.start (juce::StringArray { "/bin/bash", "-c", inner });

        juce::JUCEApplicationBase::quit();   // l'app se ferme -> le helper swap puis rouvre
       #else
        juce::ignoreUnused (zip);
        if (onMessage) onMessage ("Auto-install non disponible sur cette plateforme");
       #endif
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Updater)
};
