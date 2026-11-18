# PLEXIS FlexStrip

Channel strip VST3 (mono ou stéréo) par **PLEXIS Labs**, inspiré de la tranche d'entrée de l'interface Harrison Flex-10.
Projet JUCE 8 + CMake. Ce n'est pas un clone du matériel : le manuel ne documente ni les pentes de filtres ni la courbe de saturation.

## Chaîne de signal

Pad -20 dB → Polarité → Gain → HP (18 dB/oct) → LP (12 dB/oct) → H-DRIVE → mètre LED → Output → VU

| Contrôle | Détail |
|---|---|
| Source | Mic / Line / Inst : change la plage de gain (63 dB / 27 dB / 63 dB) |
| Gain | 0–10, lecture en dB selon la source |
| -20, Ø | Pad 20 dB, inversion de polarité |
| Filtres | HP 25 Hz–3,15 kHz, LP 160 Hz–20 kHz, bouton IN commun |
| H-DRIVE | Saturation de type transformateur (shelf grave + tanh asymétrique, anti-aliasing ADAA), niveau compensé ; le bouton passe de l'ambre au rouge selon l'intensité |
| Mètre LED | 7 segments : 0, -6, -12, -18, -24, -30, -36 dBFS (après H-DRIVE) |
| VU | Deux aiguilles (L/R), balistique VU (99 % en 300 ms), 0 VU = -18 dBFS, réglable avec VU TRIM |
| Output | -24 à +12 dB |

## Hypothèses (non documentées dans le manuel)

- Pentes des filtres : HP 18 dB/oct, LP 12 dB/oct (Butterworth).
- H-DRIVE : modèle d'inspiration, pas une mesure du matériel.
- Le 48 V, l'insert physique et la section Master matérielle (ADAT, casques, direct monitoring) n'ont pas d'équivalent dans un plugin et sont omis.
- Les marques « Harrison » et « Flex-10 » et le logo Harrison ne sont pas utilisés ; le logo affiché est une marque originale PLEXIS.

## Compilation

Prérequis : CMake ≥ 3.22, un compilateur C++17 (Visual Studio 2022 sous Windows, Xcode sous macOS), Git (JUCE 8.0.4 est téléchargé automatiquement).

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target PlexisFlexStrip_VST3
```

Le VST3 se trouve dans `build/PlexisFlexStrip_artefacts/Release/VST3/`. Copier le dossier `PLEXIS FlexStrip.vst3` dans :
- Windows : `C:\Program Files\Common Files\VST3`
- macOS : `~/Library/Audio/Plug-Ins/VST3`

Une version Standalone est aussi construite (cible `PlexisFlexStrip_Standalone`).

## Compilation via GitHub Actions (Windows)

Le fichier `.github/workflows/build.yml` compile le VST3 sur un serveur Windows de GitHub à chaque `git push`.
Onglet **Actions** du dépôt → dernier run → section **Artifacts** → télécharger `PLEXIS-FlexStrip-VST3-Windows`,
puis copier le dossier `PLEXIS FlexStrip.vst3` dans `C:\Program Files\Common Files\VST3`.

## Tests

```
cmake -S . -B build -DFLEXSTRIP_BUILD_TESTS=ON
cmake --build build --target FlexStripDspTests
./build/FlexStripDspTests
```

Les tests DSP (signaux synthétiques) vérifient : gain 63/27 dB, pad, polarité, pentes et fréquences de coupure des filtres, transparence des bypass, croissance des harmoniques du H-DRIVE, niveaux des mètres, balistique du VU, absence de clics au changement de paramètres.
