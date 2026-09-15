# Source survey — the twenty-five "closed binary" plugins

Every plugin in `research/help/mancheck/` was treated as closed source because its installed `plugin.json` carries no `sourceUrl`. This survey asks whether source exists anyway, published somewhere the manifest does not point at.

For each plugin I looked at: the declared manual URL; the other declared URLs (`pluginUrl`, `authorUrl`, `changelogUrl`); the maker's GitHub or Codeberg account where one could be identified; the VCV library page at `library.vcvrack.com`; and a web search on the plugin and author names. The VCV library pages turned out to add nothing — every one of the twenty-five renders exactly the URLs already in the manifest, so that step is not repeated below unless it found something.

A repository counts only if it holds a `src` directory with the actual module code, and its `plugin.json` slug matches the installed plugin. A repository belonging to the same maker but carrying a different plugin slug does not count, and several near-misses of exactly that kind appear below.

Nothing in the mancheck worksheets was changed.

---

## 24conditions — installed 2.1.0

Declared manual is `j-he.net/24condmain`, which returns 404. The author's site at `j-he.net` is a portfolio page with social links and no code. The GitHub account `jhe` belongs to a different person (Jeff Rozic, web developer, no audio work). No account matching "24conditions" exists on GitHub. Licence is declared proprietary.

**Source publicly available: no.**

## BastlPizza — installed 2.2.0

Declared manual is the Bastl hardware product page. Bastl does have a GitHub organisation, `bastl-instruments`, and it does have a VCV repository: `bastl-instruments/bastl-vcv`, mirrored at `hemmer/bastl-vcv`. That repository is a different plugin — slug `Bastl`, version 2.1.0, modules Kompas and ABC. The installed BastlPizza plugin is slug `BastlPizza` with modules Crust, Pizza and Basil, and none of those appear in the repository. The co-author Ewan Hemingway publishes his other VCV work openly under `hemmer` (Befaco, Audible Instruments, Rebel Tech) but has no Pizza repository. Press coverage of the plugin states the Pizza collection is closed source.

**Source publicly available: no.**

## Blamsoft-XFXDistortionPack — installed 2.0.2

Declared manual is a PDF on blamsoft.com. The Blamsoft GitHub organisation exists but has zero public repositories. The product page carries no repository link. Licence proprietary.

**Source publicly available: no.**

## DanTModules — installed 2.7.2

Declared manual is `miff-real.github.io/DanTModules-Manual`, which is a manual site; its repository, `Miff-Real/DanTModules-Manual`, holds documentation only. The same author does publish an open plugin: `Miff-Real/DanT.Synth`, GPL-3.0, with a real `src` directory. That is a different plugin — slug `DanTSynth`, version 2.0.2, two modules (AOCR, Bend). Installed DanTModules is slug `DanTModules`, version 2.7.2, twenty-one modules, none of which appear there. The author's other repositories (`vcv-dev-dant-method`, `scrapbook`) are notes, not code.

**Source publicly available: no.**

## DarkProcessIndustries — installed 2.3.666

All three declared URLs point at the same page on darkprocessindustries.com, which currently returns a WordPress fatal error — the site is down, so the maker's own pages could not be read. No GitHub or Codeberg account matching the brand or the author name (Sarah Rex) could be found, and no search result points at a repository. Licence proprietary.

**Source publicly available: no source found, but this one is not fully settled — the maker's own site was unreachable.**

## FrequencyDomain — installed 2.0.3

The known case, confirmed. Declared manual and changelog both point into `github.com/almostEric/Frequency-Domain`. The master branch has `src` (six files), `res`, a Makefile, and a `plugin.json` reading slug `FrequencyDomain`, version 2.0.3, nine modules — the same version as the installed build. Licence in the manifest is GPL-3.0-only. Two stale development branches also exist (1.8.5 and 1.9.1); master is the one that matches.

**Source publicly available: yes. `github.com/almostEric/Frequency-Domain`, master, version 2.0.3, exact match.**

## Hora-Mixers — installed 2.1.4

## Hora-ModulationFree — installed 2.1.4

## Hora-PCMDrumFree — installed 2.1.3

## Hora-ProcessorsFree — installed 2.2.4

## Hora-VCO_VCF_VCA_Free — installed 2.2.5

