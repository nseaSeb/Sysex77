#!/usr/bin/env python3
# Réconciliation : Electra JSON (TG77_Voice.json) + codec lua (main.lua, oracle)
#                  vs notre app (sysex émis dans Source/*.h) + MAP.md (statut).
# Sortie : tableau classé concordant / divergent / manquant, en Markdown.
import json, re, glob, os, collections

ROOT = "/Users/sebastienportrait/juce/Sysex77"
JSON = f"{ROOT}/docs/TG-77 Voice lua and json/TG77_Voice.json"
SRC  = f"{ROOT}/Source"

# ---------- 1) translate() du lua, ré-implémenté fidèlement ----------
def elbits(el):  # el 0..3
    return (el & 3) << 5

# SM (sign-magnitude) et OB2 (offset-binary 2 octets) : encodage par ui
SM = {}
for op in range(1, 7):
    SM[1000*op + 0x0F] = "s/m"   # RS rate scaling -7..+7
    SM[1000*op + 0x11] = "s/m"   # VSON velocity   -7..+7
    SM[1000*op + 0x1A] = "s/m"   # FP0 detune      -15..+15
SM[7201] = "s/m"; SM[7100+0x29] = "s/m"
SM[7300+0x33] = "s/m"; SM[7300+0x34] = "s/m"; SM[7300+0x10] = "s/m"
OB2 = {7300+0x15, 7300+0x16, 7300+0x17, 7300+0x18}

def translate(ui):
    """returns (group, N2, encoding, zone) ; group sans bits élément/op variables sauf AFM op."""
    if 1000 <= ui < 7000:
        op = ui // 1000; n2 = ui % 1000
        if 1 <= op <= 6:
            g = ((6-op) << 4) | 0x06
            return g, n2, enc(ui), f"AFM op{op}"
    elif 7000 <= ui < 7100:
        return 0x05, ui-7000, enc(ui), "AFM common"
    elif 7100 <= ui < 7200:
        return 0x02, ui-7100, enc(ui), "Voice common"
    elif 7200 <= ui < 7300:
        return 0x03, ui-7200, enc(ui), "Voice element"
    elif 7300 <= ui < 7400:
        return 0x09, ui-7300, enc(ui), "Filter"
    elif 7400 <= ui < 7500:
        return 0x08, ui-7400, enc(ui), "Effects"
    return None, None, None, None

def enc(ui):
    if ui in OB2: return "o/b2"
    if ui in SM:  return "s/m"
    return "plain"

# ---------- 2) Electra JSON : parameterNumber -> nom + plage ----------
d = json.load(open(JSON))
pages  = {p["id"]: p.get("name","") for p in d.get("pages",[])}
groups = {g["id"]: g.get("name","") for g in d.get("groups",[])}
master = {}  # ui -> dict
for c in d.get("controls", []):
    name = c.get("name","")
    page = pages.get(c.get("pageId"), "")
    for v in c.get("values", []):
        msg = v.get("message", {})
        ui = msg.get("parameterNumber")
        if ui is None: continue
        g, n2, e, zone = translate(ui)
        if g is None:    # non transmissible (ui virtuel type 9001, etc.)
            continue
        master[ui] = dict(name=name, page=page, ui=ui, group=g, n2=n2,
                          enc=e, zone=zone, mn=v.get("min"), mx=v.get("max"))

# ---------- 3) Notre app : couples (group, N2) réellement émis ----------
# Cherche des tableaux de 9 valeurs sysex {0x43, .., 0x34, G, AH, 0x00, N2, ..}
app_pairs = collections.Counter()   # (group, n2) -> nb de sites
app_sites = collections.defaultdict(list)
hexnum = r'(?:0[xX][0-9a-fA-F]+|\d+)'
arr = re.compile(r'\{\s*(0[xX]43)\s*,\s*([^,]+),\s*(0[xX]34)\s*,\s*('+hexnum+r')\s*,\s*([^,]+),\s*([^,]+),\s*('+hexnum+r')\s*,')
def toint(s):
    s=s.strip()
    try: return int(s,16) if s.lower().startswith('0x') else int(s)
    except: return None
for f in glob.glob(f"{SRC}/*.h"):
    base=os.path.basename(f)
    for ln,line in enumerate(open(f,errors='replace'),1):
        for m in arr.finditer(line):
            g = toint(m.group(4)); n2 = toint(m.group(7))
            if g is None or n2 is None:  # N2 variable (n2, param...) -> non résolu
                continue
            app_pairs[(g,n2)] += 1
            if len(app_sites[(g,n2)])<3: app_sites[(g,n2)].append(f"{base}:{ln}")

# ---------- 4) MAP.md : couverture déclarée (best-effort, par token de nom) ----------
mapmd = open(f"{ROOT}/MAP.md",errors='replace').read()
mapmd_low = mapmd.lower()
# base-token d'un nom Electra (sans suffixe OPn) -> alias pour matcher MAP.md
def basetok(name):
    t = re.sub(r'\s*OP[1-6]\s*$','',name).strip()
    return t
ALIAS = {  # nom Electra -> tokens probables dans MAP.md (fr/abréviations)
 "LEVEL":["tl","niveau","level"], "WAVE":["waveform","wave"], "DETUNE":["detune","fpd","détune"],
 "RS":["rate scaling","scaling","rs"], "AMS":["sensit","am","ams"], "VEL":["vel","velocity","sensit"],
 "ELVL":["elvl","element level","niveau"], "VVOL":["vvol","volume"], "ELDT":["eldt","element detune"],
}
def documented(info):
    bt = basetok(info["name"])
    toks = ALIAS.get(bt, [bt.lower()])
    return any(tk and tk in mapmd_low for tk in toks)

