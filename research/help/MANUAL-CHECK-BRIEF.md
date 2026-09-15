# What the maker's own documentation says

Read [STYLE.md](STYLE.md) and [NEW-PLUGIN-BRIEF.md](NEW-PLUGIN-BRIEF.md) first. This is a different job from writing an entry, and the difference matters: **you are reading a document to find out what it does not say.** That is an unusual task and an easy one to do carelessly, because a careless reader finds what they expect.

## Why

Twenty-five plugins in this library publish no source code, so everything we know about their ports was read out of the compiled binary. Each of them does publish *something* — a manual, a README, a product page. The question is simple and has never been measured: **for each fact we took from a binary, does the maker's own documentation say the same thing, something different, or nothing at all?**

That count decides what a public version of this data can contain. It is not a formality and it is not a search for agreement.

## Your plugin

One plugin per agent. The worksheet names it, gives the URL the installed `plugin.json` declares as its manual, and lists every port whose `why` cites the binary.

Some of those URLs are real manuals. Others are a shop listing or a company home page, and will say nothing about a voltage anywhere. **Both outcomes are results.** A page that documents nothing is as much an answer as a page that documents everything, and you must not go hunting for a better source than the one the maker declared — if the maker points at a shop page, the shop page is what the maker published.

Read the whole document. `pypdf` reads PDFs. For a web page, fetch it and read the text.

## What to record

For **every port** listed in the worksheet, one of three outcomes:

**`agrees`** — the document states the same fact. Add the document to that port's `why`, after the existing citation, naming the page or section: `… ; the manual's Voltages page gives the same 0 to 10V`. The value does not change. This port is now independently documented and no longer rests on the binary alone.

**`differs`** — the document states something else. Record the document's figure in a new `docRange` field beside the existing one (or `docPoly`, `docNormal` — same name with `doc` in front), and leave the measured value exactly as it is. Do not change what the entry says. Do not add a line about the discrepancy. Both figures simply sit in the data.

**`silent`** — the document does not address it. Nothing to add to the port; it is counted in your report.

A near miss is not agreement. "Accepts CV" where we record ±5V is `silent` on the range. "0 to 10V" where we record 0 to 8V is `differs`, not `agrees`. If the document is ambiguous, it is `silent`; say so in the report and quote the wording.

## What not to do

- **Do not change a measured value to match a document.** The measurement stands whatever the document says.
- **Do not write a line, a note or a clause about a disagreement.** This pass adds data, not prose. The entry's wording is finished.
- **Do not re-read the binary.** That work is done and is not what you are checking.
- **Do not go looking for documentation the maker did not declare** — no forum posts, no hardware manuals for a different product, no third-party wikis. The declared URL, and anything it links to as part of the same document, is the whole scope.

## Report

Per plugin: the URL you read and what kind of document it turned out to be; the three counts, `agrees` / `differs` / `silent`, for ports overall and for the `range` field specifically and for `poly` specifically; and every `differs` listed, with both figures and the wording the document uses.

That last list is the one a person will read. Keep it exact and keep it short.