Treated together because their manifests are identical in shape: all five point at product pages on hora-music.wifeo.com and the shared manuals page there. That host has a broken TLS certificate and returned an empty body, so the manuals page itself could not be read. Two Hora GitHub accounts exist, `Hora-Music` and `HoRaMusic`. Neither holds any of these plugins. What `HoRaMusic` holds is Geco, described as a "VCV plugin Maker" — a code generator — plus Geco examples, a block-model editor, hardware firmware, and an abandoned manifest repository. That is the tooling the plugins were built with, not the modules. All five are declared proprietary.

**Source publicly available: no, for all five.**

## JPFree — installed 2.1.1

Declared manual, plugin URL and changelog all point at `github.com/patheros/JPManuals`. That repository's own README says it plainly: it contains the manuals and the issue tracker for the JP plugins. No `src`, no `plugin.json`. The author, Andrew Hanson, does publish one plugin openly — `patheros/PathSetModules`, GPL-3.0, slug `PathSet`, version 2.5.0 — but that is a different plugin from JPFree and shares none of its modules. Licence declared as the VCV EULA.

**Source publicly available: no.**

## Moffenzeef — installed 2.6.1

The find. The manifest declares only moffenzeefmodular.com for plugin, author and manual, and no `sourceUrl` — but the licence field reads GPL-3.0+, which was the clue worth following. Moffenzeef Modular has a GitHub organisation with two VCV repositories. `moffenzeefmodular/VCVrack-submission` (GPL-3.0) carries `src` with thirty files and a `plugin.json` reading slug `Moffenzeef`, version 2.6.1 — the installed version exactly — with all twenty-six module slugs identical to the installed set, from 2hpBlank through Tehom. The sibling repository `moffenzeefmodular/Moffenzeef-VCV` is the MetaModule port, slug `Moffenzeef` at 2.1.0 with twenty-one modules, and is not the match.

**Source publicly available: yes. `github.com/moffenzeefmodular/VCVrack-submission`, main, version 2.6.1, exact match on version and on all twenty-six module slugs.**

## NANOModules — installed 2.3.9

Declared URLs all point at nano-modules.com, which now redirects to nanoindustri.es. That site mentions the VCV modules but links no repository. No GitHub account under `nano-modules`, `nanomodules`, or the author name Jorge Gutierrez holds the plugin. Licence proprietary.

**Source publicly available: no.**

## Noumenal-B — installed 2.0.1

Promising on the face of it: the declared manual is `github.com/partvishegy/Noumenal` and the author URL is that GitHub account. The repository is documentation only — a README, a changelog and an `img` directory. There is no `src` and no `plugin.json` on any branch, and the `v2.0.1` tag holds the same documentation-only tree. The rest of the account is unrelated machine-learning work. Licence proprietary.

**Source publicly available: no. This is a manual repository wearing a source repository's clothes.**

## OMEg1 — installed 2.3.0

Same shape as Noumenal. Manual and changelog both point at `github.com/phestrada/OME`, which contains a README and four PNG panel images and nothing else. Tags 2.0.0 and 2.1.0 exist but carry no `plugin.json`. It is the author's only repository. Licence proprietary.

**Source publicly available: no.**

## OhmerPrems — installed 2.6.13

Manual, plugin and author URLs all point into `github.com/DomiKamu/OhmerPrems`, and the repository is active (pushed this month, tags up to v2.6.14). But the v2 branch holds only two licence files, a README and a `docs` directory — no `src`, no `plugin.json`. The v1 branch is the same. Dominique Camus does publish an open plugin next door, `DomiKamu/Ohmer`, slug `Ohmer` version 2.6.14, with modules RKD, BRK, Metriks, KlokSpid and the blanks. The installed OhmerPrems modules (FranKe, FroeZe, KlokSpidMkII, KordZ, QuadPercs, Vektor, 6OPDX and the rest) are a completely disjoint set. The premium plugin is deliberately kept closed alongside the free open one.

**Source publicly available: no.**

## OmriCohenPatheros-Free — installed 2.1.0

Plugin URL, manual and changelog all point at `github.com/patheros/PathSetXOmriCohen`, whose own description states it holds the manuals and issues for the plugins. No `src`, no `plugin.json`. As with JPFree, the author's one open plugin is `PathSetModules`, a different slug with different modules. Licence declared as the VCV EULA.

**Source publicly available: no.**