# ---------- 5) Classement ----------
def app_has(group, n2):
    if (group,n2) in app_pairs: return True
    # AFM op : l'app peut diffuser via un seul groupe puis broadcast -> teste toute la famille op
    if (group & 0x0F)==0x06:
        return any((gg,n2) in app_pairs for gg in (0x06,0x16,0x26,0x36,0x46,0x56))
    return False

def emits_literal(group, n2):
    if (group,n2) in app_pairs: return True
    if (group & 0x0F)==0x06:   # AFM op : famille de groupes
        return any((gg,n2) in app_pairs for gg in (0x06,0x16,0x26,0x36,0x46,0x56))
    return False

rows=[]
for ui,info in sorted(master.items()):
    rows.append({**info,
                 "doc": documented(info),                       # mentionné dans MAP.md (déclaratif)
                 "lit": emits_literal(info["group"], info["n2"])}) # littéral sysex trouvé (preuve directe)

# divergences : (group,N2) que l'app émet mais absents du master lua/Electra
master_keys = {(translate(ui)[0], translate(ui)[1]) for ui in master}
extra = sorted([(g,n2,app_sites[(g,n2)]) for (g,n2) in app_pairs if (g,n2) not in master_keys
                and g in (0x02,0x03,0x05,0x08,0x09,0x06,0x16,0x26,0x36,0x46,0x56)])

# ---------- 6) Rapport ----------
out=[]
out.append("# Audit de réconciliation — Electra/lua (oracle) vs notre app\n")
out.append("Méthode : **master** = JSON Electra (241 contrôles) × `translate()` du codec lua "
           "(adresses + encodage, bench-verified). Côté app, 2 signaux : *doc* = mentionné dans "
           "`MAP.md` (notre couverture déclarée) ; *lit* = littéral sysex (group,N2) trouvé dans le "
           "code (preuve directe, MAIS sous-détecte car la plupart des adresses sont calculées à "
           "l'exécution → ne PAS lire ce signal comme « manquant »).\n")
out.append(f"- Master transmissible : **{len(master)}** paramètres (zones AFM op×6 / AFM common / "
           "Voice common / Voice element / Filter / Effects).")
ndoc = sum(1 for r in rows if r['doc'])
out.append(f"- Mentionnés dans MAP.md (indicatif) : **{ndoc}/{len(rows)}**.")
out.append(f"- Littéraux sysex résolus dans le code : **{len(app_pairs)}** couples (le reste est "
           "calculé → non comptable ainsi).\n")

# Récap encodage : les params signés (s/m) et offset-binary (o/b2) = sources de bugs historiques.
encc = collections.Counter(r['enc'] for r in rows)
out.append("## Encodage (à porter tel quel — pièges de signe/offset)\n")
out.append(f"- **plain** : {encc['plain']}  ·  **s/m (signé)** : {encc['s/m']}  ·  **o/b2 (offset-binary 2 octets)** : {encc['o/b2']}")
sm = [r for r in rows if r['enc']!='plain']
out.append("\nParams NON-plain (encodage spécial) :\n")
out.append("| ui | nom | group | N2 | enc | plage |")
out.append("|---|---|---|---|---|---|")
for r in sm:
    out.append(f"| {r['ui']} | {r['name']} | 0x{r['group']:02X} | 0x{r['n2']:02X} | {r['enc']} | {r['mn']}..{r['mx']} |")
out.append("")

byzone=collections.defaultdict(lambda:[0,0])
for r in rows:
    byzone[r['zone']][0]+= 1
    byzone[r['zone']][1]+= 1 if r['doc'] else 0
out.append("## Couverture déclarée (MAP.md) par zone — INDICATIF (sous-détecte : voir note)\n")
out.append("> ⚠️ « non mentionnés » ≠ « manquants dans l'app » : MAP.md est en prose et ne nomme pas "
           "les params Effects/Filter/Voice-common par leur nom Electra, et les éditeurs concernés "
           "existent. Fiable uniquement comme ordre de grandeur.\n")
out.append("| Zone | total master | mentionnés MAP | non mentionnés |")
out.append("|---|---|---|---|")
for z,(t,c) in sorted(byzone.items()):
    out.append(f"| {z} | {t} | {c} | {t-c} |")

out.append("\n## TABLE MASTER COMPLÈTE (source de vérité Electra+lua)\n")
out.append("Colonnes : ui = parameterNumber ; group/N2 = adresse param-change ; enc = encodage "
           "(plain / s/m signé / o/b2 offset-binary 2 octets) ; doc = présent MAP.md ; lit = littéral code.\n")
out.append("| ui | nom | zone | group | N2 | enc | plage | doc | lit |")
out.append("|---|---|---|---|---|---|---|:--:|:--:|")
for r in rows:
    out.append(f"| {r['ui']} | {r['name']} | {r['zone']} | 0x{r['group']:02X} | 0x{r['n2']:02X} | "
               f"{r['enc']} | {r['mn']}..{r['mx']} | {'✓' if r['doc'] else '·'} | {'✓' if r['lit'] else '·'} |")

out.append("\n## (group,N2) émis en LITTÉRAL par l'app SANS équivalent master (divergence à vérifier)\n")
out.append("| group | N2 | sites |")
out.append("|---|---|---|")
for g,n2,sites in extra:
    out.append(f"| 0x{g:02X} | 0x{n2:02X} | {', '.join(sites)} |")
if not extra: out.append("| — | — | (aucune) |")

rep="\n".join(out)
open("/Users/sebastienportrait/juce/Sysex77/docs/reconciliation_audit.md","w").write(rep)
print(rep[:3000])
print("\n...[rapport complet écrit dans scratchpad/reconciliation_audit.md]")
