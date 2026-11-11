| Field | Meaning |
|---|---|
| `id` | first 12 hex digits of the SHA-1 of the ASCII with all whitespace removed; stable across rebuilds |
| `page` | the page it appears on, relative to `archive/` |
| `collection` | `laser`, `sam`, `repair` or `other` |
| `heading`, `anchor` | nearest heading and nearest `<a name>` above it: open `archive/<page>#<anchor>` |
| `context` | up to 320 characters of the text just before the diagram |
| `parts`, `refs` | heuristic guesses: part types, and reference designators such as R1 or C4 |
| `ascii` | the diagram exactly as published, with tabs expanded and the common indent removed |
| `source` | the original URL on repairfaq.org |
| `notice`, `notice_from` | key into `notices.json`; `own` (the page's own notice), `parent` (the FAQ's main page) or `site` (site-wide terms) |
| `also` | other pages where the identical diagram appears |