## PathSet-GlassShard — installed 2.0.1

Plugin URL, manual and changelog point at `github.com/patheros/PathSetManuals`. That repository's README is a manual index — Glass Shard, Glass Smith, and the Rainbows modules, each linking to a Markdown manual page. There is no `src` anywhere in it. The open `PathSetModules` repository (slug `PathSet`, 2.5.0) contains ShiftyMod, IceTray, AstroVibe, GlassPane, PlusPane, ShiftyExpander, Nudge and OneShot — not the Glass Shard modules. Note that GlassPane in the open plugin is a different module from GlassShard; the names are close enough to mislead.

**Source publicly available: no.**

## PathSet-Rainbows — installed 2.0.0

Same repository, same finding as Glass Shard: `PathSetManuals` documents the Rainbows modules (Bridge, Crossing, Grid, Ring and the rest) but holds no code, and none of those modules appear in the open `PathSetModules` plugin.

**Source publicly available: no.**

## SynthesizersDotCom — installed 2.0.5

Authored by Vult-DSP. Declared manual is the Synthesizers.com shop. The only relevant GitHub presence is `modlfo`, whose `VultModules` repository (see below) is documentation and an issue tracker, not code, and in any case is a different plugin. Nothing under `modlfo` corresponds to this plugin. Licence proprietary.

**Source publicly available: no.**

## Virtue-Control — installed 2.0.9

All declared URLs point at kilpatrickaudio.com. Kilpatrick Audio has a GitHub account with seven repositories: hardware firmware for CARBON, PHENOL, K1600, K4815, K2579, plus `Kilpatrick-Toolbox`, which is a different and openly published VCV plugin. The Virtue product page links only to the vMIDI specification document inside the Toolbox repository — a protocol document, not the Virtue source. Licence proprietary.

**Source publicly available: no.**

## VultModules — installed 2.0.17

## VultModulesFree — installed 2.0.17

Both point at `modlfo.github.io/VultModules`. The backing repository, `github.com/modlfo/VultModules`, is real and actively maintained, but its master branch contains exactly two files: a README and an artwork licence. The README describes the repository's purpose as the issue tracker and the documentation source; the documentation itself lives on the `page-src` and `gh-pages` branches. I checked the release tags as well — `v2.0.12` and `v2.0.8` both carry the same two-file tree, so the code was never in this repository rather than having been removed recently. Vult modules are generated from the Vult DSP language, whose compiler is open (`modlfo/vult`), but the module sources are not published. Both plugins are declared proprietary.

**Source publicly available: no, for both.**

## factionoptions — installed 2.0.17

Declared manual is factionoptions.com/manual, which links only to the site and the plugin download and mentions no repository. No GitHub account matches the brand; the `JasonSoares` account that matches the author's name holds unrelated web-development work and no VCV plugin. Licence proprietary.

**Source publicly available: no.**

---

## Count

**Publicly available source that we did not use: 2 of 25.** FrequencyDomain (already known) and Moffenzeef (new). Both are exact version matches — 2.0.3 and 2.6.1 respectively — and both declare a GPL licence in the manifest while declaring no `sourceUrl`.

**Genuinely no public source: 22 of 25.**

**Uncertain: 1 of 25.** DarkProcessIndustries, only because the maker's own website is currently returning a server error and could not be read. No repository exists for it under any identifiable account, so the likely answer is no.

The practical upshot: of the twenty-five plugins read out of a compiled binary, twenty-four really were closed, and one — Moffenzeef, twenty-six modules — has complete GPL source for the exact installed version sitting in a repository the manifest never mentions. Moffenzeef is worth re-doing from source. The Hora set, the Vult set and the Path Set family are the three clusters where a maker publishes some plugins openly and deliberately keeps these ones closed, so there is no point returning to them.

One pattern is worth recording for any future sweep. A declared manual URL that points at a GitHub repository proves nothing on its own: six of the twenty-five do that, and only one of those six (FrequencyDomain) is a source repository. `patheros/PathSetManuals`, `patheros/JPManuals`, `patheros/PathSetXOmriCohen`, `partvishegy/Noumenal` and `phestrada/OME` are all documentation repositories. The field that actually predicted a find was the licence: a GPL declaration on a plugin with no `sourceUrl` is a contradiction worth chasing, and it is what turned up Moffenzeef.
