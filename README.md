# RepairFAQ: mirror and companion

A free, non-commercial mirror of [repairfaq.org](https://www.repairfaq.org/): the **Sci.Electronics.Repair FAQ**,
**Sam's Laser FAQ** and **Silicon Sam's Technology Resource**. It also adds a modern companion for exploring them:
search, a diagram explorer, and every ASCII circuit diagram redrawn as an animated schematic and a 3D board.

Live site: https://acamarocutcher.github.io/repairfaq/ (linked from [acamaro.com](https://acamaro.com/projects/)).

## Credit

All FAQ content is by **Samuel M. Goldwasser** and **Filip M. Gieszczykiewicz**, and by the contributors credited
"(From: …)" throughout. © the original authors, all rights reserved. The original and authoritative site is
https://www.repairfaq.org/. This mirror exists so the information survives, out of respect for their work.

## The terms this mirror follows

From the site's [Copyright and Fair Use Considerations](https://www.repairfaq.org/sam/copyright.htm):

- Pages in `archive/` are **byte-for-byte copies**. The only additions are:
  - an HTML comment at the top naming the source
  - a visible "Source:" line linking the original page
  - `companion/assets/mirror-theme.css`, which changes **fonts and colors only**. The terms permit this
    "to the extent required to be consistent with other pages on the Web site."
- Each document's "Author and Copyright" section stays at the top of its page. Each diagram page in the companion
  is an excerpt, and repeats its document's notice in full (`companion/data/notices.json`).
- **No ads, no charge, non-commercial.** Do not add analytics, ads or sponsored links to this repository.
- **Do not edit files in `archive/` by hand.** Rebuild them with the tools below.

## Layout

```
index.html                 companion landing page (with the 3D helium-neon laser)
companion/
  diagrams.html            diagram explorer
  diagram.html             one diagram: diagram.html#<id>, #<id>/3d or #<id>/ascii
  credits.html             authors, contributors, terms
  assets/                  companion.css/js, schematic.js (ASCII to SVG), schematic3d.js, hero3d.js, viewer.js,
                           mirror-theme.css (the fonts-and-colors stylesheet for archive/)
  vendor/                  three.js r160 (MIT, see three.LICENSE.txt)
  data/                    diagrams, notices, search index, contributors, stats (generated)
archive/                   the mirrored site, same paths as repairfaq.org (generated)
AI_DIAGRAM_GUIDE.md        brief for AI models that will build 3D versions of the diagrams (generated)
DIAGRAM_INDEX.md           every diagram, one line each (generated)
tools/                     the scripts that build all of the above
```

The three big photo galleries (`sam/laserpic`, `sam/reppic`, `sam/brochures`) are in the companion repository
[`repairfaq-photos`](https://github.com/AcamaroCutcher/repairfaq-photos), because GitHub Pages limits a site to 1 GB.
Pages here link to their photos there. 56 large videos, slide shows and programs (226 MB) are not hosted. Their
links point to repairfaq.org, and full copies are kept offline.

## Rebuilding

Everything except `companion/assets`, `companion/vendor` and this README is generated from a local download of the site.

```sh
# 1. download or refresh the site (polite: 4 workers, 0.5 s pause; resumable with Ctrl-C)
python3 -I tools/mirror.py crawl ../repairfaq_archive --rescan
python3 -I tools/mirror.py rewrite ../repairfaq_archive        # make links relative

# 2. build archive/, the photos repo and companion/data/
python3 -I tools/build.py ../repairfaq_archive . \
  --skip-ext mp4,wmv,avi,mov,ppsx,pps,exe \
  --photo-dirs sam/laserpic,sam/reppic,sam/brochures \
  --photos-site ../repairfaq-photos \
  --photos-base https://acamarocutcher.github.io/repairfaq-photos/archive/

# 3. companion pages and the AI guide
python3 -I tools/pages.py . ../repairfaq-photos
python3 -I tools/guide.py .
```

Preview with `python3 -m http.server 8000` from the folder above this one, then open
http://localhost:8000/repairfaq/. The 3D views use ES modules, which need a server: they won't load from a file:// URL.

## If you are one of the authors

If you would like anything changed or removed, please get in touch through [acamaro.com](https://acamaro.com/#contact),
and it will be done right away.
