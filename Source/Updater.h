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
        // /tmp (et NON ~/Library/Caches = File::tempDirectory) : le helper d'install tourne sous
        // launchd, qui n'a pas accès à ~/Library/Caches (TCC) -> le swap échouait silencieusement.
        auto zip = juce::File ("/tmp/Sysex77-update.zip");

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
            juce::File ("/tmp/sysex77-update.log").appendText (
                juce::Time::getCurrentTime().toString (true, true) + "  download: " + url + "\n");
            juce::WebInputStream in (juce::URL (url), false);
            if (! in.connect (nullptr))
            {
                juce::File ("/tmp/sysex77-update.log").appendText ("  connect FAILED\n");
                return;
            }

            const auto total = in.getTotalLength();
            dest.deleteFile();
            juce::FileOutputStream out (dest);
            if (out.failedToOpen())
            {
                juce::File ("/tmp/sysex77-update.log").appendText ("  output open FAILED\n");
                return;
            }

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
            juce::File ("/tmp/sysex77-update.log").appendText (
                "  download done=" + juce::String (done) + " total=" + juce::String (total)
                + " ok=" + juce::String ((int) ok) + "\n");
        }
    };

    // Installe la nouvelle version et relance (macOS). Ailleurs : ouvre la page release.
    void installAndRelaunch (const juce::File& zip)
    {
       #if JUCE_MAC
        auto tmp = juce::File ("/tmp/Sysex77-update");   // /tmp accessible par le job launchd (cf. download)
        tmp.deleteRecursively();
        tmp.createDirectory();

        // ditto préserve la structure du bundle (signature/symlinks).
        juce::ChildProcess ditto;
        ditto.start (juce::StringArray { "/usr/bin/ditto", "-x", "-k",
                                         zip.getFullPathName(), tmp.getFullPathName() });
        ditto.waitForProcessToFinish (60000);

        auto newApp = tmp.getChildFile ("Sysex77.app");
        auto oldApp = juce::File::getSpecialLocation (juce::File::currentApplicationFile); // .app en cours

        auto logf = juce::File ("/tmp/sysex77-update.log");
        auto log  = [&logf] (const juce::String& m)
        { logf.appendText (juce::Time::getCurrentTime().toString (true, true) + "  " + m + "\n"); };
        log ("--- install: zip=" + zip.getFullPathName());
        log ("newApp=" + newApp.getFullPathName() + " exists=" + juce::String ((int) newApp.exists()));
        log ("oldApp=" + oldApp.getFullPathName() + " exists=" + juce::String ((int) oldApp.exists()));

        if (! newApp.exists() || ! oldApp.exists())
        {
            log ("ABORT: archive invalide");
            if (onMessage) onMessage ("Mise a jour : archive invalide (voir /tmp/sysex77-update.log)");
            return;
        }

        // App Translocation : si l'app tourne depuis une copie en lecture seule (quarantaine non
        // levée), currentApplicationFile pointe une copie aléatoire -> on ne peut pas remplacer
        // l'original. Repli : révéler la nouvelle app dans le Finder.
        const bool translocated = oldApp.getFullPathName().contains ("/AppTranslocation/");
        const bool writable     = oldApp.getParentDirectory().hasWriteAccess();
        log ("translocated=" + juce::String ((int) translocated) + " parentWritable=" + juce::String ((int) writable));

        if (translocated || ! writable)
        {
            newApp.revealToUser();
            log ("FALLBACK Finder (translocated/non inscriptible)");
            if (onMessage) onMessage (translocated
                ? "MAJ : l'app tourne depuis une copie protegee (translocation). Glissez la nouvelle app revelee dans Applications."
                : "MAJ : emplacement protege. Glissez la nouvelle app revelee dans Applications.");
            return;
        }

        // Helper : attend la fermeture de l'app, échange le bundle (.bak réversible), nettoie la
        // quarantaine, relance. Une instruction par ligne + TRACE dans le log.
        // CRUCIAL : écrire en fins de ligne LF ("\n"). replaceWithText utilise CRLF PAR DÉFAUT, et
        // bash ne sait pas parser un script CRLF (le \r se colle à fi/then/done -> erreur de syntaxe,
        // exit 2) — c'était LA cause de l'échec silencieux de l'auto-install.
        auto script = tmp.getChildFile ("swap.sh");
        script.replaceWithText (
            "#!/bin/bash\n"
            "PID=\"$1\"; OLD=\"$2\"; NEW=\"$3\"; LOG=/tmp/sysex77-update.log\n"
            "echo \"$(date +%T)  swap start PID=$PID\" >> \"$LOG\"\n"
            "while kill -0 \"$PID\" 2>/dev/null; do sleep 0.3; done\n"
            "echo \"$(date +%T)  app exited\" >> \"$LOG\"\n"
            "sleep 0.5\n"
            "rm -rf \"$OLD.bak\"\n"
            "mv \"$OLD\" \"$OLD.bak\"\n"
            "if ditto \"$NEW\" \"$OLD\"; then\n"
            "  rm -rf \"$OLD.bak\"\n"
            "  echo \"$(date +%T)  swap OK\" >> \"$LOG\"\n"
            "else\n"
            "  rm -rf \"$OLD\"\n"
            "  mv \"$OLD.bak\" \"$OLD\"\n"
            "  echo \"$(date +%T)  ditto FAIL -> restore\" >> \"$LOG\"\n"
            "fi\n"
            "xattr -dr com.apple.quarantine \"$OLD\" 2>/dev/null\n"
            "echo \"$(date +%T)  reopening\" >> \"$LOG\"\n"
            "open \"$OLD\"\n"
            // Se désinscrire de launchd EN DERNIER : sinon launchd, voyant le job se terminer vite,
            // le relance toutes les ~10 s (boucle de ré-ouverture).
            "launchctl remove com.sysex77.update 2>/dev/null\n",
            false, false, "\n");   // <-- LF explicite (sinon CRLF -> bash casse)

        // DÉTACHEMENT FIABLE : on confie le helper à launchd via `launchctl submit`. Un processus
        // simplement backgroundé (nohup / setsid / std::system) mourait à la fermeture de l'app GUI
        // (groupe/session détruits par launchd) ; un job launchd, lui, est totalement indépendant.
        const juce::String label = "com.sysex77.update";
        { juce::ChildProcess rm; rm.start (juce::StringArray { "/bin/launchctl", "remove", label });
          rm.waitForProcessToFinish (3000); }                       // nettoie un éventuel job précédent

        juce::ChildProcess lc;
        lc.start (juce::StringArray { "/bin/launchctl", "submit", "-l", label, "--",
                                      "/bin/bash", script.getFullPathName(),
                                      juce::String ((int) getpid()),
                                      oldApp.getFullPathName(), newApp.getFullPathName() });
        const bool submitted = lc.waitForProcessToFinish (5000);
        log ("launchctl submit ok=" + juce::String ((int) submitted));

        juce::JUCEApplicationBase::quit();   // l'app se ferme -> le job launchd swap puis rouvre
       #else
        juce::ignoreUnused (zip);
        if (onMessage) onMessage ("Auto-install non disponible sur cette plateforme");
       #endif
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Updater)
};
