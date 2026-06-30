#!/usr/bin/env python3
# Génère Source/SyParamTable.h depuis le JSON Electra (× translate du lua).
# enc : OB2 -> offBin2 ; SM -> signMag(half,signbit) ; display.min<0 -> offBin1(offset,maxWire) ; sinon plain.
import json, re

ROOT = "/Users/sebastienportrait/juce/Sysex77"
JSON = f"{ROOT}/docs/TG-77 Voice lua and json/TG77_Voice.json"
OUT  = f"{ROOT}/Source/SyParamTable.h"

# tables exactes du lua (main.lua l.43-60)
SM = {}
for op in range(1,7):
    SM[1000*op+0x0F]={'h':7,'s':8}; SM[1000*op+0x11]={'h':7,'s':8}; SM[1000*op+0x1A]={'h':15,'s':16}
SM[7201]={'h':7,'s':8}; SM[7100+0x29]={'h':12,'s':16}
SM[7300+0x33]={'h':7,'s':8}; SM[7300+0x34]={'h':7,'s':8}; SM[7300+0x10]={'h':7,'s':8}
OB2 = {7300+0x15,7300+0x16,7300+0x17,7300+0x18}

def zone(ui):
    if 1000<=ui<7000: return "afmOp", ui//1000, ui%1000
    if 7000<=ui<7100: return "afmCommon",0, ui-7000
    if 7100<=ui<7200: return "voiceCommon",0, ui-7100
    if 7200<=ui<7300: return "voiceElement",0, ui-7200
    if 7300<=ui<7400: return "filter",0, ui-7300
    if 7400<=ui<7500: return "effect",0, ui-7400
    return None,None,None

d=json.load(open(JSON))
pages={p["id"]:p.get("name","") for p in d.get("pages",[])}
rows=[]; seen=set()
for c in d.get("controls",[]):
    name=c.get("name","")
    for v in c.get("values",[]):
        msg=v.get("message",{}); ui=msg.get("parameterNumber")
        if ui is None: continue
        z,op,n2=zone(ui)
        if z is None: continue
        if ui in seen: continue          # dédup : 1re occurrence (contrôles répétés sur plusieurs pages)
        seen.add(ui)
        dmn,dmx=v.get("min",0),v.get("max",0)
        mmn,mmx=msg.get("min",dmn),msg.get("max",dmx)
        if ui in OB2:
            enc,a,b="offBin2",0,0
        elif ui in SM:
            enc,a,b="signMag",SM[ui]['h'],SM[ui]['s']
        elif dmn<0:
            enc,a,b="offBin1",-dmn,mmx          # offset, maxWire
        else:
            enc,a,b="plain",0,0
        rows.append((ui,name,z,op,n2,enc,a,b,dmn,dmx))
rows.sort()

def esc(s): return s.replace('"','\\"')
L=[]
L.append("/*  SyParamTable.h — GÉNÉRÉ (scratchpad/gen_param_table.py) — NE PAS éditer à la main.")
L.append("    Source : docs/TG-77 Voice lua and json/{TG77_Voice.json, main.lua} (bench-verified).")
L.append("    Dictionnaire déclaratif : un paramètre = (zone, op, N2, encodage, plage d'affichage).")
L.append("    L'adresse (group,T2,N2) se résout via syTranslate (Phase 3) ; l'encodage via SyEnc. */")
L.append("#pragma once")
L.append('#include "SysexUtils.h"')
L.append("")
L.append("namespace SyParam")
L.append("{")
L.append("    enum class Zone : int { afmOp, afmCommon, voiceCommon, voiceElement, filter, effect };")
L.append("")
L.append("    struct Entry")
L.append("    {")
L.append("        int            ui;       // parameterNumber (= ui du codec lua)")
L.append("        const char*    name;     // libellé Electra")
L.append("        Zone           zone;")
L.append("        int            op;       // 1..6 si afmOp, sinon 0")
L.append("        juce::uint8    n2;       // octet N2 du param-change")
L.append("        SyVoice::SyEnc enc;      // encodage affichage<->filaire")
L.append("        int            encA;     // s/m: half ; offBin1: offset ; sinon 0")
L.append("        int            encB;     // s/m: signbit ; offBin1: maxWire ; sinon 0")
L.append("        int            dispMin;  // plage d'AFFICHAGE (signée éventuellement)")
L.append("        int            dispMax;")
L.append("    };")
L.append("")
L.append(f"    static constexpr Entry kParams[] =")
L.append("    {")
for (ui,name,z,op,n2,enc,a,b,dmn,dmx) in rows:
    L.append(f'        {{ {ui:4d}, "{esc(name)}", Zone::{z}, {op}, 0x{n2:02X}, '
             f'SyVoice::SyEnc::{enc}, {a}, {b}, {dmn}, {dmx} }},')
L.append("    };")
L.append(f"    static constexpr int kNumParams = (int) (sizeof (kParams) / sizeof (kParams[0]));")
L.append("}")
L.append("")
open(OUT,"w").write("\n".join(L))

# récap encodages
from collections import Counter
cc=Counter(r[5] for r in rows)
print(f"{len(rows)} entrées -> {OUT}")
print("encodages :", dict(cc))
